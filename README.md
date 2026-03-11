# Sterownik Akwarium ESP32-S3

Zaawansowany kontroler akwarium oparty na ESP32-S3 z systemem operacyjnym FreeRTOS. Projekt obsługuje kompleksowe zarządzanie ekosystemem akwarium poprzez multiple interfejsy użytkownika: lokalny wyświetlacz OLED, aplikację mobilną .NET MAUI oraz web panel.

## Główne Funkcje Systemu

### Zarządzanie urządzeniami
- **Oświetlenie**: Harmonogram dzien/nocy z trybami pracy
- **Filtracja**: Sterowanie filtrem z harmonogramem czasowym
- **Napowietrzanie**: Kontrola serwa DM-S0090 z harmonogramem
- **Ogrzewanie**: Regulacja temperatury z histerezą i zabezpieczeniami
- **Karmienie**: Automatyczny karmnik z czujnikiem położenia i trybami awaryjnymi

### Interfejsy użytkownika
- **Wyświetlacz OLED 128x32**: Interfejs lokalny z 3 przyciskami fizycznymi
- **Aplikacja mobilna**: .NET MAUI (Windows/Android) z komunikacją BLE
- **Web panel**: Osadzony serwer HTTP z OTA i captive portal
- **BLE Server**: Komunikacja bezprzewodowa z aplikacjami mobilnymi

### Zarządzanie energią
- **Bateria RTC**: Monitorowanie CR2032 z systemem ostrzeżeń
- **Light Sleep**: Oszczędzanie energii w nocy przy spełnieniu warunków
- **Power Management**: Inteligentne zarządzanie zasilaniem

### Systemowe
- **RTC DS3231**: Precyzyjny zegar czasu rzeczywistego z backupiem baterii
- **Temperatura DS18B20**: Cyfrowy czujnik temperatury wody
- **Logi systemowe**: Bufor kołowy w RAM i trwałe logi krytyczne
- **OTA Updates**: Aktualizacje firmware przez HTTP i BLE

## Architektura Projektu

### Struktura Repozytorium

```
Akwarium/
├── firmware/                 # Firmware ESP32-S3 (PlatformIO)
│   ├── src/                  # Źródła firmware
│   │   ├── AkwariumV4.ino    # Główny plik Arduino
│   │   ├── SystemController.* # Główna logika sterowania
│   │   ├── AkwariumWifi.*    # WiFi, HTTP server, OTA
│   │   ├── BleManager.*      # BLE GATT server
│   │   ├── ConfigManager.*   # Zarządzanie konfiguracją
│   │   ├── TemperatureController.* # Sterowanie temperaturą
│   │   ├── FeederController.* # Kontroler karmnika
│   │   ├── ServoController.* # Sterowanie serwem
│   │   ├── AquariumAnimation.* # UI/OLED renderowanie
│   │   └── [inne moduły]
│   ├── platformio.ini        # Konfiguracja PlatformIO
│   └── README.md             # Dokumentacja firmware
├── mobile-app/               # Aplikacja .NET MAUI
│   ├── MainPage.xaml         # Główny interfejs
│   ├── Services/             # Serwis BLE i komunikacji
│   ├── ViewModels/           # MVVM ViewModels
│   └── AquariumController.Mobile.csproj
├── docs/                     # Dokumentacja projektu
│   └── manual_smoke_test.md  # Testy manualne
├── platformio.ini            # Konfiguracja root PlatformIO
└── README.md                 # Ten plik
```

## Kluczowe Moduły Firmware

### SystemController
Centralny moduł zarządzający całym systemem:
- Inicjalizacja sprzętu i systemu
- Główna pętla decyzyjna (Core 1)
- Zarządzanie sensorami i aktuatorami
- Power management i sleep modes

### AkwariumWifi
Obsługa łączności sieciowej:
- Tryb STA (połączenie z domowym WiFi)
- Tryb AP (Access Point dla konfiguracji)
- HTTP server z web panel
- Captive portal DNS
- OTA updates przez HTTP

### BleManager
Komunikacja Bluetooth Low Energy:
- GATT server z charakterystykami statusu
- Komendy sterujące i ustawienia
- Parowanie z kodem PIN
- OTA przez BLE

### ConfigManager
Zarządzanie konfiguracją:
- Trwałe przechowywanie w Preferences/NVS
- CRC32 dla weryfikacji integralności
- Walidacja i clamp wartości
- Wersjonowanie konfiguracji

### TemperatureController
Sterowanie temperaturą:
- Odczyt z DS18B20 (OneWire)
- Regulacja grzałki z histerezą
- Ograniczenie przełączeń (120s min)
- Statystyki dzienne (min/max)

### FeederController
Automatyczny karmnik:
- Silnik z przekaznikiem
- Czujnik położenia z logiką stanów
- Safety timeout (15s)
- Tryby pracy: manualny, harmonogram, czujnik

## Wymagania Sprzętowe

### Platforma docelowa
- **Mikrokontroler**: ESP32-S3 (240MHz, Dual Core)
- **Płytka**: ESP32-S3 Zero 4MB Flash
- **Framework**: Arduino + FreeRTOS

### Podłączenia sprzętowe

```
Przyciski:
  BUTTON_UP      GPIO15
  BUTTON_SELECT  GPIO16  
  BUTTON_DOWN    GPIO14

Czujniki i wejścia:
  ONE_WIRE_BUS   GPIO1    (DS18B20 temperatura)
  FEEDER_SENSOR  GPIO12   (czujnik karmnika)
  BAT_ADC_PIN    GPIO7    (ADC baterii RTC)
  BAT_EN_PIN     GPIO10   (włączanie pomiaru baterii)

Wyjścia sterujące:
  HEATER_PIN     GPIO2    (grzałka - HIGH=ON)
  LIGHT_PIN      GPIO5    (oświetlenie - LOW=ON)
  PUMP_PIN       GPIO4    (pompa/filtr - LOW=ON)
  FEEDER_PIN     GPIO3    (karmnik - LOW=ON)
  SERVO_PIN      GPIO6    (serwo napowietrzania)

Komunikacja:
  I2C SDA        GPIO8    (OLED, RTC)
  I2C SCL        GPIO9    (OLED, RTC)
```

### Komponenty peryferyjne
- **Wyświetlacz**: OLED SSD1306 128x32 (I2C)
- **RTC**: DS3231 (I2C) z backupem CR2032
- **Temperatura**: DS18B20 (OneWire)
- **Serwo**: DM-S0090 90° (napowietrzanie)
- **Przekaźniki**: 5V sterowane stanem niskim

## Wymagania Programowe

### Firmware
- **PlatformIO**: Build system
- **Arduino Framework**: ESP32-S3 support
- **Biblioteki**:
  - U8g2 (OLED)
  - RTClib (DS3231)
  - DallasTemperature (DS18B20)
  - OneWire
  - ESP32Servo
  - ArduinoJson

### Aplikacja mobilna
- **.NET 10 SDK** z MAUI workload
- **Platformy**: Windows 10/11, Android 21+
- **Biblioteki**:
  - CommunityToolkit.Mvvm (MVVM)
  - Plugin.BLE (Bluetooth)

### Narzędzia deweloperskie
- Git
- Python 3.x
- PlatformIO CLI
- Visual Studio/VS Code

## Instrukcja Kompilacji

### Firmware (PlatformIO)

Z katalogu głównego repozytorium:

```bash
# Kompilacja
python -m platformio run -e esp32-s3-devkitc-1

# Upload do urządzenia
python -m platformio run -e esp32-s3-devkitc-1 -t upload

# Monitor szeregowy
python -m platformio device monitor -b 115200
```

Alternatywnie z katalogu `firmware/`:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

### Aplikacja mobilna (.NET MAUI)

```bash
# Instalacja workload MAUI (pierwszy raz)
dotnet workload install maui

# Kompilacja Windows
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0

# Kompilacja Android
dotnet build mobile-app/AquariumController.Mobile.csproj -f net10.0-android

# Uruchomienie Windows
dotnet run --project mobile-app/AquariumController.Mobile.csproj -f net10.0-windows10.0.19041.0
```

## Instrukcja Wgrywania Firmware

### Przygotowanie
1. Podłącz ESP32-S3 przez USB
2. Zainstaluj sterowniki CP210x/CH340 (jeśli wymagane)
3. Sprawdź port COM w Device Manager

### Wgrywanie przez PlatformIO
```bash
# Z katalogu głównego
python -m platformio run -e esp32-s3-devkitc-1 -t upload

# Lub z firmware/
pio run -t upload
```

### OTA Updates
- **Web panel**: `POST /update` z plikiem `.bin`
- **Aplikacja mobilna**: zakładka SYSTEM/OTA (BLE)

## Instrukcja Konfiguracji

### 1. Konfiguracja sekretów

Utwórz plik `firmware/src/arduino_secrets.h`:

```cpp
#pragma once

#define SECRET_SSID "TwojaSiecWiFi"
#define SECRET_PASS "HasloWiFi"
#define AP_SSID "AkwariumAP"
#define AP_PASSWORD "12345678"
#define SECRET_BLE_PASSKEY 654321
```

Szablon dostępny w `firmware/src/arduino_secrets.template.h`.

### 2. Pierwsze uruchomienie
1. Wgraj firmware
2. Urządzenie uruchomi się z domyślną konfiguracją
3. Użyj przycisków do nawigacji po menu OLED
4. Skonfiguruj harmonogramy i ustawienia

### 3. Konfiguracja WiFi
1. Z menu OLED wybierz "Wifi"
2. Uruchomi się Access Point
3. Połącz z `AkwariumAP` (hasło: `12345678`)
4. Otwórz `http://192.168.4.1/`
5. Skonfiguruj połączenie z domowym WiFi

### 4. Synchronizacja czasu
- **Web panel**: `POST /settime?epoch=<unix_timestamp>`
- **Menu OLED**: Ustawienia → Data i Czas
- **Automatyczna**: NTP przy połączeniu WiFi

## Opis Działania Systemu

### Główna pętla systemu
System działa na dwóch rdzeniach ESP32-S3:

**Core 0 (VideoTask)**:
- Obsługa wyświetlacza OLED (24 FPS)
- UI state machine i przyciski
- Renderowanie interfejsu
- Power management

**Core 1 (loop)**:
- SystemController::update()
- Odczyt sensorów
- Decyzje sterujące
- Komunikacja sieciowa
- BLE updates

### Harmonogramy
System obsługuje 5 głównych harmonogramów:
- **Oświetlenie**: 10:00-21:30 (domyslnie)
- **Napowietrzanie**: 10:00-19:00
- **Filtracja**: 10:30-20:30
- **Temperatura**: stała regulacja 25.0°C
- **Karmienie**: 18:00, codziennie

### Tryby pracy
- **Normalny**: Pełna funkcjonalność
- **Menu**: Interakcja z użytkownikiem
- **Quiet**: Ograniczona aktywność
- **Feeding**: Tymczasowe wstrzymanie urządzeń
- **Access Point**: Tryb konfiguracji sieci
- **Light Sleep**: Oszczędzanie energii

### Bezpieczeństwo
- **Temperatura**: Histereza 0.5°C, max 30°C
- **Karmnik**: Safety timeout 15s
- **Serwo**: Auto-off po 5 minutach
- **Bateria**: Ostrzeżenia poniżej 2.2V
- **OTA**: Weryfikacja rozmiaru i typu pliku

## Przykładowe Użycie

### Podstawowa obsługa przez OLED
```
1. HOME - Wyświetl status systemu
2. SELECT - Wejdź do MENU
3. DOWN - Nawiguj w menu
4. SELECT - Wybierz opcję
5. UP - Powrót do poprzedniego ekranu
```

### Manualne karmienie
```
Przytrzymaj wszystkie 3 przyciski przez 1 sekundę
```

### Web panel
```
1. Połącz z urządzeniem przez WiFi
2. Otwórz przeglądarkę: http://<IP_urzadzenia>/
3. Przeglądaj status, logi, ustawienia
4. Wykonaj akcje przez API
```

### Aplikacja mobilna
```
1. Uruchom aplikację Windows/Android
2. Włącz Bluetooth
3. Sparuj z urządzeniem (PIN z sekretów)
4. Przeglądaj status i steruj urządzeniami
5. Aktualizuj firmware przez zakładkę OTA
```

## Troubleshooting

### Problemy z połączeniem WiFi
- Sprawdź poprawność sekretów w `arduino_secrets.h`
- Upewnij się że sieć WiFi jest w zasięgu
- Zrestartuj urządzenie
- Użyj trybu AP do rekonfiguracji

### Problemy z BLE
- Sprawdź kod PIN w `SECRET_BLE_PASSKEY`
- Upewnij się że urządzenie jest w ekranie Bluetooth
- Wyczyść stare połączenia w systemie
- Zrestartuj aplikację mobilną

### Problemy z wyświetlaczem OLED
- Sprawdź połączenia I2C (GPIO8/9)
- Upewnij się że zasilanie jest stabilne
- Sprawdź kontrast w kodzie (255)
- Zresetuj urządzenie

### Problemy z temperaturą
- Sprawdź podłączenie DS18B20 (GPIO1)
- Upewnij się że rezystor 4.7k jest podłączony
- Sprawdź czy czujnik jest rozpoznawany
- Wyczyść logi i zrestartuj

### Problemy z karmieniem
- Sprawdź podłączenie silnika (GPIO3)
- Sprawdź czujnik (GPIO12)
- Upewnij się że przekaznik działa
- Sprawdź safety timeout

### Problemy z OTA
- Upewnij się że plik `.bin` jest poprawny
- Sprawdź połączenie sieciowe
- Upewnij się że jest wystarczająca pamięć
- Monitoruj logi podczas aktualizacji

## Development

### Struktura kodu
- **Modułowa**: Każda funkcjonalność w osobnej klasie
- **FreeRTOS**: Dual core z taskami
- **SharedState**: Bezpieczna komunikacja między rdzeniami
- **CRC32**: Integralność konfiguracji
- **Logowanie**: Wielopoziomowe systemowe

### Dodawanie nowych funkcji
1. Utwórz nową klasę w osobnych plikach `.h/.cpp`
2. Dodaj inicjalizację w `SystemController::init()`
3. Dodaj update w `SystemController::update()`
4. Zintegruj z UI (OLED/BLE/Web)
5. Dodaj pola konfiguracyjne w `ConfigData.h`

### Testowanie
- Manual smoke test: `docs/manual_smoke_test.md`
- Testy jednostkowe: brak (do implementacji)
- Testy integracyjne: na sprzęcie docelowym

### Debugowanie
- Serial monitor: 115200 baud
- Logi systemowe: bufor kołowy 20 wpisów
- Logi krytyczne: trwałe w Preferences
- Web panel: `/api/logs`

### Wersjonowanie
- Firmware: `CONFIG_VERSION` w `ConfigData.h`
- Aplikacja: `ApplicationDisplayVersion` w `.csproj`
- Build metadata: automatycznie przez `build_metadata.py`

## Assumptions / Notes

### Założenia projektowe
- System działa w rzeczywistym akwarium i został przetestowany w praktyce
- Logika sterowania urządzeniami jest funkcjonalna i sprawdzona
- Konfiguracja domyślna jest zoptymalizowana dla typowego akwarium słodkowodnego

### Informacje niejednoznaczne
- Dokładny model serwa napowietrzania (przyjęto DM-S0090 90°)
- Moc grzałki i typ przekaznika (zależne od implementacji)
- Precyzja czujnika temperatury (DS18B20 ±0.5°C)
- Pojemność akwarium (system uniwersalny)

### Ograniczenia
- Brak interfejsu Ethernet (tylko WiFi/BLE)
- Brak zewnętrznych sensorów pH/twardości
- Brak integracji z chmurą (lokalny system)
- Brak testów jednostkowych automatycznych

### Rozwój przyszły
- Dodanie supportu dla Ethernet
- Integracja z systemami automatyki domowej
- Aplikacja web PWA
- Testy jednostkowe i CI/CD
- Obsługa większej liczby sensorów
