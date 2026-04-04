# Testowanie

## Zakres

Po uproszczeniu repo testowanie skupia sie na firmware uruchamianym na fizycznym urzadzeniu, ale panel WWW ma tez lekki smoke test uruchamiany automatycznie w `CI`.

## Materialy testowe

| Sciezka | Rola |
| --- | --- |
| `docs/manual_smoke_test.md` | checklista najwazniejszych scenariuszy runtime |
| `tools/testing/` | miejsce na pomocnicze notatki, fixture'y i artefakty testowe |
| `tools/testing/web-dashboard.spec.js` | automatyczny smoke test panelu WWW i mockowanego API |

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
2. Dla zmian w panelu WWW uruchomic `npm run test:web-smoke` w `tools/testing/`.
3. Wgrac firmware na urzadzenie.
4. Przejsc `docs/manual_smoke_test.md`.
5. Dla zmian w power management sprawdzic zachowanie po zmroku lub na wymuszonym czasie nocnym.

## Ograniczenia

- repo nie zawiera obecnie zautomatyzowanych testow jednostkowych firmware
- najwazniejsza walidacja nadal odbywa sie na fizycznym urzadzeniu
