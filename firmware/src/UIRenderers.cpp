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
    out[outIdx++] = c;
  }
  out[outIdx] = '\0';

  if (outIdx == 0) {
    snprintf(out, outSize, "--.-C");
  }
}

static void drawLightIcon(U8G2 *oled, int x, int y, bool on) {
  if (on) {
    oled->drawDisc(x + 4, y + 2, 2, U8G2_DRAW_ALL);
    oled->drawBox(x + 3, y + 4, 2, 3);
    oled->drawBox(x + 2, y + 7, 4, 1);
  } else {
    oled->drawCircle(x + 4, y + 2, 2, U8G2_DRAW_ALL);
    oled->drawFrame(x + 3, y + 4, 2, 3);
    oled->drawLine(x + 2, y + 7, x + 5, y + 7);
  }
}

static void drawFilterIcon(U8G2 *oled, int x, int y, bool on) {
  if (on) {
    oled->drawDisc(x + 2, y + 3, 1, U8G2_DRAW_ALL);
  } else {
    oled->drawCircle(x + 2, y + 3, 1, U8G2_DRAW_ALL);
  }

  oled->drawLine(x + 0, y + 6, x + 3, y + 6);
  oled->drawLine(x + 4, y + 5, x + 7, y + 5);
  oled->drawLine(x + 0, y + 4, x + 2, y + 4);
  oled->drawLine(x + 3, y + 3, x + 5, y + 3);
  oled->drawLine(x + 6, y + 4, x + 7, y + 4);
  if (on) {
    oled->drawLine(x + 0, y + 5, x + 2, y + 5);
    oled->drawLine(x + 3, y + 4, x + 5, y + 4);
    oled->drawLine(x + 6, y + 5, x + 7, y + 5);
  }
}

static void drawHeaterIcon(U8G2 *oled, int x, int y, bool on) {
  // Stylizowana spirala 8x8.
  oled->drawLine(x + 1, y + 1, x + 3, y + 1);
  oled->drawLine(x + 3, y + 1, x + 3, y + 3);
  oled->drawLine(x + 3, y + 3, x + 1, y + 3);
  oled->drawLine(x + 1, y + 3, x + 1, y + 5);
  oled->drawLine(x + 1, y + 5, x + 3, y + 5);
  oled->drawLine(x + 5, y + 1, x + 7, y + 1);
  oled->drawLine(x + 7, y + 1, x + 7, y + 3);
  oled->drawLine(x + 7, y + 3, x + 5, y + 3);
  oled->drawLine(x + 5, y + 3, x + 5, y + 5);
  oled->drawLine(x + 5, y + 5, x + 7, y + 5);
  if (on) {
    oled->drawLine(x + 2, y + 2, x + 2, y + 6);
    oled->drawLine(x + 6, y + 2, x + 6, y + 6);
  }
}

static void drawAerationIcon(U8G2 *oled, int x, int y) {
  oled->drawCircle(x + 2, y + 6, 1, U8G2_DRAW_ALL);
  oled->drawCircle(x + 4, y + 4, 1, U8G2_DRAW_ALL);
  oled->drawCircle(x + 6, y + 2, 1, U8G2_DRAW_ALL);
}

static void drawFeederIcon(U8G2 *oled, int x, int y) {
  oled->drawFrame(x + 1, y + 3, 6, 4);
  oled->drawLine(x + 1, y + 2, x + 6, y + 2);
  oled->drawPixel(x + 3, y + 1);
  oled->drawPixel(x + 5, y + 1);
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

  // Sekcja gorna 0..21: czas + temperatura
  char timeShort[6];
  snprintf(timeShort, sizeof(timeShort), "%02d:%02d", currentHour, currentMinute);

  char temperatureDisplay[12];
  formatHomeTemperature(tempBuffer, temperatureDisplay, sizeof(temperatureDisplay));

  display->setFont(u8g2_font_logisoso16_tn);
  display->drawStr(0, 18, timeShort);
  int16_t tempWidth = display->getStrWidth(temperatureDisplay);
  int16_t tempX = 127 - tempWidth + 1;
  if (tempX < 66) {
    tempX = 66;
  }
  display->drawStr(tempX, 18, temperatureDisplay);

  // Separatory zgodnie z blueprintem.
  display->drawLine(0, 22, 127, 22);
  display->drawLine(64, 0, 64, 21);

  // Pasek statusu 24..31
  drawLightIcon(display, 0, 24, isLightOn);
  drawFilterIcon(display, 12, 24, isFilterOn);
  drawHeaterIcon(display, 24, 24, isHeaterOn);
  drawAerationIcon(display, 36, 24);
  drawFeederIcon(display, 48, 24);

  // Bateria wyrównana do prawej (ok. 16x8 px).
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
