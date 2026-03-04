#include "AkwariumWifi.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SecretConfig.h"
#include "WebAssets.h"
#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

// ==========================================
// KONFIGURACJA SIECI
// ==========================================
static const char *STA_SSID = SECRET_SSID;
static const char *STA_PASSWORD = SECRET_PASS;
static const int WIFI_TIMEOUT = 6000;

static const char *apSSID = AP_SSID;
static const char *apPassword = AP_PASSWORD;

// Stare wbudowane GUI zostalo przeniesione do zewnetrznych plikow w WebAssets.h

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;
static bool isAPMode = false;
static bool otaUploadActive = false;
static volatile bool apStartRequested = false;
static volatile bool apStopRequested = false;
static volatile bool staOffRequested = false;
static volatile bool staIsOff = false;

WebServer &AkwariumWifi::getServer() { return server; }

static bool parseUintStrict(const String &raw, uint64_t &out) {
  if (raw.length() == 0) {
    return false;
  }

  errno = 0;
  char *endPtr = nullptr;
  unsigned long long parsed = strtoull(raw.c_str(), &endPtr, 10);
  if (errno == ERANGE || endPtr == raw.c_str() || *endPtr != '\0') {
    return false;
  }

  out = static_cast<uint64_t>(parsed);
  return true;
}

static bool parseLongStrict(const String &raw, long &out) {
  if (raw.length() == 0) {
    return false;
  }

  errno = 0;
  char *endPtr = nullptr;
  long parsed = strtol(raw.c_str(), &endPtr, 10);
  if (errno == ERANGE || endPtr == raw.c_str() || *endPtr != '\0') {
    return false;
  }

  out = parsed;
  return true;
}

static bool parseEpochArg(const String &raw, uint32_t &epochSec) {
  uint64_t parsed = 0;
  if (!parseUintStrict(raw, parsed)) {
    return false;
  }

  // Akceptujemy epoch w sekundach lub milisekundach.
  if (parsed > 4102444800ULL && parsed <= 4102444800000ULL) {
    parsed /= 1000ULL;
  }

  if (parsed < 1704067200ULL || parsed > 4102444800ULL) {
    return false;
  }

  epochSec = static_cast<uint32_t>(parsed);
  return true;
}

static void setupNetwork() {
  WiFi.mode(WIFI_STA);
  staIsOff = false;
  WiFi.begin(STA_SSID, STA_PASSWORD);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    Serial.println("\n[WIFI] Polaczono. IP: " + WiFi.localIP().toString());
  } else {
    isAPMode = false;
    Serial.println(
        "\n[WIFI] Timeout. Polaczenie STA nieudane. Projekt dziala offline.");
  }
}

static void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "text/html", web_index_html);
  });

  server.on("/style.css", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "public, max-age=60");
    server.send_P(200, "text/css", web_style_css);
  });

  server.on("/script.js", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "public, max-age=60");
    server.send_P(200, "application/javascript", web_script_js);
  });

  server.on("/settime", HTTP_POST, []() {
    if (server.hasArg("epoch")) {
      PowerManager::registerActivity();
      uint32_t epochUtc = 0;
      if (!parseEpochArg(server.arg("epoch"), epochUtc)) {
        server.send(400, "text/plain", "Niepoprawny epoch");
        return;
      }

      uint32_t rtcEpoch = epochUtc;
      if (server.hasArg("tzOffsetMin")) {
        long tzOffsetMin = 0;
        if (!parseLongStrict(server.arg("tzOffsetMin"), tzOffsetMin) ||
            tzOffsetMin < -840 || tzOffsetMin > 840) {
          server.send(400, "text/plain", "Niepoprawny tzOffsetMin");
          return;
        }

        // JS: getTimezoneOffset() = UTC - LOCAL (w minutach).
        int64_t adjusted = static_cast<int64_t>(epochUtc) -
                           static_cast<int64_t>(tzOffsetMin) * 60LL;
        if (adjusted < 1704067200LL || adjusted > 4102444800LL) {
          server.send(400, "text/plain", "Epoch poza zakresem");
          return;
        }
        rtcEpoch = static_cast<uint32_t>(adjusted);
      }

      time_t epoch = static_cast<time_t>(epochUtc);
      struct timeval tv;
      tv.tv_sec = epoch;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);

      syncSystemTime(rtcEpoch);

      struct tm timeinfo;
      getLocalTime(&timeinfo);
      Serial.println(&timeinfo,
                     "[RTC] Zsynchronizowano czas ukladu: %Y-%m-%d %H:%M:%S");

      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Brak parametru epoch");
    }
  });

  server.onNotFound([]() {
    if (isAPMode) {
      server.sendHeader("Location",
                        String("http://") + WiFi.softAPIP().toString() + "/",
                        true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Error 404");
    }
  });
  server.on(
      "/update", HTTP_POST,
      []() {
        const bool otaSuccess = !Update.hasError();
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
        server.send(200, "text/html", html);

        if (otaSuccess) {
          delay(1000);
          ESP.restart();
        }
      },
      []() {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          if (!otaUploadActive) {
            OtaManager::beginOtaUpdate();
            otaUploadActive = true;
          }
          Serial.printf("[OTA] Pobieranie: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            OtaManager::endOtaUpdate(false);
            otaUploadActive = false;
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          bool ok = Update.end(true);
          if (ok) {
            Serial.printf("[OTA] Zakonczono pomyslnie (%u bajtow)\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
          Update.abort();
          OtaManager::endOtaUpdate(false);
          otaUploadActive = false;
          Serial.println("[OTA] Upload przerwany.");
        }
      });

  server.begin();
}

static void startAPInternal() {
  if (isAPMode) {
    return;
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  staIsOff = true;
  if (WiFi.softAP(apSSID, apPassword)) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    isAPMode = true;
    Serial.println("[WIFI-AP] Uruchomiono AP. IP: " +
                   WiFi.softAPIP().toString());
  } else {
    Serial.println("[WIFI-AP] BLAD: nie udalo sie uruchomic AP.");
  }
}

static void stopAPInternal() {
  if (!isAPMode) {
    return;
  }

  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  isAPMode = false;
  staIsOff = true;
  Serial.println("[WIFI-AP] Wylaczono AP.");
  // UWAGA: Nie wywolujemy setupNetwork() - blokowaloby i mogloby powodowac WDT.
}

static void disableStaForDeepSleepInternal() {
  if (isAPMode) {
    return;
  }

  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  staIsOff = true;
  Serial.println("[WIFI-STA] Wylaczono STA/radio dla deep sleep.");
}

static void processAPRequests() {
  if (apStopRequested) {
    apStopRequested = false;
    apStartRequested = false;
    stopAPInternal();
  }

  if (apStartRequested) {
    apStartRequested = false;
    startAPInternal();
  }

  if (staOffRequested && !isAPMode) {
    staOffRequested = false;
    disableStaForDeepSleepInternal();
  }
}

static void WifiTask(void *parameter) {
  setupNetwork();
  setupWebServer();

  for (;;) {
    processAPRequests();
    if (isAPMode)
      dnsServer.processNextRequest();
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS); // Przekazanie sterowania dla FreeRTOS
  }
}

void AkwariumWifi::begin() {
  xTaskCreatePinnedToCore(WifiTask,   // Funkcja zadania
                          "WifiTask", // Nazwa zadania
                          8192,       // Rozmiar stosu
                          NULL,       // Parametry
                          1,          // Priorytet
                          NULL,       // Uchwyt
                          1);         // Rdzen 1
}

bool AkwariumWifi::getIsAPMode() { return isAPMode; }

void AkwariumWifi::startAP() {
  apStopRequested = false;
  staOffRequested = false;
  apStartRequested = true;
}

void AkwariumWifi::stopAP() {
  apStartRequested = false;
  apStopRequested = true;
}

void AkwariumWifi::requestStaOffForDeepSleep() {
  apStartRequested = false;
  staOffRequested = true;
}

bool AkwariumWifi::isStaOff() { return staIsOff; }

String AkwariumWifi::getAPName() {
  return isAPMode ? String(apSSID) : String(STA_SSID);
}

String AkwariumWifi::getAPPassword() {
  return isAPMode ? String(apPassword) : String(STA_PASSWORD);
}

String AkwariumWifi::getIP() {
  return isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

uint8_t AkwariumWifi::getConnectedClients() {
  return isAPMode ? WiFi.softAPgetStationNum() : 0;
}

