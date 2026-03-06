#pragma once

// Template fallback used when local arduino_secrets.h is missing.
// Create src/arduino_secrets.h with your real credentials to override.

#ifndef SECRET_SSID
#define SECRET_SSID "CHANGEME_STA_SSID"  //Nazwa sieci Wi-Fi, do której ma się połączyć urządzenie
#endif

#ifndef SECRET_PASS
#define SECRET_PASS "CHANGEME_STA_PASS" //Hasło do sieci Wi-Fi, musi mieć co najmniej 8 znaków dla WPA2
#endif

#ifndef AP_SSID
#define AP_SSID "AkwariumAP"   //Nazwa sieci Wi-Fi, którą urządzenie utworzy, gdy nie będzie mogło połączyć się z docelową siecią. Użytkownicy mogą się do niej połączyć, aby skonfigurować urządzenie.
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "12345678"      //Hasło do sieci Wi-Fi utworzonej przez urządzenie, musi mieć co najmniej 8 znaków dla WPA2
#endif

#ifndef SECRET_BLE_PASSKEY
#define SECRET_BLE_PASSKEY 654321   //Sześciocyfrowy kod PIN używany do parowania urządzenia przez Bluetooth Low Energy (BLE). Użytkownicy będą musieli wprowadzić ten kod, aby połączyć się z urządzeniem za pomocą BLE.
#endif
