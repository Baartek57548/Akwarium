# Manual Smoke Test

## Cel

Checklista służy do szybkiej walidacji najbardziej ryzykownych ścieżek po zmianach w firmware, komunikacji lub OTA.

## Preconditions

- firmware zostało zbudowane i wgrane,
- czas urządzenia jest poprawny,
- `firmware/src/arduino_secrets.h` zawiera prawidłowe dane sieciowe i `BLE passkey`,
- brak aktywnej sesji OTA przed startem testów.

## 1. Access Point i panel WWW

1. Otwórz ekran `Wifi` na OLED.
2. Uruchom `AP`.
3. Połącz laptop lub telefon z SSID urządzenia.
4. Otwórz `http://<ip_urzadzenia>/`.
5. Zweryfikuj ładowanie dashboardu i odczyt statusu.
6. Zakończ sesję i sprawdź poprawne wyjście z trybu `AP`.

Oczekiwany wynik:

- `AP` startuje poprawnie,
- panel WWW odpowiada,
- liczba klientów jest raportowana,
- wyjście z trybu `AP` nie blokuje powrotu do normalnego runtime.

## 2. BLE pairing i reconnect

1. Otwórz ekran `Bluetooth` na OLED.
2. Sparuj urządzenie przy użyciu `SECRET_BLE_PASSKEY`.
3. Odczytaj status przez aplikację MAUI lub narzędzie BLE.
4. Rozłącz i połącz ponownie.

Oczekiwany wynik:

- pairing kończy się szyfrowanym połączeniem,
- odczyt statusu działa,
- reconnect nie wymaga restartu firmware.

## 3. Zapis konfiguracji przez HTTP i BLE

1. Przez panel WWW ustaw poprawne i niepoprawne wartości harmonogramów.
2. Zweryfikuj odpowiedź `OK` albo `OK_PARTIAL`.
3. Przez BLE wyślij analogiczny patch ustawień.
4. Zweryfikuj kody `settings_saved` albo `settings_partial`.
5. Zrestartuj urządzenie.
6. Sprawdź persystencję ustawień po restarcie.

Oczekiwany wynik:

- walidacja działa spójnie dla HTTP i BLE,
- poprawne pola są zapisywane,
- błędne pola nie psują całego payloadu,
- konfiguracja przetrwa restart.

## 4. Tryb nocny i light sleep

1. Ustaw czas urządzenia poza oknem dnia.
2. Upewnij się, że `AP` jest wyłączony.
3. Upewnij się, że BLE nie reklamuje i nie ma aktywnego klienta.
4. Upewnij się, że nie trwa OTA.
5. Odczekaj powyżej progu bezczynności.

Oczekiwany wynik:

- firmware przechodzi do light sleep tylko po spełnieniu wszystkich gate'ów,
- brak wejścia w sleep podczas aktywnego `AP`, BLE, OTA lub karmienia,
- wybudzenie działa przez przycisk albo timer.

## 5. OTA

### HTTP OTA

1. Połącz się z panelem WWW.
2. Wyślij obraz firmware przez `POST /update`.
3. Obserwuj przebieg i restart.

### BLE OTA

1. Połącz aplikację MAUI z urządzeniem.
2. Wybierz poprawny obraz `.bin`.
3. Uruchom `BLE OTA`.
4. Poczekaj na zakończenie transferu i restart.

Oczekiwany wynik:

- firmware wchodzi w stan OTA,
- wyjścia przechodzą do bezpiecznego stanu,
- po zakończeniu następuje restart i przełączenie obrazu.

## 6. Karmnik i serwo

1. Uruchom ręczne karmienie.
2. Sprawdź timeout bezpieczeństwa i poprawne zatrzymanie napędu.
3. Wymuś ręczne ustawienie serwa.
4. Wyczyść override serwa.

Oczekiwany wynik:

- karmnik nie zostaje w stanie aktywnym,
- czujnik karmnika zatrzymuje cykl zgodnie z oczekiwaniem,
- ręczne sterowanie serwem działa i można je wyczyścić.
