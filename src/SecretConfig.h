#pragma once

#if defined(__has_include)
#if __has_include("arduino_secrets.h")
#include "arduino_secrets.h"
#endif
#endif

#include "arduino_secrets.template.h"

#ifndef SECRET_SSID
#error "Missing SECRET_SSID. Define it in arduino_secrets.h."
#endif

#ifndef SECRET_PASS
#error "Missing SECRET_PASS. Define it in arduino_secrets.h."
#endif

#ifndef AP_SSID
#error "Missing AP_SSID. Define it in arduino_secrets.h."
#endif

#ifndef AP_PASSWORD
#error "Missing AP_PASSWORD. Define it in arduino_secrets.h."
#endif

#ifndef SECRET_BLE_PASSKEY
#error "Missing SECRET_BLE_PASSKEY. Define it in arduino_secrets.h."
#endif
