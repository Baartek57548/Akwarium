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

void HomeRenderer::drawFrame(AquariumAnimation *ctx) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // Layer 4: Time.
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(0, 5, timeBuffer);
  display->drawLine(0, 6, 127, 6);

  // Reset reason banner (30s po starcie, tylko dla crash-resetow)
  {
    const char *rstLabel = SystemController::getLastResetLabel();
    if (rstLabel != nullptr && millis() < 30000UL) {
      char rstBuf[22];
      snprintf(rstBuf, sizeof(rstBuf), "RST:%s c=%d", rstLabel,
               SystemController::getLastResetReason());
      display->setDrawColor(1);
      display->drawBox(0, 0, 128, 7);
      display->setDrawColor(0);
      display->setFont(u8g2_font_4x6_tr);
      display->drawStr(1, 6, rstBuf);
      display->setDrawColor(1);
    }
  }

  // Left block: filter/light status and divider.
  display->drawXBMP(1, 8, 7, 10, image_Layer_19_bits);
  display->drawXBMP(0, 21, 9, 11, image_Layer_19_1_bits);
  if (isFilterOn)
    display->drawDisc(14, 13, 3, U8G2_DRAW_ALL);
  else
    display->drawXBMP(11, 10, 7, 7, image_Layer_20_bits);
  if (isLightOn)
    display->drawDisc(14, 26, 3, U8G2_DRAW_ALL);
  else
    display->drawXBMP(11, 23, 7, 7, image_Layer_20_bits);
  display->drawLine(0, 19, 19, 19);

  // Middle blocks.
  display->drawLine(20, 7, 20, 31);
  display->drawXBMP(22, 8, 9, 23, image_Layer_21_bits);

  display->drawXBMP(32, 14, 11, 11, image_Layer_22_bits);
  if (isHeaterOn)
    display->drawDisc(37, 19, 2, U8G2_DRAW_ALL);
  else
    display->drawCircle(37, 19, 2, U8G2_DRAW_ALL);
  display->drawLine(45, 7, 45, 31);

  display->drawXBMP(48, 9, 9, 23, image_Layer_25_bits);
  if (batteryPercent > 0) {
    int fillHeight = (20 * batteryPercent) / 100;
    if (fillHeight < 1)
      fillHeight = 1;
    display->drawBox(50, 31 - fillHeight, 5, fillHeight);
  }
  // Ensure battery base is always visible.
  display->drawLine(50, 31, 54, 31);

  display->drawLine(59, 7, 59, 31);
  display->drawXBMP(62, 8, 24, 12, image_Layer_19_2_bits);

  // Text overlays.
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(44, 5, tempBuffer);
  display->drawStr(89, 5, dateBuffer);
  display->setFont(u8g2_font_profont11_tr);
  display->drawStr(65, 28, valBuffer);

  // Right block.
  display->drawLine(87, 7, 87, 31);
  display->drawXBMP(87, 16, 41, 7, image_Layer_24_bits);

  char feedTime[6];
  if (feedFreq == 0)
    snprintf(feedTime, sizeof(feedTime), "--:--");
  else
    snprintf(feedTime, sizeof(feedTime), "%02d:%02d", feedHour, feedMinute);
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(89, 13, feedTime);

  if (feedFreq == 0) {
    display->drawStr(112, 13, "OFF");
  } else {
    display->drawEllipse(120, 11, 7, 3);
    display->drawVLine(118, 9, 5);
    display->drawVLine(122, 9, 5);
    if (feedFreq >= 1)
      display->drawBox(114, 10, 4, 3);
    if (feedFreq >= 2)
      display->drawBox(118, 10, 4, 3);
    if (feedFreq >= 3)
      display->drawBox(122, 10, 4, 3);
  }

  display->drawEllipse(73, 25, 12, 6);
  display->drawXBMP(89, 25, 37, 5, image_Layer_29_bits);
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
