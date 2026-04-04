# Manual Smoke Test

## Cel

Checklista sluzy do szybkiej walidacji najwazniejszych sciezek po zmianach w firmware, panelu WWW lub OTA.

## Preconditions

- firmware zostalo zbudowane i wgrane
- czas urzadzenia jest poprawny
- `firmware/src/arduino_secrets.h` zawiera poprawne dane `STA/AP`
- przed testem nie trwa aktywna sesja OTA

## 1. Panel WWW i status

1. Otworz ekran `Wifi` na OLED.
2. Uruchom sesje WiFi.
3. Polacz sie z urzadzeniem przez `STA` albo `AP`.
4. Otworz `http://<ip_urzadzenia>/`.
5. Zweryfikuj ladowanie dashboardu i pobranie statusu.

Oczekiwany wynik:

- panel WWW odpowiada
- status urzadzenia laduje sie poprawnie
- logi sa widoczne w panelu

## 2. `AP` i timeout bez klientow

1. Uruchom `AP` z menu `Wifi`.
2. Nie lacz zadnego klienta przez co najmniej `90 s`.
3. Obserwuj OLED i logi.

Oczekiwany wynik:

- sesja `AP` zamyka sie automatycznie po `90 s`
- firmware wraca do `HOME`
- w logach pojawia sie wpis o automatycznym zamknieciu `AP`

## 3. SSE i odswiezanie statusu

1. Pozostaw otwarty panel WWW.
2. Wywolaj akcje z panelu, ktore zmieniaja stan urzadzenia lub logi.
3. Zweryfikuj, czy interfejs odswieza status bez recznego reloadu strony.

Oczekiwany wynik:

- przegladarka utrzymuje polaczenie z `GET /api/events`
- zmiany statusu i logow pojawiaja sie na zywo

## 4. Zapis konfiguracji przez HTTP

1. Przez panel WWW ustaw poprawne i niepoprawne wartosci harmonogramow.
2. Zweryfikuj odpowiedz `OK` albo `OK_PARTIAL`.
3. Zrestartuj urzadzenie.
4. Sprawdz persystencje ustawien po restarcie.

Oczekiwany wynik:

- poprawne pola sa zapisywane
- niepoprawne pola nie psuja calego payloadu
- konfiguracja przetrwa restart

## 5. Tryb nocny i light sleep

1. Ustaw czas urzadzenia poza oknem dnia.
2. Upewnij sie, ze `AP` jest wylaczony.
3. Upewnij sie, ze nie trwa OTA ani karmienie.
4. Odczekaj powyzej progu bezczynnosci.

Oczekiwany wynik:

- firmware przechodzi do light sleep tylko po spelnieniu wszystkich gate'ow
- brak wejscia w sleep podczas aktywnego `AP`, OTA lub karmienia
- wybudzenie dziala przez przycisk albo timer

## 6. HTTP OTA

1. Polacz sie z panelem WWW.
2. Wyslij obraz firmware przez `POST /update`.
3. Obserwuj przebieg i restart.

Oczekiwany wynik:

- firmware wchodzi w stan OTA
- wyjscia przechodza do bezpiecznego stanu
- po zakonczeniu nastepuje restart

## 7. Karmnik i serwo

1. Uruchom reczne karmienie.
2. Sprawdz timeout bezpieczenstwa i poprawne zatrzymanie napedu.
3. Wymus reczne ustawienie serwa.
4. Wyczysc override serwa.

Oczekiwany wynik:

- karmnik nie zostaje w stanie aktywnym
- czujnik karmnika zatrzymuje cykl zgodnie z oczekiwaniem
- reczne sterowanie serwem dziala i mozna je wyczyscic
