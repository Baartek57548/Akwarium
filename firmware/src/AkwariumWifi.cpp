#include "AkwariumWifi.h"

#include "ConfigManager.h"
#include "LogManager.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SharedState.h"
#include "SystemController.h"
#include "WebAssets.h"

#include <DNSServer.h>
#include <RTClib.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <uri/UriRegex.h>

// ==========================================
// KONFIGURACJA SIECI
// ==========================================
static const int WIFI_TIMEOUT = 6000;

static const char *NTP_TZ = "CET-1CEST,M3.5.0/2,M10.5.0/3";
static const char *NTP_SERVER_1 = "pool.ntp.org";
static const char *NTP_SERVER_2 = "time.google.com";
static const char *NTP_SERVER_3 = "time.windows.com";
static const uint8_t DAILY_SYNC_HOUR = 9;
static const uint8_t DAILY_SYNC_WINDOW_MINUTES = 5;
static const unsigned long NTP_SYNC_TIMEOUT_MS = 8000UL;
static const unsigned long WIFI_MODE_SWITCH_DELAY_MS = 25UL;

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

static bool isAPMode = false;
static bool otaUploadActive = false;
static bool otaUploadRejected = false;
static String otaUploadRejectReason;

static volatile bool serviceModeStartRequested = false;
static volatile bool serviceModeStopRequested = false;
static volatile bool staOffRequested = false;
static volatile bool staOnRequested = false;

static volatile bool staIsOff = true;
static bool staConnecting = false;
static bool serviceModeActive = false;
static bool timeSyncInProgress = false;
static bool serviceStaWasConnected = false;
static bool staConnected = false;
static uint32_t staLastConnectedEpoch = 0;
static String staLastConnectedSsid;
static uint32_t lastTimeSyncEpoch = 0;
static bool lastTimeSyncSuccessful = false;
static String lastTimeSyncStatus = "Brak proby synchronizacji czasu.";
static int lastDailySyncAttemptDateStamp = -1;
static bool webServerConfigured = false;
static bool webServerRunning = false;

WebServer &AkwariumWifi::getServer() { return server; }

static void startAPInternal(const char *reason = nullptr);
static void stopServiceModeInternal(const char *reason);
static void startWebServerIfNeeded(const char *reason = nullptr);
static void stopWebServerIfNeeded();
static void processWifiRequests();
static void maintainWifiState();
static void maybeRunDailyTimeSync();
static void sendCaptiveRedirect();
static void loadConfiguredStaCredentials(char *ssidOut, size_t ssidOutSize,
                                         char *passwordOut,
                                         size_t passwordOutSize);
static void loadConfiguredApCredentials(char *ssidOut, size_t ssidOutSize,
                                        char *passwordOut,
                                        size_t passwordOutSize);
static String getConfiguredStaSsidString();
static String getConfiguredApSsidString();
static String getConfiguredApPasswordString();

static void logWifiInfo(const char *format, ...) {
  char buffer[192];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  LogManager::logInfo(buffer);
}

static void logWifiWarn(const char *format, ...) {
  char buffer[192];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  LogManager::logWarn(buffer);
}

static void loadConfiguredStaCredentials(char *ssidOut, size_t ssidOutSize,
                                         char *passwordOut,
                                         size_t passwordOutSize) {
  const Config cfg = ConfigManager::getCopy();
  if (ssidOut != nullptr && ssidOutSize > 0) {
    snprintf(ssidOut, ssidOutSize, "%s", cfg.staSsid);
  }
  if (passwordOut != nullptr && passwordOutSize > 0) {
    snprintf(passwordOut, passwordOutSize, "%s", cfg.staPassword);
  }
}

static void loadConfiguredApCredentials(char *ssidOut, size_t ssidOutSize,
                                        char *passwordOut,
                                        size_t passwordOutSize) {
  const Config cfg = ConfigManager::getCopy();
  if (ssidOut != nullptr && ssidOutSize > 0) {
    snprintf(ssidOut, ssidOutSize, "%s", cfg.apSsid);
  }
  if (passwordOut != nullptr && passwordOutSize > 0) {
    snprintf(passwordOut, passwordOutSize, "%s", cfg.apPassword);
  }
}

static String getConfiguredStaSsidString() {
  const Config cfg = ConfigManager::getCopy();
  return String(cfg.staSsid);
}

static String getConfiguredApSsidString() {
  const Config cfg = ConfigManager::getCopy();
  return String(cfg.apSsid);
}

static String getConfiguredApPasswordString() {
  const Config cfg = ConfigManager::getCopy();
  return String(cfg.apPassword);
}

static bool hasReasonableDateTime(const DateTime &dt) {
  return dt.year() >= 2024 && dt.year() <= 2099;
}

static bool getControllerClockNow(DateTime &out) {
  const SharedStateData snap = SharedState::getSnapshot();
  const DateTime now(snap.year, snap.month, snap.day, snap.hour, snap.minute,
                     snap.second);
  if (!hasReasonableDateTime(now)) {
    return false;
  }

  out = now;
  return true;
}

static int makeDateStamp(const DateTime &dt) {
  return (dt.year() * 10000) + (dt.month() * 100) + dt.day();
}

static void formatDateTime(const DateTime &dt, char *buffer,
                           size_t bufferSize) {
  snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", dt.year(),
           dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
}

static const char *wifiStatusToString(wl_status_t status) {
  switch (status) {
  case WL_IDLE_STATUS:
    return "idle";
  case WL_NO_SSID_AVAIL:
    return "ssid_not_found";
  case WL_SCAN_COMPLETED:
    return "scan_completed";
  case WL_CONNECTED:
    return "connected";
  case WL_CONNECT_FAILED:
    return "connect_failed";
  case WL_CONNECTION_LOST:
    return "connection_lost";
  case WL_DISCONNECTED:
    return "disconnected";
  default:
    return "unknown";
  }
}

static uint32_t getBestEffortEpoch() {
  const time_t now = time(nullptr);
  if (now > 1700000000) {
    return static_cast<uint32_t>(now);
  }

  DateTime controllerNow;
  if (getControllerClockNow(controllerNow)) {
    return static_cast<uint32_t>(controllerNow.unixtime());
  }

  return 0;
}

static void setTimeSyncStatus(bool ok, uint32_t epoch, const char *status) {
  lastTimeSyncSuccessful = ok;
  if (epoch > 0) {
    lastTimeSyncEpoch = epoch;
  }
  lastTimeSyncStatus = status != nullptr ? status : "";
}

static void syncStaStatus() {
  const bool connected = !staIsOff && WiFi.status() == WL_CONNECTED;
  if (connected) {
    String ssid = WiFi.SSID();
    if (ssid.length() == 0) {
      ssid = getConfiguredStaSsidString();
    }

    if (!staConnected || staLastConnectedSsid != ssid) {
      const uint32_t currentEpoch = getBestEffortEpoch();
      if (currentEpoch > 0) {
        staLastConnectedEpoch = currentEpoch;
      }
    }

    staLastConnectedSsid = ssid;
  } else if (staLastConnectedSsid.length() == 0) {
    staLastConnectedSsid = getConfiguredStaSsidString();
  }

  staConnected = connected;
}

static void turnWifiOffInternal(const char *reason) {
  dnsServer.stop();
  stopWebServerIfNeeded();
  if (isAPMode) {
    WiFi.softAPdisconnect(true);
  }
  WiFi.disconnect(false, false);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_OFF);
  delay(WIFI_MODE_SWITCH_DELAY_MS);

  isAPMode = false;
  staIsOff = true;
  staConnecting = false;
  staConnected = false;
  if (reason != nullptr && reason[0] != '\0') {
    logWifiInfo("[WIFI] Radio wylaczone: %s", reason);
  }
}

static void applySynchronizedTime(uint32_t epoch, const char *source) {
  DateTime oldTime(2000, 1, 1, 0, 0, 0);
  const bool hadOldTime = getControllerClockNow(oldTime);

  struct timeval tv;
  tv.tv_sec = epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  syncSystemTime(epoch);

  const DateTime newTime(epoch);
  char timeBuf[32];
  formatDateTime(newTime, timeBuf, sizeof(timeBuf));

  char statusBuf[96];
  snprintf(statusBuf, sizeof(statusBuf), "%s: OK (%s)", source, timeBuf);
  setTimeSyncStatus(true, epoch, statusBuf);

  if (hadOldTime) {
    const long diffSec =
        static_cast<long>(epoch) - static_cast<long>(oldTime.unixtime());
    logWifiInfo("[TIME] %s: ustawiono %s (delta %+lds).", source, timeBuf,
                diffSec);
  } else {
    logWifiInfo("[TIME] %s: ustawiono %s.", source, timeBuf);
  }
}

static bool syncTimeFromNtp(const char *source) {
  timeSyncInProgress = true;
  logWifiInfo("[TIME] %s: start synchronizacji NTP (%s, %s, %s).", source,
              NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  configTzTime(NTP_TZ, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  struct tm timeinfo = {};
  bool gotTime = false;
  const unsigned long startedAtMs = millis();
  while (millis() - startedAtMs < NTP_SYNC_TIMEOUT_MS) {
    if (getLocalTime(&timeinfo, 1000)) {
      gotTime = true;
      break;
    }
    delay(150);
  }

  timeSyncInProgress = false;

  if (!gotTime) {
    setTimeSyncStatus(false, 0, "NTP: timeout odpowiedzi serwera.");
    logWifiWarn("[TIME] %s: nie udalo sie pobrac czasu z NTP (timeout %lus).",
                source, NTP_SYNC_TIMEOUT_MS / 1000UL);
    return false;
  }

  time_t epoch = 0;
  time(&epoch);
  if (epoch < 1700000000) {
    setTimeSyncStatus(false, 0, "NTP: odebrano nierozsadny epoch.");
    logWifiWarn("[TIME] %s: odebrano nierozsadny epoch z NTP (%ld).", source,
                static_cast<long>(epoch));
    return false;
  }

  applySynchronizedTime(static_cast<uint32_t>(epoch), source);
  return true;
}

static bool connectStaWithTimeout(const char *context) {
  char staSsid[WIFI_SSID_MAX_LENGTH + 1] = {};
  char staPassword[WIFI_PASSWORD_MAX_LENGTH + 1] = {};
  loadConfiguredStaCredentials(staSsid, sizeof(staSsid), staPassword,
                               sizeof(staPassword));

  const size_t ssidLen = strlen(staSsid);
  const size_t passLen = strlen(staPassword);
  if (ssidLen == 0 || ssidLen > WIFI_SSID_MAX_LENGTH) {
    logWifiWarn(
        "[WIFI-STA] %s: brak poprawnego SSID STA w konfiguracji (len=%u).",
        context, static_cast<unsigned>(ssidLen));
    staConnecting = false;
    staIsOff = true;
    return false;
  }
  if (passLen > WIFI_PASSWORD_MAX_LENGTH || (passLen > 0 && passLen < 8)) {
    logWifiWarn(
        "[WIFI-STA] %s: haslo STA ma niepoprawna dlugosc (len=%u).",
        context, static_cast<unsigned>(passLen));
    staConnecting = false;
    staIsOff = true;
    return false;
  }

  staConnecting = true;
  staIsOff = false;
  isAPMode = false;
  dnsServer.stop();

  WiFi.disconnect(false, false);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_STA);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  if (passLen > 0) {
    WiFi.begin(staSsid, staPassword);
  } else {
    WiFi.begin(staSsid);
  }

  logWifiInfo(
      "[WIFI-STA] %s: proba polaczenia z SSID '%s' (timeout %ds).", context,
      staSsid, WIFI_TIMEOUT / 1000);

  const unsigned long startedAtMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startedAtMs) < static_cast<unsigned long>(WIFI_TIMEOUT)) {
    delay(100);
  }

  const wl_status_t status = WiFi.status();
  staConnecting = false;

  if (status == WL_CONNECTED) {
    syncStaStatus();
    serviceStaWasConnected = serviceModeActive;
    const String ssid =
        WiFi.SSID().length() > 0 ? WiFi.SSID() : String(staSsid);
    startWebServerIfNeeded("STA aktywne");
    logWifiInfo("[WIFI-STA] %s: polaczono z '%s', IP=%s.", context,
                ssid.c_str(), WiFi.localIP().toString().c_str());
    return true;
  }

  syncStaStatus();
  logWifiWarn("[WIFI-STA] %s: brak polaczenia po %ds, status=%s (%d).", context,
              WIFI_TIMEOUT / 1000, wifiStatusToString(status),
              static_cast<int>(status));

  WiFi.disconnect(false, false);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_OFF);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  staIsOff = true;
  staConnected = false;
  return false;
}

static void startAPInternal(const char *reason) {
  if (isAPMode) {
    return;
  }

  char apSsid[WIFI_SSID_MAX_LENGTH + 1] = {};
  char apPassword[WIFI_PASSWORD_MAX_LENGTH + 1] = {};
  loadConfiguredApCredentials(apSsid, sizeof(apSsid), apPassword,
                              sizeof(apPassword));

  const size_t ssidLen = strlen(apSsid);
  const size_t passLen = strlen(apPassword);
  if (ssidLen == 0 || ssidLen > 31) {
    logWifiWarn("[WIFI-AP] BLAD: AP_SSID musi miec dlugosc 1..31 znakow.");
    return;
  }
  if (passLen < 8 || passLen > 63) {
    logWifiWarn("[WIFI-AP] BLAD: AP_PASSWORD musi miec dlugosc 8..63 znakow.");
    return;
  }

  WiFi.disconnect(false, false);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_OFF);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_AP);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  staIsOff = true;

  bool apStarted = WiFi.softAP(apSsid, apPassword);
  if (!apStarted) {
    WiFi.mode(WIFI_OFF);
    delay(WIFI_MODE_SWITCH_DELAY_MS);
    WiFi.mode(WIFI_AP);
    delay(WIFI_MODE_SWITCH_DELAY_MS);
    apStarted = WiFi.softAP(apSsid, apPassword);
  }

  if (!apStarted) {
    logWifiWarn("[WIFI-AP] BLAD: nie udalo sie uruchomic AP.");
    isAPMode = false;
    staIsOff = true;
    return;
  }

  wifi_config_t apCfg = {};
  const esp_err_t cfgErr = esp_wifi_get_config(WIFI_IF_AP, &apCfg);
  if (cfgErr == ESP_OK && apCfg.ap.authmode == WIFI_AUTH_OPEN) {
    logWifiWarn(
        "[WIFI-AP] BLAD: AP uruchomil sie jako OPEN mimo hasla. Wylaczam AP.");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    isAPMode = false;
    staIsOff = true;
    return;
  }
  if (cfgErr != ESP_OK) {
    logWifiWarn("[WIFI-AP] Ostrzezenie: esp_wifi_get_config(AP) failed: %s.",
                esp_err_to_name(cfgErr));
  }

  dnsServer.stop();
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  isAPMode = true;
  startWebServerIfNeeded("AP aktywne");

  if (reason != nullptr && reason[0] != '\0') {
    logWifiInfo("[WIFI-AP] Uruchomiono AP po zdarzeniu: %s", reason);
  }
  logWifiInfo("[WIFI-AP] AP aktywne. SSID='%s', IP=%s.", apSsid,
              WiFi.softAPIP().toString().c_str());
}

static void startServiceModeInternal() {
  serviceModeActive = true;
  serviceStaWasConnected = false;

  logWifiInfo("[WIFI] Menu WiFi: najpierw proba STA przez %ds, potem fallback do AP.",
              WIFI_TIMEOUT / 1000);

  if (connectStaWithTimeout("Menu WiFi")) {
    if (!syncTimeFromNtp("Menu WiFi")) {
      logWifiWarn(
          "[TIME] Menu WiFi: pozostawiam polaczenie STA, ale synchronizacja NTP sie nie udala.");
    }
    return;
  }

  startAPInternal("Brak polaczenia STA z menu WiFi");
}

static void stopServiceModeInternal(const char *reason) {
  serviceModeActive = false;
  serviceStaWasConnected = false;
  turnWifiOffInternal(reason);
}

static void runDailyTimeSync(const DateTime &now) {
  const int dateStamp = makeDateStamp(now);
  lastDailySyncAttemptDateStamp = dateStamp;

  char nowBuf[32];
  formatDateTime(now, nowBuf, sizeof(nowBuf));
  logWifiInfo(
      "[TIME] Harmonogram 09:00: start dziennej proby Wi-Fi + NTP (RTC=%s).",
      nowBuf);

  if (!connectStaWithTimeout("Harmonogram 09:00")) {
    setTimeSyncStatus(false, 0,
                      "09:00: brak polaczenia Wi-Fi, kolejna proba jutro.");
    logWifiWarn(
        "[TIME] Harmonogram 09:00: brak polaczenia Wi-Fi, kolejna proba jutro.");
    turnWifiOffInternal("Zakonczenie nieudanej proby 09:00.");
    return;
  }

  if (syncTimeFromNtp("Harmonogram 09:00")) {
    logWifiInfo("[TIME] Harmonogram 09:00: synchronizacja zakonczona powodzeniem.");
  } else {
    setTimeSyncStatus(false, 0,
                      "09:00: brak odpowiedzi NTP, kolejna proba jutro.");
    logWifiWarn(
        "[TIME] Harmonogram 09:00: brak odpowiedzi NTP, kolejna proba jutro.");
  }

  turnWifiOffInternal("Zakonczono dzienna synchronizacje 09:00.");
}

static void maybeRunDailyTimeSync() {
  if (serviceModeActive || isAPMode || staConnecting || timeSyncInProgress ||
      OtaManager::isOtaInProgress()) {
    return;
  }

  DateTime now(2000, 1, 1, 0, 0, 0);
  if (!getControllerClockNow(now)) {
    return;
  }

  const int dateStamp = makeDateStamp(now);
  if (dateStamp == lastDailySyncAttemptDateStamp) {
    return;
  }

  if (now.hour() == DAILY_SYNC_HOUR && now.minute() < DAILY_SYNC_WINDOW_MINUTES) {
    runDailyTimeSync(now);
  }
}

static void maintainWifiState() {
  syncStaStatus();

  if (serviceModeActive && !isAPMode && !staConnecting &&
      WiFi.status() != WL_CONNECTED && serviceStaWasConnected) {
    serviceStaWasConnected = false;
    logWifiWarn(
        "[WIFI-STA] Utracono polaczenie STA podczas sesji WiFi. Proba ponownego polaczenia przez %ds.",
        WIFI_TIMEOUT / 1000);
    if (!connectStaWithTimeout("Odzyskiwanie STA z menu WiFi")) {
      startAPInternal("Utracono STA podczas sesji WiFi");
    }
    return;
  }

  maybeRunDailyTimeSync();
}

static void sendCaptiveRedirect() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.sendHeader("Location",
                    String("http://") + WiFi.softAPIP().toString() + "/",
                    true);
  server.send(302, "text/plain", "");
}

static void setupWebServer() {
  if (webServerConfigured) {
    return;
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "text/html; charset=utf-8", web_index_html);
  });

  server.on("/style.css", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "text/css; charset=utf-8", web_style_css);
  });

  server.on("/script.js", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "application/javascript; charset=utf-8", web_script_js);
  });

  server.on("/settime", HTTP_POST, []() {
    if (!server.hasArg("epoch")) {
      server.send(400, "text/plain", "Brak parametru epoch");
      return;
    }

    PowerManager::registerActivity();

    const long rawEpoch = server.arg("epoch").toInt();
    if (rawEpoch < 1700000000L) {
      server.send(400, "text/plain", "invalid_epoch");
      logWifiWarn("[TIME] HTTP /settime: odrzucono nierozsadny epoch=%ld.",
                  rawEpoch);
      return;
    }

    DateTime rtcTime(2000, 1, 1, 0, 0, 0);
    const bool hasKnownClock = getControllerClockNow(rtcTime);
    const DateTime newTime(rawEpoch);
    const long diff = hasKnownClock
                          ? labs(static_cast<long>(newTime.unixtime()) -
                                 static_cast<long>(rtcTime.unixtime()))
                          : LONG_MAX;

    if (diff > 60 || !hasKnownClock) {
      applySynchronizedTime(static_cast<uint32_t>(rawEpoch), "HTTP /settime");
      server.send(200, "text/plain", "OK");
      return;
    }

    setTimeSyncStatus(true, static_cast<uint32_t>(rawEpoch),
                      "HTTP /settime: czas byl juz aktualny.");
    logWifiInfo(
        "[TIME] HTTP /settime: pomijam zapis, roznica z RTC wynosi tylko %lds.",
        diff);
    server.send(200, "text/plain", "OK");
  });

  server.onNotFound([]() {
    if (isAPMode) {
      sendCaptiveRedirect();
    } else {
      server.send(404, "text/plain", "Error 404");
    }
  });

  // Captive portal probes (Android/iOS/Windows)
  server.on("/generate_204", HTTP_GET, []() { sendCaptiveRedirect(); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { sendCaptiveRedirect(); });
  server.on("/connecttest.txt", HTTP_GET, []() { sendCaptiveRedirect(); });
  server.on("/ncsi.txt", HTTP_GET, []() { sendCaptiveRedirect(); });
  server.on("/fwlink", HTTP_GET, []() { sendCaptiveRedirect(); });

  server.on(
      "/update", HTTP_POST,
      []() {
        if (otaUploadRejected) {
          server.sendHeader("Connection", "close");
          String html =
              "<!DOCTYPE html><html lang='pl'><head><meta charset='UTF-8'>"
              "<meta name='viewport' content='width=device-width, initial-"
              "scale=1.0'><title>OTA Status</title>";
          html += "<style>body{background-color:#0f172a;color:#f8fafc;font-"
                  "family:'Segoe UI',sans-serif;display:flex;justify-content:"
                  "center;align-items:center;height:100vh;margin:0;}";
          html += ".card{background:#1e293b;padding:40px;border-radius:16px;"
                  "box-shadow:0 10px 25px rgba(0,0,0,0.5);text-align:center;"
                  "border:1px solid #334155;max-width:520px;}";
          html +=
              "h2{margin:0 0 10px 0;font-size:24px;color:#ef4444;}"
              "p{color:#94a3b8;margin:0;line-height:1.5;}</style></head>"
              "<body><div class='card'><h2>OTA niedostepne</h2><p>";
          html += otaUploadRejectReason.length() > 0
                      ? otaUploadRejectReason
                      : "Inna sesja OTA jest juz aktywna.";
          html += "</p></div></body></html>";
          server.send(409, "text/html; charset=utf-8", html);
          otaUploadRejected = false;
          otaUploadRejectReason = "";
          return;
        }

        const bool otaSuccess = otaUploadActive && !Update.hasError();
        if (otaUploadActive) {
          OtaManager::endOtaUpdate(otaSuccess);
          otaUploadActive = false;
        }

        server.sendHeader("Connection", "close");
        String html =
            "<!DOCTYPE html><html lang='pl'><head><meta charset='UTF-8'><meta "
            "name='viewport' content='width=device-width, "
            "initial-scale=1.0'><title>OTA Status</title>";
        html += "<style>body{background-color:#0f172a;color:#f8fafc;font-"
                "family:'Segoe "
                "UI',sans-serif;display:flex;justify-content:center;align-"
                "items:center;height:100vh;margin:0;}";
        html += ".card{background:#1e293b;padding:40px;border-radius:16px;box-"
                "shadow:0 10px 25px rgba(0,0,0,0.5);text-align:center; border: "
                "1px solid #334155;}";
        html +=
            "h2{margin:0 0 10px 0;font-size:24px;} "
            "p{color:#94a3b8;margin:0;}</style></head><body><div class='card'>";
        html += otaSuccess
                    ? "<h2 style='color:#10b981;'>OK OTA</h2><p>Trwa restart "
                      "urzadzenia.<br>Przekierowanie za 10 sekund.</p>"
                    : "<h2 style='color:#ef4444;'>Blad OTA</h2><p>Plik "
                      "odrzucony lub uszkodzony.</p>";
        html += "</div><script>setTimeout(()=>window.location.href='/', "
                "10000);</script></body></html>";
        server.send(200, "text/html; charset=utf-8", html);

        if (otaSuccess) {
          OtaManager::prepareOutputsForRestart();
          delay(1000);
          ESP.restart();
        }
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          otaUploadRejected = false;
          otaUploadRejectReason = "";

          if (!otaUploadActive) {
            if (!OtaManager::tryBeginOtaUpdate("http")) {
              otaUploadRejected = true;
              otaUploadRejectReason =
                  "Sterownik wykonuje juz aktualizacje OTA przez inny kanal.";
              logWifiWarn("[OTA] Odrzucono HTTP OTA: inna sesja OTA aktywna.");
              return;
            }
            otaUploadActive = true;
          }
          logWifiInfo("[OTA] Pobieranie: %s", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            OtaManager::endOtaUpdate(false);
            otaUploadActive = false;
          }
        } else if (otaUploadRejected) {
          return;
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          const bool ok = Update.end(true);
          if (ok) {
            logWifiInfo("[OTA] Zakonczono pomyslnie (%u bajtow).",
                        upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
          Update.abort();
          OtaManager::endOtaUpdate(false);
          otaUploadActive = false;
          logWifiWarn("[OTA] Upload przerwany.");
        }
      });

  // Catch-all route, aby WebServer nie raportowal "request handler not found"
  // dla probe requestow captive portal i nieznanych sciezek.
  server.on(UriRegex("^/.*"), HTTP_ANY, []() {
    if (isAPMode) {
      sendCaptiveRedirect();
    } else {
      server.send(404, "text/plain", "Error 404");
    }
  });
  webServerConfigured = true;
}

static void startWebServerIfNeeded(const char *reason) {
  setupWebServer();
  if (webServerRunning) {
    return;
  }

  server.begin();
  webServerRunning = true;
  if (reason != nullptr && reason[0] != '\0') {
    logWifiInfo("[WIFI] HTTP server start: %s.", reason);
  }
}

static void stopWebServerIfNeeded() {
  if (!webServerRunning) {
    return;
  }

  server.stop();
  webServerRunning = false;
}

static void processWifiRequests() {
  if (serviceModeStopRequested) {
    serviceModeStopRequested = false;
    serviceModeStartRequested = false;
    stopServiceModeInternal("Sesja WiFi zakonczona z menu.");
  }

  if (serviceModeStartRequested) {
    serviceModeStartRequested = false;
    if (!serviceModeActive) {
      startServiceModeInternal();
    }
  }

  if (staOffRequested) {
    staOffRequested = false;
    staOnRequested = false;
    if (!serviceModeActive && !timeSyncInProgress && !isAPMode && !staIsOff) {
      turnWifiOffInternal("Nocny sleep.");
    }
  }

  if (staOnRequested) {
    staOnRequested = false;
  }
}

static void WifiTask(void *parameter) {
  (void)parameter;

  setupWebServer();
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_OFF);
  staIsOff = true;

  logWifiInfo(
      "[WIFI] Start: radio wylaczone. Dzienna synchronizacja o 09:00, AP tylko jako fallback z menu WiFi.");

  for (;;) {
    processWifiRequests();
    maintainWifiState();
    if (isAPMode) {
      dnsServer.processNextRequest();
    }
    if (webServerRunning) {
      server.handleClient();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void AkwariumWifi::begin() {
  BaseType_t result =
      xTaskCreatePinnedToCore(WifiTask, "WifiTask", 10240, nullptr, 1, nullptr, 1);
  if (result == pdPASS) {
    LogManager::logInfo("WIFI: uruchomiono zadanie sieciowe.");
  } else {
    LogManager::logError("WIFI: nie mozna uruchomic zadania sieciowego.");
  }
}

bool AkwariumWifi::getIsAPMode() { return isAPMode; }

void AkwariumWifi::startAP() {
  serviceModeStopRequested = false;
  staOffRequested = false;
  serviceModeStartRequested = true;
}

void AkwariumWifi::stopAP() {
  serviceModeStartRequested = false;
  serviceModeStopRequested = true;
}

void AkwariumWifi::requestStaOffForSleep() {
  if (serviceModeActive || timeSyncInProgress || isAPMode) {
    return;
  }
  serviceModeStartRequested = false;
  staOnRequested = false;
  staOffRequested = true;
}

void AkwariumWifi::requestStaOn() {
  if (serviceModeActive || isAPMode) {
    return;
  }
  staOffRequested = false;
  staOnRequested = true;
}

bool AkwariumWifi::isStaOff() { return staIsOff; }

bool AkwariumWifi::isStaConnected() { return staConnected; }

bool AkwariumWifi::isStaConnecting() { return staConnecting; }

bool AkwariumWifi::isServiceModeActive() { return serviceModeActive; }

bool AkwariumWifi::isTimeSyncInProgress() { return timeSyncInProgress; }

String AkwariumWifi::getAPName() {
  return isAPMode ? getConfiguredApSsidString() : getConfiguredStaSsidString();
}

String AkwariumWifi::getAPPassword() {
  return isAPMode ? getConfiguredApPasswordString() : String();
}

String AkwariumWifi::getConfiguredStaSsid() {
  return getConfiguredStaSsidString();
}

String AkwariumWifi::getConfiguredAPName() { return getConfiguredApSsidString(); }

String AkwariumWifi::getConfiguredAPPassword() {
  return getConfiguredApPasswordString();
}

String AkwariumWifi::getStaSsid() {
  return staLastConnectedSsid.length() > 0 ? staLastConnectedSsid
                                           : getConfiguredStaSsidString();
}

uint32_t AkwariumWifi::getStaLastConnectedEpoch() {
  return staLastConnectedEpoch;
}

uint32_t AkwariumWifi::getLastTimeSyncEpoch() { return lastTimeSyncEpoch; }

bool AkwariumWifi::wasLastTimeSyncSuccessful() {
  return lastTimeSyncSuccessful;
}

String AkwariumWifi::getLastTimeSyncStatus() { return lastTimeSyncStatus; }

String AkwariumWifi::getIP() {
  return isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

uint8_t AkwariumWifi::getConnectedClients() {
  return isAPMode ? WiFi.softAPgetStationNum() : 0;
}
