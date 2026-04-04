# Changelog

Wszystkie istotne zmiany w tym repozytorium beda zapisywane w tym pliku.

## [2.0.0] - 2026-04-04

Nowy punkt startowy dla uproszczonego repozytorium `firmware-only`.

### Added

- panel WWW serwowany bezposrednio z firmware wraz z `REST API` i `SSE`
- automatyczne zamkniecie sesji `AP` po `90 s` bez klientow
- uproszczona dokumentacja dla jednego celu repo: firmware, OLED i lokalny panel WWW

### Changed

- repozytorium zostalo zredukowane do utrzymania firmware `ESP32-S3`
- wersjonowanie przechodzi na nowa baze po duzym uproszczeniu struktury projektu

### Removed

- `BLE` z firmware i panelu WWW
- aplikacje mobilne i desktopowe
- emulatory, symulatory i projekty `.NET`
