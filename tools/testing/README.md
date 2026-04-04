# tools/testing

Katalog na uporzadkowane materialy pomocnicze zwiazane z weryfikacja firmware.

## Automatyczny smoke test WWW

Panel WWW ma tu lekki smoke test oparty o `Playwright`.

```powershell
npm ci
npm run test:web-smoke
```

Test uruchamia lokalny serwer statyczny dla `firmware/web`, mockuje API urzadzenia i sprawdza:

- brak krytycznych bledow JS
- brak poziomego overflow
- dzialanie mobilnej nawigacji
- przelaczanie sekcji i podstawowy render dashboardu

## Gdzie czytac dalej

- [Dokumentacja testowania](../../docs/testing.md)
- [Manual smoke test](../../docs/manual_smoke_test.md)
