# Akwarium UI (React)

Aplikacja mobilna (SPA) dla sterownika Akwarium PRO, podlaczona do realnego API firmware:

- `GET /api/status`
- `GET /api/logs`
- `POST /api/action`
- `POST /settime?epoch=<unix>`
- `POST /update`

## Tech stack

- React 19 + TypeScript
- Vite 7
- React Router 7
- Tailwind CSS 4
- Recharts
- Lucide Icons

## Uruchomienie lokalne (PowerShell)

1. Przejdz do katalogu aplikacji:

```powershell
cd testy\akwarium-ui
```

2. Zainstaluj paczki:

```powershell
npm.cmd install
```

3. (Opcjonalnie, zalecane dla dev) ustaw proxy do ESP32:

```powershell
$env:VITE_API_PROXY_TARGET="http://192.168.1.105"
```

4. Uruchom dev server:

```powershell
npm.cmd run dev
```

Domyslnie: `http://localhost:5173`

5. W aplikacji wejdz w `Ustawienia -> Polaczenie z urzadzeniem` i wpisz IP/URL ESP32
   (np. `192.168.1.105`). Wybor zapamietuje sie lokalnie do czasu recznego resetu.

## Uwaga o `npm` w PowerShell

Jesli masz blad `npm.ps1 cannot be loaded because running scripts is disabled`, uzywaj:

- `npm.cmd install`
- `npm.cmd run dev`
- `npm.cmd run build`

## Konfiguracja API

Aplikacja wywoluje endpointy relatywnie (`/api/...`, `/settime`, `/update`), ale mozesz
tez ustawic runtime target API bezposrednio w UI (`Ustawienia -> Polaczenie z urzadzeniem`).

W trybie developerskim mozesz przekierowac ruch do ESP32 przez proxy Vite:

- `VITE_API_PROXY_TARGET=http://<IP_ESP32>`

Opcjonalnie mozna wymusic bezposredni bazowy URL:

- `VITE_API_BASE_URL=http://<IP_ESP32>`

Uwaga: dla bezposredniego polaczenia z `http://localhost:5173` firmware musi zwracac
naglowki CORS dla `/api/*`, `/settime`, `/update`.

## Skrypty

- `npm.cmd run dev` - tryb developerski
- `npm.cmd run build` - build produkcyjny
- `npm.cmd run preview` - podglad buildu
- `npm.cmd run lint` - lint kodu

## Co jest wspierane przez UI

- Podglad statusu temperatury, baterii, relayow i sieci
- Reczne sterowanie swiatlem i filtrem (`set_light`, `set_filter`)
- Sterowanie AP (`set_ap`)
- Reczne karmienie (`action=feed_now`)
- Manualny override serwa (`set_servo` / `clear_servo`)
- Zapis harmonogramu i temperatury (`save_schedule`)
- Sterowanie `alwaysScreenOn` (`set_always_screen`)
- Sterowanie opcja grzalki (`set_heater_enabled`)
- Podglad i czyszczenie logow krytycznych (`clear_critical_logs`)
- Synchronizacja czasu (`/settime`)
- Upload OTA (`/update`)
