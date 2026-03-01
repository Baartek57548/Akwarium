#pragma once

// Template fallback used when local arduino_secrets.h is missing.
// Create src/arduino_secrets.h with your real credentials to override.

#ifndef SECRET_SSID
#define SECRET_SSID "CHANGEME_STA_SSID"
#endif

#ifndef SECRET_PASS
#define SECRET_PASS "CHANGEME_STA_PASS"
#endif

#ifndef AP_SSID
#define AP_SSID "AkwariumAP_Setup"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "ChangeMe_1234"
#endif

#ifndef SECRET_BLE_PASSKEY
#define SECRET_BLE_PASSKEY 654321
#endif
