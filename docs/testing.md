# Testowanie

## Zakres

Projekt posiada mieszany model testowania: scenariusze manualne, emulację funkcjonalną, emulację UI oraz punktowe raporty weryfikacyjne. Ten dokument opisuje, co jest obecnie testowane, a czego jeszcze brakuje.

## Struktura materiałów testowych

| Ścieżka | Rola |
| --- | --- |
| `docs/manual_smoke_test.md` | checklista testów manualnych na urządzeniu |
| `docs/technical_verification_report.md` | raport przeglądu technicznego i ryzyk |
| `apps/Aquarium.Emulator/` | emulator funkcjonalny urządzenia |
| `tools/simulator/` | emulator lokalnego UI OLED |
| `tools/testing/` | katalog na uporządkowane materiały pomocnicze |
| `tools/testy/` | eksperymenty, prototypy i starsze narzędzia testowe |

## Co jest testowane

### Firmware

Obszary możliwe do weryfikacji:

- scheduling logic,
- zachowanie API HTTP,
- zachowanie BLE,
- fail-safe temperatury,
- logika karmnika,
- przejścia OTA,
- power management i warunki sleep.

Aktualnie dominują:

- smoke testy manualne,
- testy pośrednie przez emulator,
- przegląd statyczny kodu.

### UI

Warstwa MAUI jest testowana głównie przez:

- ręczne scenariusze operatorskie,
- tryb `Emulator` w `SelectableBluetoothService`,
- walidację formularzy w runtime.

### Protokół

Warstwa `shared/` stabilizuje modele i mappery kompatybilności, ale repozytorium nie zawiera jeszcze pełnego pakietu automatycznych testów kontraktowych.

## Testy symulacyjne vs rzeczywiste

### Testy symulacyjne

Obejmują:

- `apps/Aquarium.Emulator/` do weryfikacji API i zachowania aplikacji,
- `tools/simulator/` do weryfikacji OLED state machine i renderingu,
- `tools/testy/Aquarium.ControllerEmulator/` do analizy struktury UI firmware.

Zaleta:

- szybki feedback bez fizycznego `ESP32-S3`.

Ograniczenie:

- brak pełnej zgodności z timingiem, driverami i zachowaniem hardware.

### Testy rzeczywiste

Obejmują urządzenie fizyczne oraz checklistę smoke:

- Wi-Fi `STA/AP`,
- panel HTTP,
- BLE pairing i reconnect,
- zapis konfiguracji,
- OTA,
- power management nocny,
- zgodność lokalnego UI z realnym hardware.

## Zalecany workflow testowy

1. Zweryfikować zmiany kontraktów i UI na emulatorze funkcjonalnym.
2. Sprawdzić rendering OLED w `tools/simulator/`.
3. Przejść checklistę z `docs/manual_smoke_test.md`.
4. Dla zmian w protokole lub power management wykonać test na fizycznym urządzeniu.

## Design Decisions

- Emulacja została potraktowana jako pierwsza linia obrony przed regresją, zanim dojdziemy do testów sprzętowych.
- Manual smoke test ma charakter produkcyjny i obejmuje najbardziej ryzykowne ścieżki operacyjne.
- Raport techniczny jest utrzymywany jako dokument pomocniczy dla refaktoryzacji i planowania długu technicznego.

## Known Limitations

- Brakuje zautomatyzowanych testów jednostkowych dla firmware i shared contracts.
- Brakuje pełnego CI uruchamiającego scenariusze emulatora oraz walidację payloadów.
- `tools/testy/` zawiera również materiały eksperymentalne, które nie zawsze odzwierciedlają produkcyjny workflow.

## Future Improvements

- Dodać testy kontraktowe dla `Aquarium.Protocol` i `LegacyModelMappers`.
- Zbudować zestaw regresyjnych scenariuszy emulatora uruchamianych z CLI.
- Wydzielić stabilne artefakty testowe od katalogu eksperymentalnego `tools/testy/`.
