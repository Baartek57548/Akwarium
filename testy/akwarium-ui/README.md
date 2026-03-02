# Akwarium UI (React)

Interfejs mobilny (single-page app) dla sterownika akwarium.

## Tech stack

- React 19 + TypeScript
- Vite 7
- React Router 7
- Tailwind CSS 4
- Recharts
- Lucide Icons

## Uruchomienie

```bash
npm install
npm run dev
```

Domyślnie: `http://localhost:5173`

## Komendy

- `npm run dev` - tryb developerski
- `npm run build` - build produkcyjny
- `npm run preview` - podgląd buildu
- `npm run lint` - lint kodu

## Struktura

- `src/app/App.tsx` - root aplikacji
- `src/app/routes.ts` - routing
- `src/app/components/Layout.tsx` - shell + dolna nawigacja
- `src/app/pages/*` - widoki:
  - `Dashboard`
  - `Schedule`
  - `Feeder`
  - `Logs`
  - `Settings`
- `src/app/deviceContext.tsx` - provider + mock state urządzenia
- `src/app/useDevice.ts` - hook do dostępu do stanu urządzenia

## Uwagi

- Aplikacja działa na mockach stanu (`DeviceProvider`) i nie wymaga backendu.
- API/OTA są pokazane jako elementy UI/UX i logika demonstracyjna.
