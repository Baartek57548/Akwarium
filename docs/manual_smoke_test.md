# Manual Smoke Test Checklist

## Preconditions
- Firmware built and flashed.
- Device time is synchronized (`/settime` or RTC menu).
- `src/arduino_secrets.h` contains real WiFi/AP/BLE secrets.
- No active OTA upload at test start.

## 1) AP start/stop + Web UI access
1. On device UI open `Wifi` menu entry.
2. Confirm AP is started and visible.
3. Connect from phone/laptop to AP SSID from local secrets.
4. Open `http://<AP_IP>/` and verify dashboard loads.
5. Disconnect client and verify AP auto-exits (or stop with `UP`).

Expected:
- AP starts and serves UI.
- Client count is visible.
- AP exits correctly and device returns to menu/home.

## 2) BLE pairing + reconnect
1. Open `Bluetooth` menu entry.
2. Pair with BLE app using PIN from local secrets (`SECRET_BLE_PASSKEY`).
3. Read status characteristic.
4. Disconnect and reconnect once.

Expected:
- Pairing succeeds with encrypted/bonded session.
- Status read/notify works.
- Reconnect works without firmware reset.

## 3) Config save via HTTP and BLE + persistence
1. Via Web UI/HTTP set schedule/temperature fields including out-of-range sample values.
2. Verify HTTP reply is `OK` or `OK_PARTIAL` for mixed-valid payload.
3. Via BLE `settings` characteristic send mixed payload (valid + invalid fields).
4. Verify BLE result code: `settings_saved` or `settings_partial`.
5. Reboot device and confirm persisted values in API/BLE status.

Expected:
- All config channels use shared clamp/partial rules.
- Valid fields are saved.
- Invalid fields do not block valid ones.
- Values persist after reboot.

## 4) Night deep sleep gate (OTA/AP/STA/BLE/idle)
1. Set RTC time to night interval (outside day schedule).
2. Keep device active and verify no deep sleep before idle timeout.
3. Ensure AP is OFF.
4. Ensure BLE advertising/client is OFF.
5. Ensure OTA not in progress.
6. Wait for idle timeout (>5 min).

Expected:
- Device requests STA power-off before sleep.
- Deep sleep starts only when all gates pass:
  - OTA OFF
  - AP OFF
  - STA OFF
  - BLE OFF
  - idle > 5 min

## 5) OTA at night (sleep blocked during OTA)
1. Keep night conditions.
2. Start OTA upload (`POST /update`).
3. Observe logs during upload and finalize OTA.

Expected:
- No deep sleep entry during OTA upload.
- OTA completes and device restarts normally.
