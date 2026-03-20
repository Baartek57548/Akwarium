# Development

## Zakres

Ten dokument opisuje zasady rozwoju projektu, konwencje architektoniczne i zalecany workflow przy zmianach w firmware, UI, emulatorach i warstwie shared.

## Zasady architektoniczne

### 1. Firmware pozostaje source of truth dla runtime urządzenia

Scheduling logic, fail-safe, hardware interface i power management muszą być definiowane w firmware, a nie w aplikacji operatorskiej.

### 2. UI korzysta z abstraction layer

Nowy kod UI powinien przechodzić przez:

- `IDeviceModeService`,
- `SelectableBluetoothService`,
- `IBluetoothService`.

Nie należy mieszać logiki widoku z transportem sprzętowym.

### 3. Shared contracts są warstwą graniczną

Zmiany w modelach lub payloadach powinny najpierw być ocenione w:

- `Aquarium.Models`,
- `Aquarium.Protocol`,
- `LegacyModelMappers`.

### 4. Dokumentacja ma być częścią zmiany

Zmiana protokołu, emulatora lub workflow developerskiego powinna aktualizować odpowiedni dokument w `docs/`.

## Konwencje kodu

### Firmware C++

- nowe reguły runtime umieszczaj w modułach domenowych, nie w kodzie UI OLED,
- używaj `ConfigValidation` dla wszystkich ścieżek zapisu konfiguracji,
- synchronizację między taskami prowadź przez `SharedState` albo jawne sekcje krytyczne,
- unikaj ciężkich operacji w callbackach BLE; deleguj je do bezpieczniejszego kontekstu runtime.

### C# / .NET

- modele i kontrakty trzymaj w `shared/`,
- logikę transportu utrzymuj w `Services/`,
- logikę widoku trzymaj w `ViewModels/`,
- emulator funkcjonalny powinien implementować te same kontrakty, których używa aplikacja.

### Dokumentacja

- język: polski techniczny,
- styl: konkretny, bez redundancji,
- opis katalogu lokalnego w lokalnym `README.md`,
- opis systemowy w `README.md` i `docs/`.

## Zalecany workflow zmian

### Zmiana logiki urządzenia

1. Zmodyfikuj firmware.
2. Oceń wpływ na `shared/` i protokół.
3. Zaktualizuj emulator funkcjonalny, jeśli wpływa na kontrakty.
4. Zaktualizuj dokumentację w `docs/firmware.md`, `docs/communication.md` lub `docs/testing.md`.

### Zmiana protokołu

1. Zmień `Aquarium.Protocol` i mappery kompatybilności.
2. Zmień firmware HTTP/BLE.
3. Zmień `BluetoothService` i `EmulatorBluetoothService`.
4. Uzupełnij dokumentację komunikacji.

### Zmiana UI

1. Utrzymaj niezależność warstwy widoku od sprzętu.
2. Zweryfikuj tryb `RealDevice` i `Emulator`.
3. Jeżeli zmiana dotyczy lokalnego OLED, sprawdź emulator `tools/simulator/`.

## Workflow repozytorium

- `README.md` ma prowadzić od poziomu systemowego do szczegółów.
- `docs/` zawiera dokumenty referencyjne i operacyjne.
- lokalne `README.md` mają tłumaczyć tylko kontekst katalogu i kierować dalej.

## Design Decisions

- Hybrydowy układ repo został zachowany, ale dokumentacja jawnie rozróżnia strukturę logiczną i fizyczną.
- Legacy wire format nie jest usuwany gwałtownie; zmiany muszą uwzględniać warstwę kompatybilności.
- Emulatory są traktowane jako część workflow deweloperskiego, a nie osobne poboczne narzędzia.

## Known Limitations

- Projekt nie ma jeszcze jednego, twardo egzekwowanego standardu testów automatycznych.
- Część architektury nadal odzwierciedla etap przejściowy między starszym układem repo a docelowym modelem warstwowym.
- Duże klasy, takie jak `MainViewModel` i `AkwariumV4.ino`, utrudniają małe, izolowane zmiany.

## Future Improvements

- Wprowadzenie checklisti PR dla zmian w protokole, firmware i dokumentacji.
- Rozbicie dużych modułów na mniejsze bounded contexts.
- Automatyczne testy regresyjne dla emulacji i kontraktów.
