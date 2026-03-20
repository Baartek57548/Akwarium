# UI

## Zakres

Warstwa UI w repozytorium nie jest jednorodna. Obejmuje:

- aplikację operatorską `MAUI` w `mobile-app/`,
- lokalny panel HTTP serwowany przez firmware,
- eksperymentalne materiały webowe w `tools/testy/WWWpage`.

Ten dokument opisuje przede wszystkim aktywną warstwę operatorską oraz jej integrację z firmware.

## Aplikacja MAUI

### Lokalizacja

Rzeczywisty kod hosta UI znajduje się w `mobile-app/`. Katalogi `apps/Aquarium.Mobile/` i `apps/Aquarium.Desktop/` mają obecnie charakter dokumentacyjny.

### Funkcje użytkownika

Główne funkcje dostępne z `MainViewModel` i `MainPage.xaml`:

- dashboard statusu urządzenia,
- edycja harmonogramów pracy,
- sterowanie karmnikiem i serwem,
- przegląd logów,
- ustawienia połączenia,
- skanowanie BLE i łączenie z urządzeniem,
- przełączanie trybu `RealDevice` / `Emulator`,
- wybór pakietu firmware i `BLE OTA`.

### Struktura UI

Aktualny interfejs jest zorganizowany wokół kart:

- `Dashboard`,
- `Harmonogram`,
- `Feeder`,
- `Logs`,
- `Settings`.

Warstwa prezentacji opiera się na `MVVM`:

- `MainPage.xaml` - układ widoku,
- `MainPage.xaml.cs` - inicjalizacja strony,
- `MainViewModel` - stan ekranu, walidacja formularzy i komendy,
- `Services/*` - transport, OTA, tryb urządzenia i wybór pakietu firmware.

## Communication flow aplikacji

### Abstraction layer

UI nie komunikuje się bezpośrednio z konkretnym transportem. Przepływ wygląda następująco:

1. `MainViewModel` wywołuje komendę użytkownika.
2. `SelectableBluetoothService` wybiera aktywny transport.
3. Transportem jest:
   - `BluetoothService` dla fizycznego BLE,
   - `EmulatorBluetoothService` dla emulatora.
4. Wynik mapowany jest na modele `AquariumStatus`, `AquariumSettings`, `AquariumDeviceInfo`.

### Tryby pracy

- `RealDevice` - połączenie z prawdziwym `ESP32-S3`.
- `Emulator` - połączenie z `EmulatedDeviceCore`.

Tryb połączenia jest utrzymywany przez `DeviceModeService`.

## Panel WWW firmware

Firmware udostępnia lokalny panel HTTP. To osobne UI, które działa bez aplikacji MAUI i jest serwowane bezpośrednio z urządzenia.

Zakres panelu:

- status runtime,
- zapis harmonogramów,
- akcje sterujące,
- logi,
- `HTTP OTA`.

Panel ten jest ważny operacyjnie, ale nie stanowi tej samej bazy kodu co MAUI.

## Eksperymentalne materiały webowe

`tools/testy/WWWpage/` zawiera statyczny prototyp interfejsu WWW. Należy traktować go jako materiał pomocniczy lub demonstracyjny, a nie produkcyjny source of truth.

## OTA w UI

`FirmwarePackageService` analizuje obraz `.bin` przed wysyłką:

- weryfikuje magic obrazów ESP,
- sprawdza `chip id` dla `ESP32-S3`,
- odczytuje marker `AQFWMETA`,
- prezentuje metadane wersji, daty builda i IDF.

To ogranicza ryzyko wysłania niewłaściwego obrazu do urządzenia.

## Design Decisions

- UI jest klientem kontraktów, a nie miejscem implementacji scheduling logic.
- Transport został ukryty za `IBluetoothService`, aby zachować ten sam model interakcji dla urządzenia i emulatora.
- Walidacja formularzy w `MainViewModel` wykorzystuje profile walidacyjne zwracane przez firmware.

## Known Limitations

- Kod MAUI skupia dużą część logiki w `MainViewModel`, co utrudnia dalszy podział na mniejsze bounded contexts.
- Widok logów w MAUI nie streamuje pełnych logów firmware przez BLE; pokazuje głównie wyniki akcji i logi aplikacji.
- W repozytorium nie ma oddzielnego, aktywnego projektu web/mobile poza `mobile-app/`; ścieżki `apps/Aquarium.Mobile` i `apps/Aquarium.Desktop` nie zawierają pełnych hostów.

## Future Improvements

- Rozbicie `MainViewModel` na mniejsze view modele per obszar funkcjonalny.
- Dodanie pełniejszego kontraktu logów i eventów asynchronicznych po BLE.
- Ujednolicenie relacji między panelem WWW firmware a aplikacją operatorską.
