# AKWARIUM-1.2.1

Sterownik akwarium oparty o ESP32-S3. Uklad automatyzuje oswietlenie, filtracje, napowietrzanie, dogrzewanie i karmienie, a jednoczesnie udostepnia lokalna obsluge (OLED + przyciski), panel WWW i BLE.

Pelna specyfikacja inzynierska jest w `Info.md`. Ten plik to opis urzadzenia i sposobu uzywania.

## 1. Co to jest to urzadzenie

Jest to centralny kontroler akwarium, ktory:

- wlacza/wylacza swiatlo wg harmonogramu dnia
- steruje pompka filtracyjna wg harmonogramu filtra
- steruje grzalka na podstawie DS18B20 (setpoint + histereza)
- steruje serwem napowietrzania wg harmonogramu i warunkow temperatury
- uruchamia karmnik automatycznie i recznie
- pokazuje status na ekranie OLED
- udostepnia konfiguracje i podglad stanu przez WWW i BLE

## 2. Najwazniejsze funkcje

- harmonogramy:
  - dzien (swiatlo)
  - filtr (pompka)
  - aeracja (serwo)
  - karmienie
- lokalna obsluga przez 3 przyciski
- REST API (`/api/status`, `/api/logs`, `/api/action`)
- OTA przez `/update`
- BLE GATT z szyfrowaniem (MITM + bond + PIN)
- logi i diagnostyka resetow zapisane trwale w NVS
- RTC DS3231 z czasem UTC i strefa Europe/Warsaw po stronie logiki

## 3. Jak urzadzenie dziala na co dzien

## 3.1 Cykl dzienny

W kazdej iteracji firmware:

1. odczytuje sensory (temperatura, bateria podtrzymania RTC)
2. liczy, czy trwa dzien i czy aktywne sa okna harmonogramow
3. wyznacza docelowe stany wyjsc
4. zapisuje te stany na piny

## 3.2 Logika wykonawcza (wyjscia)

Aktualnie wszystkie 3 wyjscia glowne maja ten sam model pinowy:

- `ON => LOW`
- `OFF => HIGH`

Dotyczy:

- swiatlo (`LIGHT_PIN`)
- pompka (`PUMP_PIN`)
- grzalka (`HEATER_PIN`)

Tryb AUTO:

- swiatlo = stan dnia
- pompka = okno filtra (z wymuszeniem OFF w nocy)
- grzalka = tylko w dzien i tylko przy poprawnym odczycie temperatury

Tryb TESTS (menu testowe):

- recznie nadpisuje stany swiatla, grzalki i pompki
- po wyjsciu z TESTS wraca do automatyki

## 3.3 Grzalka i bezpieczenstwo

- setpoint i histereza sa konfigurowalne
- minimalny odstep miedzy przelaczeniami: 120 s
- hard cut-off: przy `>= 28.0C` grzalka OFF
- w nocy i przy awarii czujnika grzalka jest wymuszana na OFF

## 3.4 Karmienie

Karmnik mozna uruchomic:

- z harmonogramu
- z web API
- z BLE
- z lokalnego skrotu (3 przyciski przytrzymane)

Sterownik karmnika ma timeout bezpieczenstwa i logike stopu po cyklu czujnika.

## 4. Interfejsy uzytkownika

## 4.1 OLED + przyciski

Urzadzenie ma lokalne menu:

- HOME (status)
- MENU
- harmonogramy (light/aeration/filter/temp/feeding)
- logi
- ustawianie daty/czasu
- testy
- AP mode
- Bluetooth mode
- ekran karmienia

Uwaga praktyczna:

- po wygaszeniu OLED pierwsze nacisniecie tylko wybudza ekran

## 4.2 Panel WWW

Po wejsciu na IP sterownika dostepny jest panel:

- status temperatury, baterii, przelacznikow i diagnostyki
- edycja harmonogramow
- reczne akcje (`feed_now`, serwo)
- podglad logow
- OTA firmware (`/update`)

## 4.3 BLE

- nazwa urzadzenia: `Akwarium_BLE`
- parowanie: statyczny PIN (`SECRET_BLE_PASSKEY`)
- status i ustawienia dostepne przez characteristic GATT

## 5. Komunikacja i API

## 5.1 Endpoints

- `GET /` - panel web
- `POST /settime?epoch=<unix_utc>` - ustawienie czasu
- `POST /update` - OTA
- `GET /api/status` - status runtime
- `GET /api/logs` - logi
- `POST /api/action` - akcje i zapis harmonogramu

## 5.2 `/api/action` (najwazniejsze akcje)

- `action=feed_now`
- `action=set_servo&angle=<0..90>`
- `action=clear_servo`
- `action=clear_critical_logs`
- `action=save_schedule` + pola harmonogramow/temperatury

## 6. Sprzet i podlaczenie

## 6.1 Piny

```text
BUTTON_UP_PIN      GPIO15
BUTTON_SELECT_PIN  GPIO16
BUTTON_DOWN_PIN    GPIO14

ONE_WIRE_BUS       GPIO1   (DS18B20)
HEATER_PIN         GPIO3
PUMP_PIN           GPIO4
FEEDER_PIN         GPIO5
SERVO_PIN          GPIO6

BAT_ADC_PIN        GPIO7
BAT_EN_PIN         GPIO10
FEEDER_SENSOR_PIN  GPIO12
LIGHT_PIN          GPIO17

I2C SDA            GPIO8
I2C SCL            GPIO9
```

## 6.2 Uwaga o przekaznikach

Firmware zaklada, ze wyjscia sa aktywne stanem `LOW` (przekazniki active-low).
Jesli modul wykonawczy jest aktywny stanem `HIGH`, nalezy dostosowac warstwe sprzetowa albo logike mapowania poziomow.

## 7. Szybki start

## 7.1 Konfiguracja sekretow

1. Skopiuj `src/arduino_secrets.template.h` do `src/arduino_secrets.h`.
2. Ustaw:
   - `SECRET_SSID`
   - `SECRET_PASS`
   - `AP_SSID`
   - `AP_PASSWORD`
   - `SECRET_BLE_PASSKEY`

## 7.2 Build i upload

```bash
python -m platformio run
python -m platformio run -t upload
python -m platformio device monitor -b 115200
```

## 7.3 Pierwsze uruchomienie

1. Sprawdz logi UART po starcie.
2. Ustaw czas przez panel WWW (`/settime`), jesli RTC nie ma poprawnej daty.
3. Zweryfikuj harmonogramy i temperatury w panelu.
4. Sprawdz testowo `feed_now`.

## 8. Aktualny stan power management

Aktualnie deep sleep runtime jest wylaczony.
Aktywne jest tylko wygaszanie OLED po bezczynnosci (4 min, o ile `alwaysScreenOn=false`).

Kod deep sleep istnieje, ale nie jest obecnie wlaczony w glownej sciezce pracy.

## 9. Diagnostyka i logi

Urzadzenie zapisuje:

- logi biezace (RAM)
- logi krytyczne trwale (NVS)
- licznik bootow i przyczyny resetow (brownout/wdt/panic)

W panelu WWW i API mozna szybko sprawdzic:

- `diag.bootCount`
- `diag.lastResetReason`
- `diag.lastWakeupCause`
- `diag.brownoutCount`
- `diag.wdtCount`
- `diag.panicCount`

## 10. Ograniczenia i uwagi praktyczne

- serwo i silnik karmnika moga generowac piki pradowe (dobry zapas zasilania jest wymagany)
- brak poprawnej daty/czasu pogorszy dzialanie harmonogramow
- przy OTA logika runtime jest wstrzymana do konca aktualizacji/restartu

## 11. Struktura projektu

- `src/src.ino` - setup/loop i UI state machine
- `src/SystemController.*` - logika runtime i I/O
- `src/AkwariumWifi.*` - WiFi/AP/HTTP/OTA
- `src/BleManager.*` - BLE
- `src/ApiHandlers.*` - REST API
- `src/Config*.{h,cpp}` - konfiguracja, walidacja, NVS
- `src/TimeUtils.*` - czas i strefa
- `src/SystemDiagnostics.*` - diagnostyka resetow
- `src/LogManager.*` - logi
- `Info.md` - pelna dokumentacja techniczna
