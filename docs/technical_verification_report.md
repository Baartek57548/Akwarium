# Raport weryfikacji technicznej

## Cel

Raport zbiera najważniejsze ryzyka techniczne zidentyfikowane podczas przeglądu repozytorium. Nie zastępuje testów automatycznych ani manualnych, ale pomaga priorytetyzować refaktoryzację.

## Zakres przeglądu

- firmware `ESP32` w `firmware/src/`,
- aplikacja `MAUI` w `mobile-app/`,
- emulatory i narzędzia w `apps/` oraz `tools/`,
- dokumentacja operacyjna i testowa.

## Najważniejsze obserwacje

### 1. Duże moduły o szerokiej odpowiedzialności

Najbardziej obciążone odpowiedzialnością pliki:

- `firmware/src/AkwariumV4.ino`,
- `mobile-app/ViewModels/MainViewModel.cs`.

Skutek:

- trudniejsza nawigacja po kodzie,
- wyższe ryzyko regresji przy zmianach punktowych,
- słabsza testowalność.

### 2. Złożona warstwa kompatybilności protokołu

Projekt utrzymuje równolegle:

- legacy compact payloady firmware,
- canonical modele w `shared/`,
- mappery kompatybilności.

Skutek:

- większy koszt utrzymania,
- wyższe ryzyko rozjazdu dokumentacji z wire format.

### 3. Ograniczona automatyzacja testów

Repozytorium ma solidne materiały manualne i emulatory, ale nie ma jeszcze pełnego zestawu automatycznych testów:

- kontraktowych,
- jednostkowych,
- integracyjnych.

### 4. Złożoność ścieżek emulatorów

Istnieją trzy różne podejścia do emulacji:

- emulator funkcjonalny,
- emulator renderingu OLED,
- emulator analityczny UI.

Skutek:

- wysoka elastyczność,
- ale również wyższy koszt onboardingowy i ryzyko niejednoznaczności.

## Mocne strony architektury

- dobra separacja `shared models`, `protocol` i `emulator core`,
- zachowana kompatybilność z istniejącym firmware,
- wyraźna obecność fail-safe i walidacji w firmware,
- sensowny abstraction layer dla wyboru transportu w aplikacji MAUI,
- dojrzały kierunek architektury mimo przejściowego układu repo.

## Obszary wysokiego ryzyka

### Firmware

- zależność dużej części lokalnego UI od jednego pliku wejściowego,
- trudne semantycznie sterowanie grzałką wynikające z obecnego hardware interface,
- ryzyko dalszego rozrostu callbacków BLE i HTTP bez dodatkowej separacji.

### UI

- skupienie wielu zachowań w jednym view modelu,
- częściowa niejednoznaczność między logiczną a fizyczną strukturą aplikacji.

### Dokumentacja

- historycznie występowały duplikaty i rozjazdy między opisem a stanem repo,
- brakowało jednego systemu dokumentów prowadzących od ogółu do szczegółu.

## Rekomendacje

1. Rozdzielić dokumentację systemową od dokumentacji lokalnej katalogów.
2. Stopniowo dzielić `AkwariumV4.ino` i `MainViewModel` na mniejsze moduły.
3. Dodać testy kontraktowe dla `Aquarium.Protocol` i mapperów.
4. Ujednolicić ścieżkę developerską dla emulatorów.
5. Traktować zmianę dokumentacji jako obowiązkową część zmian w protokole i runtime.

## Ocena ogólna

Projekt jest technicznie dojrzały kierunkowo, ale nadal znajduje się w fazie porządkowania architektury i dokumentacji. Największą wartość na teraz przyniesie dalsza stabilizacja kontraktów, automatyzacja testów oraz redukcja modułów o zbyt szerokiej odpowiedzialności.
