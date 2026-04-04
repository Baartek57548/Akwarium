# Testowanie

## Zakres

Po uproszczeniu repo testowanie skupia sie na firmware uruchamianym na fizycznym urzadzeniu.

## Materialy testowe

| Sciezka | Rola |
| --- | --- |
| `docs/manual_smoke_test.md` | checklista najwazniejszych scenariuszy runtime |
| `tools/testing/` | miejsce na pomocnicze notatki, fixture'y i artefakty testowe |

## Co warto sprawdzac po zmianach

- build firmware przez `PlatformIO`
- panel WWW i endpointy HTTP
- SSE na `GET /api/events`
- `STA/AP` oraz fallback do `AP`
- timeout `AP` po `90 s` bez klientow
- harmonogramy i zapis konfiguracji
- karmnik, serwo i fail-safe temperatury
- `HTTP OTA`
- nocny low power i light sleep

## Zalecany workflow

1. Zbudowac firmware.
2. Wgrac je na urzadzenie.
3. Przejsc `docs/manual_smoke_test.md`.
4. Dla zmian w power management sprawdzic zachowanie po zmroku lub na wymuszonym czasie nocnym.

## Ograniczenia

- repo nie zawiera obecnie zautomatyzowanych testow jednostkowych
- najwazniejsza walidacja nadal odbywa sie na fizycznym urzadzeniu
