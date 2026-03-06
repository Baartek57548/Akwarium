#ifndef UI_RENDERERS_H
#define UI_RENDERERS_H

#include <Arduino.h>
#include <U8g2lib.h>

class AquariumAnimation;

class HomeRenderer {
public:
    static void drawFrame(AquariumAnimation* ctx);
    static void drawFeedingScreen(AquariumAnimation* ctx);
};

class MenuRenderer {
public:
    static void drawMenu(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawLogs(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
};

class ScheduleRenderer {
public:
    static void drawSchedule(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawScheduleAeration(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawScheduleCO2(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawScheduleTemp(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawScheduleFeeding(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
    static void drawSettingsDateTime(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
};

class TestRenderer {
public:
    static void drawTests(AquariumAnimation* ctx, bool btnBack, bool btnSelect, bool btnNext);
};

class OtaRenderer {
public:
    static void drawAccessPointScreen(AquariumAnimation* ctx, const char *apName, const char *apPass, const char *ip, uint8_t clients);
};

#endif // UI_RENDERERS_H
