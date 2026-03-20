# Komunikacja

## Zakres

System wykorzystuje kilka kanałów komunikacyjnych. Nie wszystkie mają tę samą rangę:

- `WiFi + HTTP + JSON` to kanał operacyjny i serwisowy,
- `BLE GATT + JSON` to główny kanał dla aplikacji MAUI,
- `UART` jest używany głównie do programowania, logów i diagnostyki,
- `OTA` działa osobnymi przepływami przez HTTP i BLE.

## Kanały komunikacyjne

### WiFi / HTTP

Firmware uruchamia `WebServer` i udostępnia:

- statyczny panel WWW,
- endpointy REST,
- `HTTP OTA`,
- synchronizację czasu.

Kluczowe endpointy:

| Metoda | Endpoint | Cel |
| --- | --- | --- |
| `GET` | `/` | panel WWW urządzenia |
| `GET` | `/style.css` | asset UI |
| `GET` | `/script.js` | asset UI |
| `POST` | `/settime?epoch=<unix>` | synchronizacja RTC |
| `POST` | `/update` | wgranie firmware przez HTTP OTA |
| `GET` | `/api/status` | status runtime |
| `GET` | `/api/logs` | logi normalne i krytyczne |
| `POST` | `/api/action` | akcje i zapis konfiguracji |

### BLE

Firmware wystawia jedną usługę GATT i kilka charakterystyk z payloadami JSON.

#### Usługa

- `Service UUID`: `4fafc201-1fb5-459e-8bcc-c5c9c331914b`

#### Charakterystyki

| Nazwa | UUID | Kierunek | Rola |
| --- | --- | --- | --- |
| `status` | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | `READ + NOTIFY` | snapshot statusu runtime |
| `command` | `828917c1-ea55-4d4a-a66e-fd202cea0645` | `WRITE` | komendy sterujące |
| `settings` | `d2912856-de63-11ed-b5ea-0242ac120002` | `READ + WRITE` | konfiguracja |
| `result` | `8e22cb9c-1728-45f9-8c50-2f7252f07379` | `READ + NOTIFY` | `ACK/ERR` |
| `device_info` | `73d4b922-9d7d-4f5a-9f88-0871b07ec21b` | `READ + NOTIFY` | capability i metadata firmware |
| `ota_control` | `b5f6d0d0-0c6a-4cb0-a9b8-6b4e6cb6e550` | `READ + WRITE + NOTIFY` | sterowanie BLE OTA |
| `ota_data` | `f2a4f5f5-89d0-4d3c-a4f7-e1db30c6ff0c` | `WRITE` | strumień danych OTA |

Wszystkie charakterystyki robocze wymagają szyfrowania i bondingu.

### UART

`UART` nie jest w tym repozytorium pełnym business communication protocol. Służy do:

- flashowania firmware,
- monitoringu szeregowego,
- logów diagnostycznych.

Jeżeli w przyszłości pojawi się sterowanie po UART, powinno zostać opisane jako osobna specyfikacja protokołu.

## Format danych

### Legacy wire format

Faktyczny wire format firmware jest zwięzły. Przykład statusu BLE:

```json
{
  "tmp": 24.6,
  "tar": 25.0,
  "thr": 25.5,
  "hm": 0,
  "hys": 0.5,
  "bv": 3.07,
  "bp": 61,
  "l": true,
  "f": true,
  "h": false,
  "srv": 45
}
```

Przykład odpowiedzi `result`:

```json
{
  "t": "ack",
  "c": "settings_saved"
}
```

Przykład `device_info`:

```json
{
  "nm": "Aquarium Controller",
  "ver": "v6.0.0",
  "slot": 1966080,
  "ble": true,
  "http": true,
  "chunk": 160
}
```

### Canonical model

Warstwa `.NET` utrzymuje dodatkowo canonical envelope:

```json
{
  "type": "command | status | event | response",
  "name": "string",
  "payload": {}
}
```

Ten format nie zastępuje jeszcze bezpośrednio payloadów firmware. Jest używany jako docelowy abstraction layer i warstwa kompatybilności.

## Akcje i ustawienia

### HTTP `POST /api/action`

Obsługiwane akcje:

- `feed_now`
- `set_light`
- `set_filter`
- `set_servo`
- `clear_servo`
- `clear_critical_logs`
- `restart_device`
- `factory_reset`
- `save_schedule`

`save_schedule` przyjmuje częściowy patch ustawień. Walidacja odbywa się przez `ConfigValidation`.

### BLE `command`

Obsługiwane komendy są logicznie równoważne:

- `feed_now`
- `set_servo`
- `clear_servo`
- `clear_critical_logs`

### BLE `settings`

Konfiguracja przesyłana jest jako JSON patch na skróconych polach, np.:

- `lm`, `am`, `fm` - tryby harmonogramów,
- `lsH`, `lsM`, `leH`, `leM` - okno dnia,
- `tar`, `hm`, `hys` - temperatura i grzałka,
- `fdH`, `fdM`, `fdF` - karmienie,
- `spO` - `servoPreOffMinutes`.

## Synchronizacja stanów

### Firmware

- `SharedState` utrzymuje spójny snapshot między taskami.
- `ApiHandlers` buduje snapshot HTTP na żądanie.
- `BleManager` publikuje status cyklicznie przez `NOTIFY` po udanej autoryzacji.

### UI

- `MainViewModel` odczytuje status, ustawienia i device info jako osobne payloady.
- `SelectableBluetoothService` dba o to, by model UI był niezależny od konkretnego transportu.

### Emulator

- `EmulatorBluetoothService` emuluje charakterystyki BLE na bazie `EmulatedDeviceCore`.
- `EmulatorHttpApiServer` wystawia HTTP API o zbliżonej strukturze do firmware.

## OTA

### HTTP OTA

- transport przez `POST /update`,
- upload obsługiwany przez `Update.h`,
- `OtaManager` blokuje normalny runtime.

### BLE OTA

Sterowanie:

- `begin` - deklaracja rozmiaru obrazu i metadanych,
- `status` - odczyt bieżącego stanu OTA,
- `abort` - przerwanie sesji,
- `finish` - finalizacja i przełączenie partycji.

Dane firmware są wysyłane paczkami z `4-byte little-endian offset + payload`.

## Design Decisions

- Zachowano kompatybilność legacy JSON, aby nie rozbić istniejącej ścieżki firmware.
- BLE i HTTP współdzielą walidację konfiguracji przez ten sam mechanizm runtime patch.
- UI czyta metadata obrazu `.bin` przed rozpoczęciem OTA, żeby wczesnym etapem odrzucać niekompatybilne paczki.

## Known Limitations

- Legacy payloady i canonical model współistnieją, co zwiększa złożoność warstwy protokołu.
- Firmware nie publikuje jeszcze pełnego strumienia eventów domenowych; część komunikacji ma charakter request/response.
- `UART` nie ma jawnej, produkcyjnej specyfikacji protokołu aplikacyjnego.

## Future Improvements

- Jedna formalna specyfikacja communication protocol generowana z kontraktów shared.
- Testy kontraktowe dla HTTP, BLE i emulatora.
- Rozszerzenie warstwy eventów o streaming logów i zdarzeń asynchronicznych.
