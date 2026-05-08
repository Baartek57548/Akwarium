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
#define isFeedingAnim ctx->isFeedingAnim
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

static void drawBatteryIndicator(U8G2 *oled, int x, int y, uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  oled->drawFrame(x, y, 13, 7);
  oled->drawFrame(x + 13, y + 2, 2, 3);

  int fillWidth = (11 * percent) / 100;
  if (fillWidth > 0) {
    oled->drawBox(x + 1, y + 1, fillWidth, 5);
  }
}

static void drawTinyFeedingIndicator(U8G2 *oled, int x, int y) {
  oled->drawTriangle(x, y + 3, x + 4, y + 1, x + 4, y + 5);
  oled->drawDisc(x + 8, y + 3, 4, U8G2_DRAW_ALL);
  oled->setDrawColor(0);
  oled->drawPixel(x + 9, y + 2);
  oled->setDrawColor(1);
  oled->drawPixel(x + 14, y + 1);
  oled->drawPixel(x + 15, y + 4);
}

} // namespace

void HomeRenderer::drawFrame(AquariumAnimation *ctx) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // Home pokazuje tylko dwa komunikaty: czas oraz jedna informacja o akwarium.
  char timeMain[6];
  snprintf(timeMain, sizeof(timeMain), "%02d:%02d", currentHour, currentMinute);

  char temperatureDisplay[12];
  formatHomeTemperature(tempBuffer, temperatureDisplay, sizeof(temperatureDisplay));

  display->setFont(u8g2_font_timR24_tr);
  int16_t timeX = (128 - display->getStrWidth(timeMain)) / 2;
  if (timeX < 0) {
    timeX = 0;
  }
  display->drawStr(timeX, 23, timeMain);

  display->drawHLine(7, 24, 101);

  display->setFont(u8g2_font_5x7_tr);
  int16_t tempX = (128 - display->getStrWidth(temperatureDisplay)) / 2;
  if (tempX < 0) {
    tempX = 0;
  }
  display->drawStr(tempX, 31, temperatureDisplay);

  // Jeden maly wskaznik: karmienie ma priorytet, w innym razie bateria.
  if (isFeedingAnim) {
    drawTinyFeedingIndicator(display, 112, 25);
  } else {
    drawBatteryIndicator(display, 112, 25, batteryPercent);
  }
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
