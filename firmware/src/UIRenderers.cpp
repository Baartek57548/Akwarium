#include "UIRenderers.h"
#include "AquariumAnimation.h"
#include "AquariumBitmaps.h"
#include "SystemController.h"

// Makra ulatwiajace migracje - dzieki temu unikamy 1000 refaktorow ctx->
#define display ctx->display
#define timeBuffer ctx->timeBuffer
#define tempBuffer ctx->tempBuffer
#define dateBuffer ctx->dateBuffer
#define valBuffer ctx->valBuffer
#define batteryVoltageBuffer ctx->batteryVoltageBuffer
#define feedTimeBuffer ctx->feedTimeBuffer
#define batteryPercent ctx->batteryPercent
#define isFilterOn ctx->isFilterOn
#define isLightOn ctx->isLightOn
#define isHeaterOn ctx->isHeaterOn
#define feedFreq ctx->feedFreq
#define feedDaysPassed ctx->feedDaysPassed
#define feedHour ctx->feedHour
#define feedMinute ctx->feedMinute
#define menuSelection ctx->menuSelection
#define menuScrollOffset ctx->menuScrollOffset
#define scheduleSelection ctx->scheduleSelection
#define isEditing ctx->isEditing
#define editState ctx->editState
#define tempHour ctx->tempHour
#define tempMinute ctx->tempMinute
#define tempSecond ctx->tempSecond
#define tempDay ctx->tempDay
#define tempMonth ctx->tempMonth
#define tempYear ctx->tempYear
#define scheduleHourOn ctx->scheduleHourOn
#define scheduleMinOn ctx->scheduleMinOn
#define scheduleHourOff ctx->scheduleHourOff
#define scheduleMinOff ctx->scheduleMinOff
#define aerationHourOn ctx->aerationHourOn
#define aerationMinOn ctx->aerationMinOn
#define aerationHourOff ctx->aerationHourOff
#define aerationMinOff ctx->aerationMinOff
#define filterHourOn ctx->filterHourOn
#define filterMinOn ctx->filterMinOn
#define filterHourOff ctx->filterHourOff
#define filterMinOff ctx->filterMinOff
#define targetTemp ctx->targetTemp
#define currentHour ctx->currentHour
#define currentMinute ctx->currentMinute
#define currentSecond ctx->currentSecond
#define currentDay ctx->currentDay
#define currentMonth ctx->currentMonth
#define currentYear ctx->currentYear
#define activeScheduleId ctx->activeScheduleId
#define scheduleChangePending ctx->scheduleChangePending
#define timeChangePending ctx->timeChangePending
#define logs ctx->logs
#define logCount ctx->logCount
#define logScroll ctx->logScroll
#define testSelection ctx->testSelection
#define testLight ctx->testLight
#define testHeater ctx->testHeater
#define testFilter ctx->testFilter
#define testAerationVal ctx->testAerationVal
#define confirmAnimActive ctx->confirmAnimActive
#define confirmAnimFrame ctx->confirmAnimFrame
#define confirmAnimLastStep ctx->confirmAnimLastStep

#define drawWaves ctx->drawWaves
#define moveFish ctx->moveFish
#define drawFood ctx->drawFood
#define drawFishObj ctx->drawFishObj
#define drawBubbles ctx->drawBubbles
#define initFishObjs ctx->initFishObjs
#define dropFood ctx->dropFood
#define getMenuSelection ctx->getMenuSelection
#define getScheduleSelection ctx->getScheduleSelection
#define isEditingActive ctx->isEditingActive
#define fish ctx->fish
#define food ctx->food
#define bubbles ctx->bubbles
#define feedingActive ctx->feedingActive
#define lastFeedTime ctx->lastFeedTime
#define waveOffset ctx->waveOffset

// Wymaga externalowania ikon i bitmap, bo wczesniej siedzialy u gory
// AquariumAnimation.cpp. Najlatwiej wyciagnac same metody i zamienic oryginaly
// na nakladki.

namespace {

static void formatHomeTemperature(const char *rawTemp, char *out,
                                  size_t outSize) {
  if (outSize == 0) {
    return;
  }

  const char *src = rawTemp ? rawTemp : "T:--.-'C";
  if (src[0] == 'T' && src[1] == ':') {
    src += 2;
  }

  size_t outIdx = 0;
  for (size_t i = 0; src[i] != '\0' && outIdx + 1 < outSize; i++) {
    char c = src[i];
    if (c == '\'') {
      continue;
    }
    if (c == 'C' || c == 'c') {
      break;
    }
    out[outIdx++] = c;
  }
  if (outIdx == 0) {
    snprintf(out, outSize, "--.-");
    outIdx = strlen(out);
  }

  if (outIdx + 2 < outSize) {
    out[outIdx++] = '*';
    out[outIdx++] = 'C';
  }
  out[outIdx] = '\0';
}

static void drawRelayStatusBadge(U8G2 *oled, int x, int y, char label,
                                 bool on) {
  char labelText[2] = {label, '\0'};
  oled->drawFrame(x, y, 12, 8);

  if (on) {
    oled->drawBox(x + 1, y + 1, 10, 6);
    oled->setDrawColor(0);
    oled->drawStr(x + 4, y + 7, labelText);
    oled->setDrawColor(1);
    oled->drawPixel(x + 1, y + 1);
  } else {
    oled->drawStr(x + 4, y + 7, labelText);
    oled->drawCircle(x + 2, y + 2, 1, U8G2_DRAW_ALL);
  }
}

static void drawStaticBadge(U8G2 *oled, int x, int y, char label) {
  char labelText[2] = {label, '\0'};
  oled->drawFrame(x, y, 10, 8);
  oled->drawStr(x + 3, y + 7, labelText);
}

static void drawBatteryIndicator(U8G2 *oled, int x, int y, uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  oled->drawFrame(x, y, 14, 8);
  oled->drawFrame(x + 14, y + 2, 2, 4);

  int fillWidth = (12 * percent) / 100;
  if (fillWidth > 0) {
    oled->drawBox(x + 1, y + 1, fillWidth, 6);
  }
}

} // namespace

void HomeRenderer::drawFrame(AquariumAnimation *ctx) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // Sekcja gorna 0..21: czas + temperatura.
  char timeMain[6];
  snprintf(timeMain, sizeof(timeMain), "%02d:%02d", currentHour, currentMinute);
  char secondsText[3];
  snprintf(secondsText, sizeof(secondsText), "%02d", currentSecond);

  char temperatureDisplay[12];
  formatHomeTemperature(tempBuffer, temperatureDisplay, sizeof(temperatureDisplay));

  display->setFont(u8g2_font_logisoso16_tn);
  display->drawStr(0, 18, timeMain);

  const int16_t timeMainWidth = display->getStrWidth(timeMain);
  display->setFont(u8g2_font_6x10_tr);
  display->drawStr(timeMainWidth + 2, 18, secondsText);

  display->setFont(u8g2_font_logisoso16_tn);
  int16_t tempWidth = display->getStrWidth(temperatureDisplay);
  int16_t tempX = 127 - tempWidth + 1;
  if (tempX < 77) {
    tempX = 77;
  }
  display->drawStr(tempX, 18, temperatureDisplay);

  // Separatory zgodnie z blueprintem.
  display->drawLine(0, 22, 127, 22);
  display->drawLine(74, 0, 74, 21);

  // Pasek statusu 24..31 (czytelne statusy przekaźnikow).
  display->setFont(u8g2_font_4x6_tr);
  drawRelayStatusBadge(display, 0, 24, 'L', isLightOn);
  drawRelayStatusBadge(display, 13, 24, 'F', isFilterOn);
  drawRelayStatusBadge(display, 26, 24, 'H', isHeaterOn);
  drawStaticBadge(display, 40, 24, 'A');
  drawStaticBadge(display, 52, 24, 'K');

  // Bateria wyrownana do prawej (ok. 16x8 px).
  drawBatteryIndicator(display, 112, 24, batteryPercent);
}

void HomeRenderer::drawFeedingScreen(AquariumAnimation *ctx) {
  drawWaves(waveOffset++);
  drawBubbles();
  moveFish();
  for (int i = 0; i < NUM_FISH; i++)
    drawFishObj(fish[i].x, fish[i].y, fish[i].dir);
  if (feedingActive) {
    drawFood();
    bool anyVisible = false;
    for (int j = 0; j < NUM_FOOD; j++)
      if (food[j].visible)
        anyVisible = true;
    if (!anyVisible)
      feedingActive = false;
  }
}
