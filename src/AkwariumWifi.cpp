#include "AkwariumWifi.h"
#include "OtaManager.h"
#include "PowerManager.h"
#include "WebAssets.h"
#include "arduino_secrets.h"
#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
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

// Stare wbudowane GUI zostaĹ‚o przeniesione do zewnÄ™trznych plikĂłw w WebAssets.h

static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;
static bool isAPMode = false;
static bool otaUploadActive = false;

WebServer &AkwariumWifi::getServer() { return server; }

static void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASSWORD);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    Serial.println("\n[WIFI] PoĹ‚Ä…czono. IP: " + WiFi.localIP().toString());
  } else {
    isAPMode = false;
    Serial.println(
        "\n[WIFI] Timeout. PoĹ‚Ä…czenie STA nieudane. Projekt dziala Offline.");
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
      time_t epoch = server.arg("epoch").toInt();
      struct timeval tv;
      tv.tv_sec = epoch;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);

      syncSystemTime((uint32_t)epoch);

      struct tm timeinfo;
      getLocalTime(&timeinfo);
      Serial.println(&timeinfo,
                     "[RTC] Zsynchronizowano czas ukĹ‚adu: %Y-%m-%d %H:%M:%S");

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

static void WifiTask(void *parameter) {
  setupNetwork();
  setupWebServer();

  for (;;) {
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
                          1);         // RdzeĹ„ 1
}

bool AkwariumWifi::getIsAPMode() { return isAPMode; }

void AkwariumWifi::startAP() {
  if (!isAPMode) {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(apSSID, apPassword)) {
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      isAPMode = true;
      Serial.println("[WIFI-AP] Uruchomiono AP. IP: " +
                     WiFi.softAPIP().toString());
    }
  }
}

void AkwariumWifi::stopAP() {
  if (isAPMode) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    isAPMode = false;
    Serial.println("[WIFI-AP] WyĹ‚Ä…czono AP.");
    // UWAGA: Nie wywolujemy setupNetwork() - blokowaloby na 10s i powodowalo
    // reset przez WDT (watchdog = 5s), co uruchamia kalibracje karmnika!
  }
}

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

