#include "SettingsRenderer.h"

#include "AquariumAnimation.h"
#include "AquariumBitmaps.h"
#include "ConfigData.h"

#include <U8g2lib.h>

// Makra utrzymuja ten sam styl, ktory maja istniejace renderery.
#define display ctx->display
#define menuSelection ctx->menuSelection
#define menuScrollOffset ctx->menuScrollOffset
#define scheduleSelection ctx->scheduleSelection
#define isEditing ctx->isEditing
#define editState ctx->editState
#define tempHour ctx->tempHour
#define tempMinute ctx->tempMinute
#define lightMode ctx->lightMode
#define aerationMode ctx->aerationMode
#define filterMode ctx->filterMode
#define heaterMode ctx->heaterMode
#define targetTemp ctx->targetTemp
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
#define feedHour ctx->feedHour
#define feedMinute ctx->feedMinute
#define feedFreq ctx->feedFreq
#define currentHour ctx->currentHour
#define currentMinute ctx->currentMinute
#define aerationPercent ctx->aerationPercent
#define batteryPercent ctx->batteryPercent
#define batteryVoltageBuffer ctx->batteryVoltageBuffer

namespace {

constexpr uint8_t OLED_W = 128;
constexpr uint8_t OLED_H = 32;
constexpr uint8_t NAV_X = 115;
constexpr uint8_t CONTENT_W = 114;
constexpr uint8_t MENU_TEXT_X = 20;
constexpr uint8_t ICON_TILE = 20;
constexpr uint8_t ICON_SIZE = 16;
constexpr uint8_t SYSTEM_ITEM_FACTORY_RESET = 3;

struct ScheduleCard {
  char label;
  const char *title;
  uint8_t mode;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  bool pointOnly;
};

struct SettingsMenuItem {
  SettingsScreen screen;
  const char *label;
  const char *hint;
};

static const SettingsMenuItem SETTINGS_MENU_ITEMS[] = {
    {SettingsScreen::HARMONOGRAMY, "Harmonogramy", "Swiatlo, CO2, filtr"},
    {SettingsScreen::TEMPERATURE_AND_HEATER, "Temp. i grzalka", "Prog ciepla"},
    {SettingsScreen::AERATION_CO2, "Napow. / CO2", "Cykl i polozenie"},
    {SettingsScreen::FEEDING, "Karmienie", "Godzina i rytm"},
    {SettingsScreen::WIFI, "Siec WiFi", "Tryb serwisowy"},
    {SettingsScreen::DISPLAY_AND_POWER, "Ekran i energia", "OLED i bateria"},
    {SettingsScreen::SYSTEM, "System", "Info, logi, reset"}};

constexpr uint8_t SETTINGS_MENU_COUNT =
    sizeof(SETTINGS_MENU_ITEMS) / sizeof(SETTINGS_MENU_ITEMS[0]);

static bool editCursorVisible() {
  return (millis() % 1280UL) < 860UL;
}

static void prepareDisplay(U8G2 *oled) {
  oled->setFontMode(1);
  oled->setBitmapMode(1);
}

static uint8_t shimmer(uint8_t width, uint16_t stepMs = 80) {
  if (width == 0) {
    return 0;
  }
  return static_cast<uint8_t>((millis() / stepMs) % width);
}

static void drawFocusFrame(U8G2 *oled, int x, int y, int w, int h,
                           bool active) {
  oled->drawHLine(x + 1, y, w - 2);
  oled->drawHLine(x + 1, y + h - 1, w - 2);
  oled->drawVLine(x, y + 1, h - 2);
  oled->drawVLine(x + w - 1, y + 1, h - 2);

  if (active) {
    const int sweepX = x + 2 + shimmer(max(1, w - 4), 70);
    oled->drawPixel(sweepX, y);
    if (sweepX + 1 < x + w - 1) {
      oled->drawPixel(sweepX + 1, y);
    }
  } else {
    oled->drawPixel(x + 1, y + 1);
    oled->drawPixel(x + w - 2, y + h - 2);
  }
}

static void drawCenteredText(U8G2 *oled, int x, int y, int w,
                             const char *text) {
  if (text == nullptr) {
    return;
  }
  const int textW = oled->getStrWidth(text);
  oled->drawStr(x + max(0, (w - textW) / 2), y, text);
}

static uint16_t minuteOfDay(uint8_t hour, uint8_t minute) {
  return (static_cast<uint16_t>(constrain(hour, 0, 23)) * 60U) +
         static_cast<uint16_t>(constrain(minute, 0, 59));
}

static const char *modeShortLabel(uint8_t mode) {
  if (mode == static_cast<uint8_t>(ScheduleMode::AlwaysOn)) {
    return "ON";
  }
  if (mode == static_cast<uint8_t>(ScheduleMode::AlwaysOff)) {
    return "OFF";
  }
  return "SCH";
}

static const char *feedQuickModeLabel(uint8_t freq) {
  if (freq == 0) {
    return "OFF";
  }
  if (freq == 1) {
    return "DAY";
  }
  return "SCH";
}

static const char *feedQuickEditLabel(uint8_t mode) {
  if (mode == 1) {
    return "DAY";
  }
  if (mode == 2) {
    return "OFF";
  }
  return "SCH";
}

static const char *feedFreqLabel(uint8_t freq) {
  switch (freq) {
  case 1:
    return "Codziennie";
  case 2:
    return "Co 2 dni";
  case 3:
    return "Co 3 dni";
  default:
    return "Wylaczone";
  }
}

static bool useCompactMenuFont(SettingsScreen screen) {
  return screen == SettingsScreen::TEMPERATURE_AND_HEATER ||
         screen == SettingsScreen::DISPLAY_AND_POWER;
}

static void formatTime(char *out, size_t outSize, uint8_t hour,
                       uint8_t minute) {
  if (outSize == 0U) {
    return;
  }
  snprintf(out, outSize, "%02u:%02u", static_cast<unsigned>(hour),
           static_cast<unsigned>(minute));
}

static const unsigned char *iconForSettingsScreen(SettingsScreen screen) {
  switch (screen) {
  case SettingsScreen::HARMONOGRAMY:
    return icon_ui_calendar_16_bits;
  case SettingsScreen::TEMPERATURE_AND_HEATER:
    return icon_ui_temp_16_bits;
  case SettingsScreen::AERATION_CO2:
    return icon_ui_co2_16_bits;
  case SettingsScreen::FEEDING:
    return icon_ui_feeding_16_bits;
  case SettingsScreen::WIFI:
    return icon_ui_wifi_16_bits;
  case SettingsScreen::DISPLAY_AND_POWER:
    return icon_ui_power_16_bits;
  case SettingsScreen::SYSTEM:
    return icon_ui_system_16_bits;
  default:
    return icon_ui_calendar_16_bits;
  }
}

static const unsigned char *iconForScheduleLabel(char label) {
  switch (label) {
  case 'L':
    return icon_ui_light_16_bits;
  case 'A':
    return icon_ui_co2_16_bits;
  case 'F':
    return icon_ui_filter_16_bits;
  case 'K':
    return icon_ui_feeding_16_bits;
  default:
    return icon_ui_calendar_16_bits;
  }
}

static void drawIconTile(U8G2 *oled, int x, int y,
                         const unsigned char *iconBits, bool filled) {
  drawFocusFrame(oled, x, y, ICON_TILE, ICON_TILE, filled);
  if (filled) {
    oled->drawBox(x + 1, y + 1, ICON_TILE - 2, ICON_TILE - 2);
    oled->setDrawColor(0);
  }
  oled->drawXBMP(x + 2, y + 2, ICON_SIZE, ICON_SIZE, iconBits);
  if (filled) {
    oled->setDrawColor(1);
  }
}

static void drawIconAccent(U8G2 *oled, SettingsScreen screen, int x, int y,
                           uint8_t battery) {
  const uint8_t phase = shimmer(4, 220);
  if (screen == SettingsScreen::AERATION_CO2) {
    oled->drawPixel(x + 16, y + 16 - phase);
    oled->drawPixel(x + 14, y + 12 - ((phase + 2) % 4));
  } else if (screen == SettingsScreen::FEEDING) {
    oled->drawPixel(x + 15 + (phase % 2), y + 4 + phase);
  } else if (screen == SettingsScreen::WIFI) {
    oled->drawHLine(x + 5, y + 17, 2 + phase);
  } else if (screen == SettingsScreen::DISPLAY_AND_POWER && battery < 20 &&
             editCursorVisible()) {
    oled->drawFrame(x - 1, y - 1, ICON_TILE + 2, ICON_TILE + 2);
  }
}

static void drawNavRail(U8G2 *oled, bool btnBack, bool btnSelect,
                        bool btnNext) {
  oled->drawLine(NAV_X, 0, NAV_X, OLED_H - 1);
  oled->drawLine(NAV_X + 1, 10, OLED_W - 1, 10);
  oled->drawLine(NAV_X + 1, 21, OLED_W - 1, 21);
  oled->drawLine(OLED_W - 1, 0, OLED_W - 1, OLED_H - 1);

  if (!btnBack) {
    oled->drawXBMP(117, 1, 10, 8, image_Pin_back_arrow_bits);
  }
  if (!btnSelect) {
    oled->drawXBMP(117, 13, 10, 7, image_MenuCheck_bits);
  }
  if (!btnNext) {
    oled->drawXBMP(120, 24, 5, 7, image_arrow_down_bits);
  }
}

static void editField(U8G2 *oled, int x, int y, int w, int h, const char *text,
                      bool selected, bool editing) {
  if (oled == nullptr) {
    return;
  }

  if (selected) {
    drawFocusFrame(oled, x, y, w, h, editing);
    if (!editing) {
      oled->drawPixel(x + 1, y + 1);
      oled->drawPixel(x + w - 2, y + h - 2);
    }
  }

  if (text != nullptr) {
    oled->drawStr(x + 3, y + h - 2, text);
  }

  if (editing) {
    const int textWidth = text != nullptr ? oled->getStrWidth(text) : 0;
    const int cursorX = min(x + w - 3, x + 4 + textWidth);
    if (editCursorVisible()) {
      oled->drawVLine(cursorX, y + 3, max(2, h - 6));
      oled->drawPixel(cursorX + 1, y + h - 3);
    } else {
      oled->drawPixel(cursorX, y + h - 3);
    }
  }
}

static void drawModeBadge(U8G2 *oled, int x, int y, int w, const char *label,
                          bool selected, bool editing) {
  const bool filled = selected && !editing;
  if (filled) {
    oled->drawBox(x, y, w, 11);
    oled->setDrawColor(0);
  } else {
    drawFocusFrame(oled, x, y, w, 11, selected);
  }

  oled->setFont(u8g2_font_5x7_tr);
  drawCenteredText(oled, x, y + 8, w, label);

  if (filled) {
    oled->setDrawColor(1);
  }
  if (editing && editCursorVisible()) {
    oled->drawHLine(x + 4, y + 10, max(4, w - 8));
  }
}

static void drawValueBar(U8G2 *oled, int x, int y, int w, uint8_t percent,
                         bool pulse) {
  if (percent > 100) {
    percent = 100;
  }
  oled->drawHLine(x, y + 1, w);
  oled->drawVLine(x, y, 3);
  oled->drawVLine(x + w - 1, y, 3);
  const uint8_t fill = static_cast<uint8_t>((static_cast<uint16_t>(w) * percent) /
                                            100U);
  if (fill > 0 && (!pulse || editCursorVisible())) {
    oled->drawBox(x, y, max(1, static_cast<int>(fill)), 3);
  }
}

static void drawTimelineSegment(U8G2 *oled, int x, int y, int w,
                                uint16_t startMinute, uint16_t endMinute) {
  const int x0 = x + static_cast<int>((static_cast<uint32_t>(startMinute) * w) /
                                      1440UL);
  const int x1 = x + static_cast<int>((static_cast<uint32_t>(endMinute) * w) /
                                      1440UL);
  const int segmentWidth = max(1, x1 - x0);
  oled->drawBox(x0, y, segmentWidth, 3);
}

static void drawTimeline(U8G2 *oled, int x, int y, int w,
                         const ScheduleCard &card) {
  oled->drawHLine(x, y + 1, w);
  oled->drawVLine(x, y, 3);
  oled->drawVLine(x + (w / 2), y, 3);
  oled->drawVLine(x + w - 1, y, 3);

  if (card.mode == static_cast<uint8_t>(ScheduleMode::AlwaysOn)) {
    oled->drawBox(x, y, w, 3);
    return;
  }

  if (card.mode == static_cast<uint8_t>(ScheduleMode::AlwaysOff)) {
    return;
  }

  const uint16_t start = minuteOfDay(card.startHour, card.startMinute);
  if (card.pointOnly) {
    const int markerX =
        x + static_cast<int>((static_cast<uint32_t>(start) * w) / 1440UL);
    oled->drawBox(constrain(markerX - 1, x, x + w - 2), y - 1, 3, 5);
    return;
  }

  const uint16_t end = minuteOfDay(card.endHour, card.endMinute);
  if (end >= start) {
    drawTimelineSegment(oled, x, y, w, start, end);
  } else {
    drawTimelineSegment(oled, x, y, w, start, 1440);
    drawTimelineSegment(oled, x, y, w, 0, end);
  }
}

static uint8_t feedCardMode(uint8_t freq) {
  return freq == 0 ? static_cast<uint8_t>(ScheduleMode::AlwaysOff)
                   : static_cast<uint8_t>(ScheduleMode::Schedule);
}

static uint8_t shownCardMode(const ScheduleCard &card, uint8_t index,
                             bool editing, uint8_t editModeValue) {
  if (!editing) {
    return card.mode;
  }
  if (index == 3) {
    return editModeValue == 2 ? static_cast<uint8_t>(ScheduleMode::AlwaysOff)
                              : static_cast<uint8_t>(ScheduleMode::Schedule);
  }
  return editModeValue;
}

static const char *shownCardModeLabel(const ScheduleCard &card, uint8_t index,
                                      bool editing, uint8_t editModeValue,
                                      uint8_t feedingFrequency) {
  if (index == 3) {
    return editing ? feedQuickEditLabel(editModeValue)
                   : feedQuickModeLabel(feedingFrequency);
  }
  return modeShortLabel(shownCardMode(card, index, editing, editModeValue));
}

static void formatScheduleDetail(char *out, size_t outSize,
                                 const ScheduleCard &card, uint8_t index,
                                 bool editing, uint8_t editModeValue,
                                 uint8_t feedingFrequency,
                                 uint8_t feedingHour, uint8_t feedingMinute) {
  if (outSize == 0U) {
    return;
  }

  const uint8_t mode = shownCardMode(card, index, editing, editModeValue);
  if (mode == static_cast<uint8_t>(ScheduleMode::AlwaysOff)) {
    snprintf(out, outSize, "wylaczone");
    return;
  }
  if (mode == static_cast<uint8_t>(ScheduleMode::AlwaysOn)) {
    snprintf(out, outSize, "caly dzien");
    return;
  }

  if (index == 3) {
    const uint8_t shownFreq =
        editing && editModeValue == 1 ? 1 : feedingFrequency;
    const char *freq = shownFreq <= 1 ? "codz."
                       : (shownFreq == 2 ? "co 2d" : "co 3d");
    snprintf(out, outSize, "%02u:%02u %s",
             static_cast<unsigned>(feedingHour),
             static_cast<unsigned>(feedingMinute), freq);
    return;
  }

  snprintf(out, outSize, "%02u:%02u-%02u:%02u",
           static_cast<unsigned>(card.startHour),
           static_cast<unsigned>(card.startMinute),
           static_cast<unsigned>(card.endHour),
           static_cast<unsigned>(card.endMinute));
}

static void drawScheduleTypeIcon(U8G2 *oled, int x, int y, char label) {
  drawIconTile(oled, x, y, iconForScheduleLabel(label), false);

  char labelText[2] = {label, '\0'};
  oled->setFont(u8g2_font_4x6_tr);
  oled->drawStr(x + 8, y + 19, labelText);

  const uint8_t phase = shimmer(4, 180);
  if (label == 'A') {
    oled->drawPixel(x + 17, y + 15 - phase);
    oled->drawPixel(x + 15, y + 10 - ((phase + 2) % 4));
  } else if (label == 'K') {
    oled->drawPixel(x + 15 + (phase % 2), y + 4 + phase);
  } else if (label == 'L' && editCursorVisible()) {
    oled->drawPixel(x + 10, y + 1);
    oled->drawPixel(x + 18, y + 10);
  }
}

static void drawCardPositionDots(U8G2 *oled, uint8_t selected) {
  for (uint8_t i = 0; i < 4; ++i) {
    const int x = 92 + (i * 5);
    if (i == selected) {
      oled->drawDisc(x, 4, 1, U8G2_DRAW_ALL);
    } else {
      oled->drawPixel(x, 4);
    }
  }
}

static void drawHeader(U8G2 *oled, const char *title) {
  oled->setFont(u8g2_font_5x7_tr);
  oled->drawStr(2, 7, title);
  oled->drawHLine(0, 9, CONTENT_W - 7);
  oled->drawPixel(CONTENT_W - 4 + shimmer(4, 180), 9);
}

static void drawDetailShell(U8G2 *oled, SettingsScreen screen,
                            const char *title, uint8_t battery) {
  drawHeader(oled, title);
  drawFocusFrame(oled, 0, 10, CONTENT_W, 22, true);
  drawIconTile(oled, 2, 11, iconForSettingsScreen(screen), false);
  drawIconAccent(oled, screen, 2, 11, battery);
  oled->drawVLine(24, 12, 18);
}

} // namespace

void SettingsRenderer::drawSettingsMenu(AquariumAnimation *ctx, bool btnBack,
                                        bool btnSelect, bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawHeader(display, "Ustawienia");

  const uint8_t itemIndex = menuSelection % SETTINGS_MENU_COUNT;
  const SettingsMenuItem &item = SETTINGS_MENU_ITEMS[itemIndex];
  drawFocusFrame(display, 0, 10, CONTENT_W, 22, true);
  drawIconTile(display, 3, 11, iconForSettingsScreen(item.screen), true);
  drawIconAccent(display, item.screen, 3, 11, batteryPercent);

  display->setFont(useCompactMenuFont(item.screen) ? u8g2_font_4x6_tr
                                                   : u8g2_font_5x7_tr);
  display->drawStr(28, 18, item.label);
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(28, 27, item.hint);

  char indexBuf[6];
  snprintf(indexBuf, sizeof(indexBuf), "%u/%u", static_cast<unsigned>(itemIndex + 1),
           static_cast<unsigned>(SETTINGS_MENU_COUNT));
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(95, 7, indexBuf);

  const uint8_t scrollTrackH = 17;
  const uint8_t thumbY =
      12 + ((scrollTrackH - 4) * itemIndex) / (SETTINGS_MENU_COUNT - 1);
  display->drawVLine(111, 12, scrollTrackH);
  display->drawBox(110, thumbY, 3, 4);

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawUnifiedSchedules(AquariumAnimation *ctx,
                                            bool btnBack, bool btnSelect,
                                            bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);

  const ScheduleCard cards[] = {
      {'L', "Swi.", lightMode, scheduleHourOn, scheduleMinOn,
       scheduleHourOff, scheduleMinOff, false},
      {'A', "Nap.", aerationMode, aerationHourOn, aerationMinOn,
       aerationHourOff, aerationMinOff, false},
      {'F', "Filtr", filterMode, filterHourOn, filterMinOn, filterHourOff,
       filterMinOff, false},
      {'K', "Karm.", feedCardMode(feedFreq), feedHour, feedMinute, feedHour,
       feedMinute, true}};

  const uint8_t cardIndex = constrain(scheduleSelection, 0, 3);
  const ScheduleCard &card = cards[cardIndex];
  const bool editing = isEditing;
  const uint8_t modeForTimeline =
      shownCardMode(card, cardIndex, editing, tempHour);
  ScheduleCard shownCard = card;
  shownCard.mode = modeForTimeline;

  char detail[18];
  formatScheduleDetail(detail, sizeof(detail), card, cardIndex, editing,
                       tempHour, feedFreq, feedHour, feedMinute);

  drawHeader(display, "Harmonogramy");
  drawCardPositionDots(display, cardIndex);
  drawFocusFrame(display, 0, 10, CONTENT_W, 22, true);
  drawScheduleTypeIcon(display, 2, 11, card.label);
  display->drawVLine(24, 12, 18);

  display->setFont(u8g2_font_5x7_tr);
  display->drawStr(28, 17, card.title);
  display->drawStr(58, 17, detail);

  drawModeBadge(display, 28, 20, 28,
                shownCardModeLabel(card, cardIndex, editing, tempHour, feedFreq),
                true, editing);

  drawTimeline(display, 61, 27, 49, shownCard);
  const uint16_t nowMinute = minuteOfDay(currentHour, currentMinute);
  const int nowX =
      61 + static_cast<int>((static_cast<uint32_t>(nowMinute) * 49) / 1440UL);
  display->drawVLine(constrain(nowX, 61, 109), 25, 5);

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawTemperatureAndHeater(AquariumAnimation *ctx,
                                                bool btnBack, bool btnSelect,
                                                bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawDetailShell(display, SettingsScreen::TEMPERATURE_AND_HEATER, "Temperatura",
                  batteryPercent);

  const uint8_t shownTarget =
      isEditing ? tempHour
                : (heaterMode == static_cast<uint8_t>(HeaterMode::Off)
                       ? 0
                       : targetTemp);
  char targetBuf[12];
  if (shownTarget == 0) {
    snprintf(targetBuf, sizeof(targetBuf), "OFF");
  } else {
    snprintf(targetBuf, sizeof(targetBuf), "%u*C",
             static_cast<unsigned>(shownTarget));
  }

  drawModeBadge(display, 28, 12, 37, targetBuf, true, isEditing);
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(69, 17, heaterMode == static_cast<uint8_t>(HeaterMode::Off)
                              ? "grzalka OFF"
                              : "prog grzania");
  const uint8_t targetPct =
      shownTarget == 0 ? 0 : static_cast<uint8_t>(((shownTarget - 18U) * 100U) / 12U);
  drawValueBar(display, 28, 27, 82, targetPct, false);

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawAerationCo2(AquariumAnimation *ctx, bool btnBack,
                                       bool btnSelect, bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawDetailShell(display, SettingsScreen::AERATION_CO2, "Napow. / CO2",
                  batteryPercent);

  char onBuf[8];
  char offBuf[8];
  formatTime(onBuf, sizeof(onBuf), aerationHourOn, aerationMinOn);
  formatTime(offBuf, sizeof(offBuf), aerationHourOff, aerationMinOff);

  display->setFont(u8g2_font_4x6_tr);
  drawModeBadge(display, 28, 12, 29,
                modeShortLabel(isEditing && scheduleSelection == 0 ? tempHour
                                                                    : aerationMode),
                scheduleSelection == 0, isEditing && scheduleSelection == 0);
  editField(display, 60, 12, 26, 8, onBuf, scheduleSelection == 1,
            isEditing && scheduleSelection == 1);
  editField(display, 87, 12, 27, 8, offBuf, scheduleSelection == 2,
            isEditing && scheduleSelection == 2);

  char servoBuf[18];
  snprintf(servoBuf, sizeof(servoBuf), "CO2 %u%%",
           static_cast<unsigned>(aerationPercent));
  display->drawStr(28, 27, servoBuf);
  drawValueBar(display, 63, 26, 47, aerationPercent, false);

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawFeeding(AquariumAnimation *ctx, bool btnBack,
                                   bool btnSelect, bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawDetailShell(display, SettingsScreen::FEEDING, "Karmienie", batteryPercent);

  char timeBuf[8];
  if (isEditing && scheduleSelection == 0) {
    formatTime(timeBuf, sizeof(timeBuf), tempHour, tempMinute);
  } else {
    formatTime(timeBuf, sizeof(timeBuf), feedHour, feedMinute);
  }

  display->setFont(u8g2_font_5x7_tr);
  editField(display, 28, 12, 35, 11, timeBuf, scheduleSelection == 0,
            isEditing && scheduleSelection == 0);

  const uint8_t shownFreq =
      isEditing && scheduleSelection == 1 ? tempHour : feedFreq;
  display->setFont(u8g2_font_5x7_tr);
  editField(display, 66, 12, 47, 11, feedFreqLabel(shownFreq),
            scheduleSelection == 1, isEditing && scheduleSelection == 1);
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(28, 30, shownFreq == 0 ? "pauza" : "automatyczne karmienie");

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawWifi(AquariumAnimation *ctx, const char *modeLabel,
                                const char *primaryLine,
                                const char *secondaryLine, const char *ip,
                                uint8_t clients) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawHeader(display, modeLabel != nullptr ? modeLabel : "WiFi");
  drawFocusFrame(display, 0, 10, CONTENT_W, 22, true);
  drawIconTile(display, 2, 11, icon_ui_wifi_16_bits, false);
  drawIconAccent(display, SettingsScreen::WIFI, 2, 11, batteryPercent);
  display->drawVLine(24, 12, 18);
  display->drawVLine(NAV_X, 0, OLED_H - 1);
  display->drawLine(OLED_W - 1, 0, OLED_W - 1, OLED_H - 1);

  const uint8_t phase = (millis() / 350UL) % 4UL;
  for (uint8_t b = 0; b < 3; ++b) {
    const uint8_t bx = 94 + (b * 5);
    const uint8_t bh = 3 + (b * 3);
    const uint8_t by = 18 - bh;
    if (b <= phase || phase == 3) {
      display->drawBox(bx, by, 4, bh);
    } else {
      display->drawFrame(bx, by, 4, bh);
    }
  }

  char ipBuf[18];
  snprintf(ipBuf, sizeof(ipBuf), "%.15s", ip != nullptr ? ip : "");
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(28, 17, ipBuf);

  char clientsBuf[6];
  snprintf(clientsBuf, sizeof(clientsBuf), "[%u]",
           static_cast<unsigned>(clients));
  display->drawStr(101, 7, clientsBuf);

  char primaryBuf[30];
  char secondaryBuf[30];
  snprintf(primaryBuf, sizeof(primaryBuf), "%.20s",
           primaryLine != nullptr ? primaryLine : "");
  snprintf(secondaryBuf, sizeof(secondaryBuf), "%.18s",
           secondaryLine != nullptr ? secondaryLine : "");
  display->drawStr(28, 25, primaryBuf);
  display->drawStr(28, 31, secondaryBuf);
}

void SettingsRenderer::drawDisplayAndPower(AquariumAnimation *ctx,
                                           bool btnBack, bool btnSelect,
                                           bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawDetailShell(display, SettingsScreen::DISPLAY_AND_POWER, "Ekran i energia",
                  batteryPercent);

  display->setFont(u8g2_font_5x7_tr);
  editField(display, 28, 12, 42, 10, "OLED auto", scheduleSelection == 0,
            isEditing && scheduleSelection == 0);

  char batBuf[18];
  snprintf(batBuf, sizeof(batBuf), "%u%% %s",
           static_cast<unsigned>(batteryPercent), batteryVoltageBuffer);
  display->setFont(u8g2_font_4x6_tr);
  editField(display, 73, 12, 40, 10, batBuf, scheduleSelection == 1,
            isEditing && scheduleSelection == 1);

  drawValueBar(display, 28, 27, 82, batteryPercent, batteryPercent < 20);

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

void SettingsRenderer::drawSystem(AquariumAnimation *ctx, bool btnBack,
                                  bool btnSelect, bool btnNext) {
  if (!display) {
    return;
  }

  prepareDisplay(display);
  drawDetailShell(display, SettingsScreen::SYSTEM,
                  isEditing && scheduleSelection == SYSTEM_ITEM_FACTORY_RESET
                      ? "Trzymaj select"
                      : (isEditing ? "Potwierdz" : "System"),
                  batteryPercent);

  static const char *const items[] = {"Info", "Logi", "Restart", "Fabryka"};
  for (uint8_t i = 0; i < 4; ++i) {
    const int x = (i % 2) == 0 ? 28 : 72;
    const int y = (i < 2) ? 13 : 24;
    const bool selected = scheduleSelection == i;
    display->setFont(i == 3 ? u8g2_font_4x6_tr : u8g2_font_5x7_tr);
    editField(display, x, y - 7, i == 3 ? 41 : 39, 9, items[i], selected,
              selected && isEditing);
  }

  drawNavRail(display, btnBack, btnSelect, btnNext);
}

#undef display
#undef menuSelection
#undef menuScrollOffset
#undef scheduleSelection
#undef isEditing
#undef editState
#undef tempHour
#undef tempMinute
#undef lightMode
#undef aerationMode
#undef filterMode
#undef heaterMode
#undef targetTemp
#undef scheduleHourOn
#undef scheduleMinOn
#undef scheduleHourOff
#undef scheduleMinOff
#undef aerationHourOn
#undef aerationMinOn
#undef aerationHourOff
#undef aerationMinOff
#undef filterHourOn
#undef filterMinOn
#undef filterHourOff
#undef filterMinOff
#undef feedHour
#undef feedMinute
#undef feedFreq
#undef aerationPercent
#undef batteryPercent
#undef batteryVoltageBuffer
