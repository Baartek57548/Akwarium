#include "AkwariumWifi.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "SecretConfig.h"
#include "SystemController.h"
#include "WebAssets.h"
#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <uri/UriRegex.h>
#include <cstring>
#include <esp_err.h>
#include <esp_wifi.h>
#include <sys/time.h>
#include <time.h>
#include <RTClib.h>

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
static bool otaUploadRejected = false;
static String otaUploadRejectReason;
static volatile bool apStartRequested = false;
static volatile bool apStopRequested = false;
static volatile bool staOffRequested = false;
static volatile bool staOnRequested = false;
static volatile bool staIsOff = false;
static unsigned long lastStaReconnectAttemptMs = 0;
static unsigned long staDisconnectedSinceMs = 0;
static uint8_t staReconnectAttempts = 0;
static const unsigned long STA_RECONNECT_INTERVAL_MS = 15000UL;
static const unsigned long STA_FALLBACK_TO_AP_MS = 180000UL;
static const unsigned long WIFI_MODE_SWITCH_DELAY_MS = 25UL;

WebServer &AkwariumWifi::getServer() { return server; }
static void startAPInternal();
static void sendCaptiveRedirect();

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
    staDisconnectedSinceMs = 0;
    staReconnectAttempts = 0;
    Serial.println("\n[WIFI] Polaczono. IP: " + WiFi.localIP().toString());
  } else {
    isAPMode = false;
    Serial.println(
        "\n[WIFI] Timeout. Polaczenie STA nieudane. Projekt dziala offline.");
  }
}

static void maintainStaConnection() {
  if (isAPMode || staIsOff) {
    staDisconnectedSinceMs = 0;
    staReconnectAttempts = 0;
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    staDisconnectedSinceMs = 0;
    staReconnectAttempts = 0;
    return;
  }

  const unsigned long nowMs = millis();
  if (staDisconnectedSinceMs == 0) {
    staDisconnectedSinceMs = nowMs;
  }

  if (nowMs - lastStaReconnectAttemptMs >= STA_RECONNECT_INTERVAL_MS) {
    lastStaReconnectAttemptMs = nowMs;
    if (staReconnectAttempts < 255) {
      staReconnectAttempts++;
    }
    Serial.printf("[WIFI-STA] Ponowna proba laczenia (%u)\n",
                  static_cast<unsigned>(staReconnectAttempts));
    WiFi.reconnect();
  }

  if (nowMs - staDisconnectedSinceMs >= STA_FALLBACK_TO_AP_MS) {
    Serial.println("[WIFI-STA] Brak lacznosci >3 min. Fallback do AP.");
    staDisconnectedSinceMs = 0;
    staReconnectAttempts = 0;
    startAPInternal();
  }
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
    if (server.hasArg("epoch")) {
      PowerManager::registerActivity();
      time_t epoch = server.arg("epoch").toInt();

      if (!SystemController::isRtcReady()) {
        server.send(503, "text/plain", "rtc_unavailable");
        return;
      }

      // Sprawdz czy RTC jest dostepne i czy czas jest rozsadny
      DateTime rtcTime = SystemController::rtc.now();
      DateTime newTime(epoch);

      // Aktualizuj RTC tylko jesli roznica jest wieksza niz 1 minuta
      // lub jesli RTC ma niepoprawny czas
      long diff = abs((long)newTime.unixtime() - (long)rtcTime.unixtime());
      if (diff > 60 || rtcTime.year() < 2024 || rtcTime.year() > 2030) {
        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        syncSystemTime((uint32_t)epoch);

        struct tm timeinfo;
        getLocalTime(&timeinfo);
        Serial.println(&timeinfo, "[RTC] Zsynchronizowano czas ukladu: %Y-%m-%d %H:%M:%S");
      } else {
        Serial.println("[RTC] Czas w RTC jest juz aktualny, pomijam synchronizacje");
      }

      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Brak parametru epoch");
    }
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
              Serial.println("[OTA] Odrzucono HTTP OTA: inna sesja OTA aktywna.");
              return;
            }
            otaUploadActive = true;
          }
          Serial.printf("[OTA] Pobieranie: %s\n", upload.filename.c_str());
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

  // Catch-all route, aby WebServer nie raportowal "request handler not found"
  // dla probe requestow captive portal i nieznanych sciezek.
  server.on(UriRegex("^/.*"), HTTP_ANY, []() {
    if (isAPMode) {
      sendCaptiveRedirect();
    } else {
      server.send(404, "text/plain", "Error 404");
    }
  });

  server.begin();
}

static void startAPInternal() {
  if (isAPMode) {
    return;
  }

  const size_t ssidLen = strlen(apSSID);
  const size_t passLen = strlen(apPassword);
  if (ssidLen == 0 || ssidLen > 31) {
    Serial.println(
        "[WIFI-AP] BLAD: AP_SSID musi miec dlugosc 1..31 znakow.");
    return;
  }
  if (passLen < 8 || passLen > 63) {
    Serial.println(
        "[WIFI-AP] BLAD: AP_PASSWORD musi miec dlugosc 8..63 znakow. AP nie zostal uruchomiony.");
    return;
  }

  WiFi.disconnect(false, false);
  delay(WIFI_MODE_SWITCH_DELAY_MS);
  WiFi.mode(WIFI_AP_STA);
  staIsOff = false;

  bool apStarted = WiFi.softAP(apSSID, apPassword);
  if (!apStarted) {
    // Druga proba po pelnym restarcie interfejsu WiFi.
    WiFi.mode(WIFI_OFF);
    delay(WIFI_MODE_SWITCH_DELAY_MS);
    WiFi.mode(WIFI_AP);
    apStarted = WiFi.softAP(apSSID, apPassword);
  }

  if (apStarted) {
    wifi_config_t apCfg = {};
    const esp_err_t cfgErr = esp_wifi_get_config(WIFI_IF_AP, &apCfg);
    if (cfgErr == ESP_OK) {
      if (apCfg.ap.authmode == WIFI_AUTH_OPEN) {
        Serial.println(
            "[WIFI-AP] BLAD: AP uruchomil sie jako OPEN mimo hasla. Wylaczam AP.");
        WiFi.softAPdisconnect(true);
        isAPMode = false;
        staIsOff = true;
        return;
      }
      Serial.printf("[WIFI-AP] SSID=%s auth=%d passLen=%u\n", apSSID,
                    static_cast<int>(apCfg.ap.authmode),
                    static_cast<unsigned>(passLen));
    } else {
      Serial.printf("[WIFI-AP] Ostrzezenie: esp_wifi_get_config(AP) failed: %s\n",
                    esp_err_to_name(cfgErr));
    }

    dnsServer.stop();
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    isAPMode = true;
    Serial.println("[WIFI-AP] Uruchomiono AP. IP: " +
                   WiFi.softAPIP().toString());
  } else {
    Serial.printf(
        "[WIFI-AP] BLAD: nie udalo sie uruchomic AP (mode=%d, passLen=%u).\n",
        static_cast<int>(WiFi.getMode()), static_cast<unsigned>(passLen));
    isAPMode = false;
    staIsOff = true;
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

static void disableStaForSleepInternal() {
  if (isAPMode) {
    return;
  }

  WiFi.mode(WIFI_STA);
  // Stabilniejsza sciezka na ESP32-S3:
  // unikamy hard un-init (disconnect(..., true)), bo potrafi logowac
  // "wifi:timeout when WiFi un-init, type=4".
  WiFi.disconnect(false, false);
  staIsOff = true;
  staDisconnectedSinceMs = 0;
  staReconnectAttempts = 0;
  Serial.println("[WIFI-STA] Wylaczono STA dla nocnego sleep.");
}

static void enableStaAfterSleepInternal() {
  if (isAPMode || !staIsOff) {
    return;
  }

  setupNetwork();
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
    staOnRequested = false;
    disableStaForSleepInternal();
  }

  if (staOnRequested && !isAPMode) {
    staOnRequested = false;
    enableStaAfterSleepInternal();
  }
}

static void WifiTask(void *parameter) {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  setupNetwork();
  setupWebServer();

  for (;;) {
    processAPRequests();
    maintainStaConnection();
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

void AkwariumWifi::requestStaOffForSleep() {
  apStartRequested = false;
  staOnRequested = false;
  staOffRequested = true;
}

void AkwariumWifi::requestStaOn() {
  if (isAPMode) {
    return;
  }
  staOffRequested = false;
  staOnRequested = true;
}

bool AkwariumWifi::isStaOff() { return staIsOff; }

String AkwariumWifi::getAPName() {
  return isAPMode ? String(apSSID) : String(STA_SSID);
}

String AkwariumWifi::getAPPassword() {
  return isAPMode ? String(apPassword) : String(STA_PASSWORD);
}

String AkwariumWifi::getConfiguredAPName() { return String(apSSID); }

String AkwariumWifi::getConfiguredAPPassword() { return String(apPassword); }

String AkwariumWifi::getIP() {
  return isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

uint8_t AkwariumWifi::getConnectedClients() {
  return isAPMode ? WiFi.softAPgetStationNum() : 0;
}


