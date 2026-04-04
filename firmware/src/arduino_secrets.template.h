#pragma once

// Template fallback used when local arduino_secrets.h is missing.
// Create src/arduino_secrets.h with your real credentials to override.

#ifndef SECRET_SSID
#define SECRET_SSID "JakasNazwa"
#endif

#ifndef SECRET_PASS
#define SECRET_PASS "12345678"
#endif

#ifndef AP_SSID
#define AP_SSID "AkwariumAP"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "12345678"
#endif
