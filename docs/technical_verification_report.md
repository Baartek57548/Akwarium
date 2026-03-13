# Pełna weryfikacja techniczna repozytorium Akwarium

## Zakres i metodologia
- Przeprowadzono przegląd statyczny kluczowych modułów: firmware ESP32 (`firmware/src`), aplikacja mobilna MAUI (`mobile-app`) oraz interfejs web testowy (`testy/WWWpage`).
- Uruchomiono lokalne kontrole możliwe w środowisku (składnia JS, kompilacja skryptów Python).
- Próby pełnej kompilacji firmware i aplikacji mobilnej nie były możliwe z powodu braku narzędzi (`pio`, `dotnet`) w środowisku wykonawczym.

## 1) Poprawność składni i potencjalne błędy kompilacji

### Wynik
- `testy/WWWpage/script.js` przechodzi `node --check` (brak błędów składni).
- Skrypty Python `scripts/build_metadata.py` i `firmware/scripts/build_metadata.py` przechodzą `py_compile`.
- Niezweryfikowana pełna kompilacja C++/C# z powodu ograniczeń środowiska.

## 2) Błędy logiczne i ryzyka nieprawidłowego działania

### Problem A — reset licznika nieudanych autoryzacji BLE nie działa
**Lokalizacja:** `firmware/src/BleManager.cpp` (metoda `onAuthenticationComplete`).

**Opis:**
W gałęzi `if (!auth_cmpl.success)` i w gałęzi `else` użyto dwóch różnych zmiennych:
`static int failedAuthCount = 0;` zadeklarowanych w różnych blokach. Oznacza to, że „reset” w gałęzi sukcesu nie zeruje licznika inkrementowanego przy błędach.

**Konsekwencja:**
Niewłaściwe zarządzanie bondami i potencjalnie agresywne czyszczenie bondów mimo poprawnych późniejszych połączeń.

**Poprawiona wersja kodu (przykład):**
```cpp
class SecurityCallbacks : public BLESecurityCallbacks {
private:
  static int failedAuthCount;

public:
  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) override {
    if (!auth_cmpl.success) {
      failedAuthCount++;
      // ...
      if (failedAuthCount > 2) {
        cleanupExcessBonds(0);
        failedAuthCount = 0;
      }
    } else {
      failedAuthCount = 0;
      // ...
    }
  }
};

int SecurityCallbacks::failedAuthCount = 0;
```

### Problem B — możliwy data race na flagach wznowienia advertisingu BLE
**Lokalizacja:** `firmware/src/BleManager.cpp` (`resumeAdvertisingPending`, `resumeAdvertisingAtMs`).

**Opis:**
Zmienne są modyfikowane i odczytywane z różnych ścieżek (callbacki BLE + pętla `BleManager::update`) bez sekcji krytycznej/mutexa.

**Konsekwencja:**
Niedeterministyczne zachowanie: advertising może nie wznowić się na czas lub wznowić się błędnie.

**Poprawiona wersja kodu (przykład):**
```cpp
static void setResumeAdvertising(bool pending, unsigned long atMs) {
  portENTER_CRITICAL(&bleStateMux);
  resumeAdvertisingPending = pending;
  resumeAdvertisingAtMs = atMs;
  portEXIT_CRITICAL(&bleStateMux);
}

static bool shouldResumeAdvertisingNow() {
  portENTER_CRITICAL(&bleStateMux);
  bool ready = resumeAdvertisingPending && !deviceConnected &&
               (static_cast<long>(millis() - resumeAdvertisingAtMs) >= 0);
  if (ready) {
    resumeAdvertisingPending = false;
  }
  portEXIT_CRITICAL(&bleStateMux);
  return ready;
}
```

### Problem C — brak walidacji elementów DOM może generować wyjątek runtime
**Lokalizacja:** `testy/WWWpage/script.js`, funkcje `triggerFeed()` i `simulateOTA()`.

**Opis:**
Kod zakłada istnienie elementów (`modal`, `icon`, `text`, `p`, `firmware-file`) i od razu dereferencjuje obiekty. Jeśli HTML się zmieni, pojawi się `TypeError`.

**Konsekwencja:**
Przerwanie działania UI i brak obsługi akcji użytkownika.

**Poprawiona wersja kodu (przykład):**
```js
function triggerFeed() {
  const modal = document.getElementById('feed-modal');
  const icon = document.getElementById('modal-icon');
  const text = document.getElementById('modal-text');
  const p = document.getElementById('modal-subtext');

  if (!modal || !icon || !text || !p) {
    console.warn('Brak wymaganych elementów modala karmienia.');
    return;
  }

  modal.style.display = 'flex';
  // ...
}
```

## 3) Fragmenty podatne na wyjątki i błędy graniczne
- Ryzyko `TypeError` w JS (brak null-check dla DOM).
- Potencjalne warunki wyścigu w BLE (flagi wznowienia advertisingu).
- Brak sprawdzenia wyniku `preferences.begin(...)` w konfiguracji firmware (przy błędzie NVS aplikacja działa dalej jakby storage był gotowy).

## 4) Ocena architektury

### Mocne strony
- Dobra separacja domen firmware (BLE, OTA, konfiguracja, zasilanie, UI).
- Użycie snapshotów stanu i struktur patchowania konfiguracji.
- W MAUI: czytelny model MVVM, sensowny podział usług i ViewModel.

### Słabe strony
- Zbyt duża odpowiedzialność pojedynczych plików (`AkwariumV4.ino`, `MainViewModel.cs`).
- Część logiki stanu BLE jest oparta na globalach i utrudnia formalną kontrolę współbieżności.
- Niewystarczająco defensywna obsługa błędów w kodzie demonstracyjnym WWW.

## 5) Miejsca do optymalizacji
- `VideoTask` odczytuje i mapuje pełną konfigurację przy każdej klatce (~24 FPS); można cache’ować konfigurację i odświeżać ją tylko po zmianie.
- Część stringów/logów i JSON może być ograniczona (embedded RAM/heap pressure).
- W MAUI warto rozbić `MainViewModel` na mniejsze VM-y per zakładka (dashboard/schedule/logs/settings), co uprości utrzymanie i testowanie.

## 6) Dobre praktyki językowe
- C++: w wielu miejscach dobre użycie `const`, snapshotów, enkapsulacji modułów.
- C#: poprawne użycie `ObservableObject`, command pattern i nullable reference types.
- JS: zalecane dopisanie guard clauses oraz unikanie „gołych” `alert` w kodzie produkcyjnym.

## 7) Lista wszystkich problemów (skrót)
1. Błąd logiczny w liczniku `failedAuthCount` (dwie różne statyczne zmienne o tej samej nazwie w różnych blokach).
2. Ryzyko race condition dla `resumeAdvertisingPending`/`resumeAdvertisingAtMs`.
3. Ryzyko wyjątków `TypeError` w `triggerFeed()` i `simulateOTA()` przez brak walidacji DOM.
4. Brak sprawdzenia wyniku inicjalizacji NVS (`preferences.begin`) w `ConfigManager::init()`.
5. Nadmierna odpowiedzialność i rozmiar klas/modułów (`AkwariumV4.ino`, `MainViewModel.cs`).

## Ogólna ocena jakości kodu
**7/10**

## Rekomendacje refaktoryzacji
1. Wydzielić machine-state UI oraz harmonogram do osobnych komponentów (firmware).
2. Dodać jednolity adapter synchronizacji dla wszystkich flag BLE modyfikowanych między callbackami i pętlą główną.
3. Wprowadzić testy jednostkowe walidacji konfiguracji (zakresy godzin/minut/temperatur).
4. Podzielić `MainViewModel` na mniejsze VM-y i dodać testy dla mapowania DTO↔UI.
5. W JS dodać warstwę bezpiecznych helperów DOM (`getRequiredElement`) i spójny handling błędów UI.
