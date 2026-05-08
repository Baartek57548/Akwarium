#ifndef SETTINGS_RENDERER_H
#define SETTINGS_RENDERER_H

#include <Arduino.h>

class AquariumAnimation;

class SettingsRenderer {
public:
  static void drawSettingsMenu(AquariumAnimation *ctx, bool btnBack,
                               bool btnSelect, bool btnNext);
  static void drawUnifiedSchedules(AquariumAnimation *ctx, bool btnBack,
                                   bool btnSelect, bool btnNext);
  static void drawTemperatureAndHeater(AquariumAnimation *ctx, bool btnBack,
                                       bool btnSelect, bool btnNext);
  static void drawAerationCo2(AquariumAnimation *ctx, bool btnBack,
                              bool btnSelect, bool btnNext);
  static void drawFeeding(AquariumAnimation *ctx, bool btnBack, bool btnSelect,
                          bool btnNext);
  static void drawWifi(AquariumAnimation *ctx, const char *modeLabel,
                       const char *primaryLine, const char *secondaryLine,
                       const char *ip, uint8_t clients);
  static void drawDisplayAndPower(AquariumAnimation *ctx, bool btnBack,
                                  bool btnSelect, bool btnNext);
  static void drawSystem(AquariumAnimation *ctx, bool btnBack, bool btnSelect,
                         bool btnNext);
};

#endif // SETTINGS_RENDERER_H
