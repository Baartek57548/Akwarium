#include "AquariumAnimation.h"
#include "UIRenderers.h"
#include <Arduino.h>

// ==========================================
// 1. BITMAPY - EKRAN GĹĂ“WNY (ZAKTUALIZOWANE)
// ==========================================
static const unsigned char image_Layer_10_bits[] U8X8_PROGMEM = {};
static const unsigned char image_Layer_19_1_bits[] U8X8_PROGMEM = {
    0x7c, 0x00, 0x82, 0x00, 0x21, 0x01, 0x41, 0x01, 0x01, 0x01, 0x82,
    0x00, 0x44, 0x00, 0x44, 0x00, 0x38, 0x00, 0x28, 0x00, 0x38, 0x00};
// ZAKTUALIZOWANA BITMAPA (CO2/BÄ…belki?)
static const unsigned char image_Layer_19_2_bits[] U8X8_PROGMEM = {
    0x00, 0xe0, 0x00, 0xc2, 0x90, 0x09, 0xa5, 0x11, 0x15, 0x22, 0xe1, 0x08,
    0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x68, 0x08, 0x70,
    0x48, 0x14, 0xa8, 0x30, 0x08, 0xc8, 0x00, 0x00, 0x88, 0x00, 0x00, 0x70};
static const unsigned char image_Layer_19_bits[] U8X8_PROGMEM = {
    0x08, 0x14, 0x14, 0x22, 0x22, 0x51, 0x51, 0x41, 0x22, 0x1c};
static const unsigned char image_Layer_20_bits[] U8X8_PROGMEM = {
    0x1c, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1c};
static const unsigned char image_Layer_21_bits[] U8X8_PROGMEM = {
    0x10, 0x00, 0x28, 0x00, 0x44, 0x00, 0x44, 0x00, 0x46, 0x00,
    0x44, 0x00, 0x56, 0x00, 0x54, 0x00, 0x56, 0x00, 0x54, 0x00,
    0x54, 0x00, 0x54, 0x00, 0x54, 0x00, 0x92, 0x00, 0x39, 0x01,
    0x5d, 0x01, 0x7d, 0x01, 0x39, 0x01, 0x82, 0x00, 0x7c, 0x00};
static const unsigned char image_Layer_22_bits[] U8X8_PROGMEM = {
    0xf8, 0x00, 0x04, 0x01, 0x02, 0x02, 0x01, 0x04, 0x01, 0x04, 0x01,
    0x04, 0x01, 0x04, 0x01, 0x04, 0x02, 0x02, 0x04, 0x01, 0xf8, 0x00};
// ZAKTUALIZOWANA BITMAPA (Rybki?)
static const unsigned char image_Layer_24_bits[] U8X8_PROGMEM = {
    0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x26, 0x00, 0x7c, 0x00, 0x00, 0x80, 0x13, 0x00, 0x47,
    0x00, 0x00, 0xf9, 0x10, 0x8e, 0x21, 0x00, 0x00, 0x0e, 0xe0, 0xf1,
    0xf0, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00};
// ZAKTUALIZOWANA BITMAPA (Bateria?)
static const unsigned char image_Layer_25_bits[] U8X8_PROGMEM = {
    0x38, 0x00, 0x44, 0x00, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0x01};
// NOWA BITMAPA (Dno/Kamienie)
static const unsigned char image_Layer_29_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x01, 0x80, 0x17, 0x00, 0x00, 0x3a, 0x40,
    0x0f, 0x1d, 0x00, 0x6e, 0x80, 0x17, 0x2e, 0x00, 0x3a,
    0x00, 0x00, 0x1d, 0x00, 0x01, 0x00, 0x00};

static const unsigned char image_Circle_Filled_bits[] U8X8_PROGMEM = {
    0x1c, 0x3e, 0x7f, 0x7f, 0x7f, 0x3e, 0x1c};
static const unsigned char image_Split_2_bits[] U8X8_PROGMEM = {
    0x01, 0x01, 0x01, 0x01, 0x01};
static const unsigned char image_Split_3_bits[] U8X8_PROGMEM = {
    0x41, 0x41, 0x41, 0x41, 0x41, 0x40};
static const unsigned char image_SmallFish_bits[] U8X8_PROGMEM = {0x18, 0x1e,
                                                                  0x18};

// ==========================================
// 2. BITMAPY - MENU I NAWIGACJA
// ==========================================
static const unsigned char image_arrow_down_bits[] U8X8_PROGMEM = {
    0x04, 0x04, 0x04, 0x04, 0x15, 0x0e, 0x04};
static const unsigned char image_Pin_back_arrow_bits[] U8X8_PROGMEM = {
    0x04, 0x00, 0x06, 0x00, 0xff, 0x00, 0x06, 0x01,
    0x04, 0x02, 0x00, 0x02, 0x00, 0x01, 0xf8, 0x00};
static const unsigned char image_ButtonLeft_bits[] U8X8_PROGMEM = {
    0x08, 0x0c, 0x0e, 0x0f, 0x0e, 0x0c, 0x08};
static const unsigned char image_ButtonLeft_copy_bits[] U8X8_PROGMEM = {
    0x08, 0x0c, 0x0e, 0x0f, 0x0e, 0x0c, 0x08};
static const unsigned char image_MenuCheck_bits[] U8X8_PROGMEM = {
    0x00, 0x03, 0x80, 0x01, 0xc1, 0x00, 0x63,
    0x00, 0x36, 0x00, 0x1c, 0x00, 0x08, 0x00};
static const unsigned char image_Layer_11_bits[] U8X8_PROGMEM = {
    0x00, 0x03, 0x80, 0x01, 0xc1, 0x00, 0x63,
    0x00, 0x36, 0x00, 0x1c, 0x00, 0x08, 0x00};
static const unsigned char image_Layer_15_check_bits[] U8X8_PROGMEM = {
    0x00, 0x03, 0x80, 0x01, 0xc1, 0x00, 0x63,
    0x00, 0x36, 0x00, 0x1c, 0x00, 0x08, 0x00};

// ==========================================
// 3. BITMAPY - HARMONOGRAMY
// ==========================================
static const unsigned char image_Layer_4_1_bits[] U8X8_PROGMEM = {
    0x78, 0x00, 0x86, 0x01, 0x31, 0x02, 0xfd, 0x02, 0xfd, 0x02,
    0x79, 0x02, 0x32, 0x01, 0xb4, 0x00, 0x84, 0x00, 0x48, 0x00,
    0x48, 0x00, 0x78, 0x00, 0x48, 0x00, 0x78, 0x00};
static const unsigned char image_Layer_4_copy_bits[] U8X8_PROGMEM = {
    0x78, 0x00, 0x86, 0x01, 0x01, 0x02, 0x41, 0x02, 0x81, 0x02,
    0x01, 0x02, 0x02, 0x01, 0x84, 0x00, 0x84, 0x00, 0x48, 0x00,
    0x48, 0x00, 0x78, 0x00, 0x48, 0x00, 0x78, 0x00};
static const unsigned char image_weather_humidity_1_bits[] U8X8_PROGMEM = {
    0x20, 0x00, 0x30, 0x00, 0x70, 0x00, 0x78, 0x00, 0xf8, 0x00,
    0xfc, 0x01, 0xfc, 0x01, 0x7e, 0x03, 0xfe, 0x02, 0xff, 0x06,
    0xff, 0x07, 0xfe, 0x03, 0xfe, 0x01, 0xf8, 0x00};
static const unsigned char image_weather_humidity_white_bits[] U8X8_PROGMEM = {
    0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x48, 0x00, 0x88, 0x00,
    0x04, 0x01, 0x04, 0x01, 0x82, 0x02, 0x02, 0x03, 0x01, 0x05,
    0x01, 0x04, 0x01, 0x02, 0x06, 0x02, 0xf8, 0x01};
static const unsigned char image_Layer_15_bits[] U8X8_PROGMEM = {};
static const unsigned char image_weather_temperature_bits[] U8X8_PROGMEM = {
    0x30, 0x00, 0x48, 0x00, 0x84, 0x00, 0xa4, 0x00, 0xb4, 0x00, 0xb4, 0x01,
    0xb4, 0x00, 0xb4, 0x01, 0xb4, 0x00, 0xb4, 0x01, 0xb4, 0x00, 0xb4, 0x00,
    0xb4, 0x00, 0xb4, 0x00, 0xb4, 0x00, 0x32, 0x01, 0x69, 0x12, 0xf5, 0x02,
    0xfd, 0x02, 0xfd, 0x02, 0x79, 0x02, 0x02, 0x01, 0xfc, 0x00};
static const unsigned char image_Layer_1_bits[] U8X8_PROGMEM = {
    0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00,
    0xec, 0x03, 0x00, 0x00, 0xfc, 0x05, 0x01, 0x00, 0xec, 0x13, 0x00, 0x00,
    0x26, 0x00, 0x00, 0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x3a, 0x02, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
    0x2e, 0x00, 0x78, 0x00, 0x1d, 0x40, 0x7c, 0x00, 0x00, 0x00, 0x74, 0x01,
    0x00, 0x00, 0xfe, 0x00, 0x20, 0x00, 0x7c, 0x01};

// ==========================================
// 4. BITMAPY - LOGI I DEBUG
// ==========================================
static const unsigned char image_schedule_aeration_off_bits[] U8X8_PROGMEM = {
    0x00, 0x1e, 0xff, 0x61, 0x00, 0x00, 0x0e, 0x38, 0xc0, 0x01,
    0x00, 0x00, 0x18, 0x1c, 0x00, 0x00, 0x00, 0x03, 0x1c, 0x30};
static const unsigned char image_schedule_aeration_wind_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xc0, 0x11, 0x20, 0x22, 0x20,
    0x22, 0x00, 0x22, 0x00, 0x11, 0xff, 0x4c, 0x00, 0x00, 0xb5, 0x41,
    0x00, 0x06, 0x00, 0x08, 0x00, 0x08, 0x80, 0x04, 0x00, 0x03};

static const unsigned char image_ButtonRightSmall_copy_bits[] U8X8_PROGMEM = {
    0x01, 0x03, 0x07, 0x03, 0x01};
static const unsigned char image_operation_warning_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x80, 0x01, 0x40, 0x02, 0x40, 0x02, 0x20, 0x04, 0x90,
    0x09, 0x90, 0x09, 0x88, 0x11, 0x88, 0x11, 0x84, 0x21, 0x02, 0x40,
    0x82, 0x41, 0x81, 0x81, 0x01, 0x80, 0xfe, 0x7f, 0x00, 0x00};

// ==========================================
// 5. BITMAPY - DATA I CZAS
// ==========================================
static const unsigned char image_clock_quarters_bits[] U8X8_PROGMEM = {
    0xe0, 0x03, 0x98, 0x0c, 0x84, 0x10, 0x02, 0x20, 0x82, 0x20, 0x81,
    0x40, 0x81, 0x40, 0x87, 0x70, 0x01, 0x41, 0x01, 0x42, 0x02, 0x20,
    0x02, 0x20, 0x84, 0x10, 0x98, 0x0c, 0xe0, 0x03, 0x00, 0x00};
static const unsigned char image_date_day_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x90, 0x04, 0xfe, 0x3f, 0x93, 0x64, 0x01, 0x40, 0x31,
    0x44, 0x49, 0x46, 0x49, 0x45, 0x41, 0x44, 0x21, 0x44, 0x11, 0x44,
    0x09, 0x44, 0x79, 0x4f, 0x03, 0x60, 0xfe, 0x3f, 0x00, 0x00};
static const unsigned char image_Layer_12_bits[] U8X8_PROGMEM = {
    0x00, 0x03, 0x80, 0x01, 0xc1, 0x00, 0x63,
    0x00, 0x36, 0x00, 0x1c, 0x00, 0x08, 0x00};

// ==========================================
// 6. BITMAPY - TESTY
// ==========================================
static const unsigned char image_test_light_bits[] U8X8_PROGMEM = {
    0x04, 0x20, 0xc8, 0x13, 0x20, 0x04, 0x10, 0x08, 0x95, 0xa8, 0x90,
    0x09, 0x90, 0x08, 0x24, 0x24, 0x42, 0x42, 0x80, 0x00, 0xc0, 0x03,
    0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00};
static const unsigned char image_test_humidity_white_bits[] U8X8_PROGMEM = {
    0x20, 0x00, 0x20, 0x00, 0x50, 0x00, 0x48, 0x00, 0x88, 0x00,
    0x84, 0x00, 0x04, 0x01, 0x42, 0x02, 0x82, 0x02, 0x81, 0x04,
    0x01, 0x04, 0x02, 0x02, 0x02, 0x02, 0x0c, 0x01, 0xf0, 0x00};
static const unsigned char image_test_temperature_bits[] U8X8_PROGMEM = {
    0x38, 0x40, 0x44, 0xa0, 0x54, 0x40, 0xd4, 0x1c, 0x54, 0x06,
    0xd4, 0x02, 0x54, 0x02, 0x54, 0x06, 0x92, 0x1c, 0x39, 0x01,
    0x75, 0x01, 0x7d, 0x01, 0x39, 0x01, 0x82, 0x00, 0x7c, 0x00};
static const unsigned char image_test_wind_bits[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xc0, 0x11, 0x20, 0x22, 0x20,
    0x22, 0x00, 0x22, 0x00, 0x11, 0xff, 0x4c, 0x00, 0x00, 0xb5, 0x41,
    0x00, 0x06, 0x00, 0x08, 0x00, 0x08, 0x80, 0x04, 0x00, 0x03};

// ==========================================
// 7. ANIMACJA: ZAPIS (SAVE CHANGES)
// ==========================================
const byte PROGMEM frames_save[][132] = {
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   15,  255, 255,
     0,   24, 192, 2,   128, 24,  192, 50,  64,  24,  192, 114, 32,  24,  192,
     114, 16, 24,  192, 114, 24,  24,  192, 114, 8,   24,  192, 2,   8,   24,
     112, 6,  8,   16,  63,  248, 8,   16,  0,   0,   8,   16,  0,   0,   8,
     16,  0,  0,   8,   16,  127, 254, 8,   16,  192, 2,   8,   16,  192, 3,
     8,   16, 192, 3,   24,  16,  192, 3,   24,  16,  192, 3,   24,  16,  192,
     3,   24, 16,  192, 3,   24,  16,  192, 3,   24,  16,  192, 3,   24,  18,
     192, 3,  88,  16,  192, 3,   24,  15,  255, 255, 240, 7,   255, 255, 224,
     0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,  15, 255, 255, 0,
     8,  64,  3,   128, 8,  64,  50,  192, 8,  64,  114, 96, 8,  64,  114, 48,
     8,  64,  114, 16,  8,  64,  114, 16,  8,  64,  2,   16, 8,  96,  6,   16,
     8,  63,  248, 16,  8,  0,   0,   16,  8,  0,   0,   16, 8,  0,   0,   16,
     8,  127, 254, 16,  8,  64,  2,   16,  8,  64,  2,   16, 8,  64,  2,   16,
     8,  64,  2,   16,  8,  192, 2,   16,  24, 192, 2,   16, 24, 192, 2,   16,
     24, 192, 2,   16,  26, 192, 2,   80,  8,  192, 2,   16, 15, 255, 255, 240,
     0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,  0,  0,   0,   0},
    {0, 0,   0,   0,  0, 0,   0,  0,   0, 0,   0,   0,   7,  255, 254, 0,
     8, 96,  3,   0,  8, 96,  3,  128, 8, 96,  58,  64,  8,  96,  42,  32,
     8, 96,  42,  16, 8, 64,  58, 16,  8, 64,  2,   16,  8,  32,  2,   16,
     8, 31,  252, 16, 8, 0,   0,  16,  8, 0,   0,   16,  8,  0,   0,   16,
     8, 127, 252, 16, 8, 64,  2,  16,  8, 64,  2,   16,  8,  64,  2,   16,
     8, 64,  2,   16, 8, 64,  2,  16,  8, 64,  2,   16,  10, 192, 2,   16,
     8, 192, 2,   16, 8, 192, 2,  16,  7, 255, 255, 240, 0,  0,   0,   0,
     0, 0,   0,   0,  0, 0,   0,  0,   0, 0,   0,   0},
    {0,  0,   0,   0,  0, 0,   0,  0,   0,  0,   0,  0,  7, 255, 254, 0,
     8,  32,  3,   0,  8, 32,  1,  128, 8,  32,  25, 64, 8, 32,  57,  32,
     8,  32,  57,  16, 8, 32,  57, 16,  8,  32,  1,  16, 8, 48,  3,   16,
     8,  31,  254, 16, 8, 0,   0,  16,  8,  0,   0,  16, 8, 0,   0,   16,
     8,  127, 252, 16, 8, 64,  2,  16,  8,  64,  2,  16, 8, 64,  2,   16,
     8,  64,  2,   16, 8, 64,  2,  16,  8,  64,  2,  16, 8, 192, 2,   16,
     10, 192, 2,   16, 8, 192, 2,  144, 12, 192, 2,  16, 7, 255, 255, 224,
     0,  0,   0,   0,  0, 0,   0,  0,   0,  0,   0,  0,  0, 0,   0,   0},
    {0,  0,  0,   0,   0,  0,  0,  0,   0,  0,  0,  0,  15, 255, 255, 0,
     8,  64, 3,   128, 24, 64, 50, 192, 24, 64, 58, 96, 24, 64,  58,  48,
     24, 64, 58,  16,  24, 64, 58, 16,  8,  64, 2,  16, 8,  64,  6,   16,
     8,  63, 248, 16,  8,  0,  0,  16,  8,  0,  0,  16, 8,  0,   0,   16,
     8,  63, 254, 16,  8,  64, 2,  16,  8,  64, 2,  16, 8,  64,  2,   16,
     8,  64, 2,   16,  8,  64, 3,  24,  8,  64, 3,  24, 8,  64,  3,   24,
     8,  64, 3,   24,  10, 64, 3,  88,  8,  64, 3,  24, 15, 255, 255, 240,
     0,  0,  0,   0,   0,  0,  0,  0,   0,  0,  0,  0,  0,  0,   0,   0},
    {0,   0,   0,   0,  0,   0,   28,  0,   0,   31,  254, 0,  15,  240, 17,
     128, 62,  0,   24, 192, 102, 1,   216, 32,  102, 1,   72, 16,  34,  1,
     72,  16,  34,  1,  200, 16,  34,  0,   200, 24,  34,  0,  8,   8,   34,
     1,   240, 8,   33, 255, 0,   8,   48,  0,   0,   8,   48, 0,   0,   8,
     16,  0,   14,  8,  16,  15,  251, 8,   16,  120, 1,   12, 16,  192, 1,
     4,   16,  64,  1,  4,   16,  64,  1,   132, 24,  64,  0,  132, 8,   64,
     0,   132, 8,   64, 0,   132, 8,   64,  0,   150, 8,   64, 0,   134, 8,
     96,  0,   196, 9,  32,  7,   248, 12,  39,  252, 0,   7,  252, 0,   0,
     2,   0,   0,   0,  0,   0,   0,   0},
    {0,   0,   0,   0,  0,   0,   252, 0,   0,   63,  163, 0,   7,   224, 33,
     128, 120, 1,   48, 96,  72,  3,   176, 48,  72,  2,   144, 16,  72,  2,
     144, 16,  76,  3,  144, 24,  68,  1,   16,  8,   68,  0,   16,  8,   100,
     3,   224, 8,   38, 126, 0,   8,   35,  128, 0,   8,   32,  0,   0,   12,
     32,  0,   31,  4,  48,  3,   241, 4,   16,  124, 1,   132, 16,  64,  0,
     132, 16,  192, 0,  134, 16,  64,  0,   134, 16,  64,  0,   130, 24,  64,
     0,   130, 8,   64, 0,   194, 8,   96,  0,   90,  8,   96,  0,   66,  8,
     32,  0,   110, 9,  32,  3,   248, 12,  32,  126, 0,   4,   63,  128, 0,
     7,   240, 0,   0,  0,   0,   0,   0},
    {0,   0,   0,   0,  0,   0,   252, 0,   0,   63,  163, 0,   7,   224, 33,
     128, 120, 1,   48, 96,  72,  3,   176, 48,  72,  2,   144, 16,  72,  2,
     144, 16,  76,  3,  144, 24,  68,  1,   16,  8,   68,  0,   16,  8,   100,
     3,   224, 8,   38, 126, 0,   8,   35,  128, 0,   8,   32,  0,   0,   12,
     32,  0,   31,  4,  48,  3,   241, 4,   16,  124, 1,   132, 16,  64,  0,
     132, 16,  192, 0,  134, 16,  64,  0,   134, 16,  64,  0,   130, 24,  64,
     0,   130, 8,   64, 0,   194, 8,   96,  0,   90,  8,   96,  0,   66,  8,
     32,  0,   110, 9,  32,  3,   248, 12,  32,  126, 0,   4,   63,  128, 0,
     7,   240, 0,   0,  0,   0,   0,   0},
    {0,   0,   0,   0,  0,   0,   252, 0,   0,   63,  163, 0,   7,   224, 33,
     128, 120, 1,   48, 96,  72,  3,   176, 48,  72,  2,   144, 16,  72,  2,
     144, 16,  76,  3,  144, 24,  68,  1,   16,  8,   68,  0,   16,  8,   100,
     3,   224, 8,   38, 126, 0,   8,   35,  128, 0,   8,   32,  0,   0,   12,
     32,  0,   31,  4,  48,  3,   241, 4,   16,  124, 1,   132, 16,  64,  0,
     132, 16,  192, 0,  134, 16,  64,  0,   134, 16,  64,  0,   130, 24,  64,
     0,   130, 8,   64, 0,   194, 8,   96,  0,   90,  8,   96,  0,   66,  8,
     32,  0,   110, 9,  32,  3,   248, 12,  32,  126, 0,   4,   63,  128, 0,
     7,   240, 0,   0,  0,   0,   0,   0},
    {0,   0,   0,   0,  0,   0,   252, 0,   0,   63,  163, 0,   7,   224, 33,
     128, 120, 1,   48, 96,  72,  3,   176, 48,  72,  2,   144, 16,  72,  2,
     144, 16,  76,  3,  144, 24,  68,  1,   16,  8,   68,  0,   16,  8,   100,
     3,   224, 8,   38, 126, 0,   8,   35,  128, 0,   8,   32,  0,   0,   12,
     32,  0,   31,  4,  48,  3,   241, 4,   16,  124, 1,   132, 16,  64,  0,
     132, 16,  192, 0,  134, 16,  64,  0,   134, 16,  64,  0,   130, 24,  64,
     0,   130, 8,   64, 0,   194, 8,   96,  0,   90,  8,   96,  0,   66,  8,
     32,  0,   110, 9,  32,  3,   248, 12,  32,  126, 0,   4,   63,  128, 0,
     7,   240, 0,   0,  0,   0,   0,   0},
    {0,  0,   0,   0,   0,  0,   60,  0,   0,  15, 242, 0,   7,  240, 17,  128,
     62, 0,   144, 192, 68, 1,   208, 32,  68, 1,  88,  16,  70, 1,   72,  16,
     98, 1,   200, 16,  34, 0,   136, 24,  34, 0,  24,  8,   34, 1,   240, 8,
     35, 255, 0,   8,   32, 128, 0,   8,   48, 0,  0,   8,   16, 0,   14,  8,
     16, 7,   241, 12,  16, 124, 1,   4,   16, 64, 1,   4,   16, 64,  1,   132,
     16, 64,  0,   132, 24, 64,  0,   132, 8,  64, 0,   134, 8,  64,  0,   134,
     8,  64,  0,   146, 8,  96,  0,   194, 8,  32, 0,   78,  13, 32,  7,   240,
     12, 35,  252, 0,   6,  254, 0,   0,   3,  0,  0,   0,   0,  0,   0,   0},
    {0, 0,  0,  0,  0,  0,   0, 0,  0,  0,  0,  0,   15,  255, 254, 0,  8,  64,
     3, 0,  8,  64, 50, 128, 8, 64, 58, 64, 8,  64,  58,  48,  8,   64, 58, 16,
     8, 64, 58, 16, 8,  64,  2, 16, 8,  64, 6,  16,  8,   63,  248, 16, 8,  0,
     0, 16, 8,  0,  0,  16,  8, 0,  0,  16, 8,  127, 254, 16,  8,   64, 2,  16,
     8, 64, 2,  16, 8,  64,  2, 16, 8,  64, 2,  16,  8,   64,  2,   16, 8,  64,
     2, 16, 10, 64, 2,  80,  8, 64, 2,  16, 15, 255, 255, 240, 0,   0,  0,  0,
     0, 0,  0,  0,  0,  0,   0, 0,  0,  0,  0,  0},
    {0, 0,   0,   0,   0,  0,   0,  0,   0, 0,   0,   0,   7,  255, 254, 0,
     8, 32,  3,   0,   8,  32,  1,  128, 8, 32,  25,  64,  8,  32,  57,  32,
     8, 32,  57,  16,  8,  32,  57, 16,  8, 32,  1,   16,  8,  48,  3,   16,
     8, 31,  254, 16,  8,  0,   0,  16,  8, 0,   0,   16,  8,  0,   0,   16,
     8, 127, 252, 16,  8,  64,  2,  16,  8, 64,  2,   16,  8,  64,  2,   16,
     8, 64,  2,   16,  8,  64,  2,  16,  8, 192, 2,   16,  10, 192, 2,   16,
     8, 192, 2,   144, 12, 192, 2,  16,  7, 255, 255, 224, 0,  0,   0,   0,
     0, 0,   0,   0,   0,  0,   0,  0,   0, 0,   0,   0},
    {0, 0,   0,   0,   0, 0,   0,  0,   0, 0,   0,   0,   7,  255, 254, 0,
     8, 32,  3,   0,   8, 96,  3,  128, 8, 96,  58,  64,  8,  96,  42,  32,
     8, 96,  42,  16,  8, 64,  58, 16,  8, 64,  2,   16,  8,  32,  2,   16,
     8, 31,  252, 16,  8, 0,   0,  16,  8, 0,   0,   16,  8,  0,   0,   16,
     8, 127, 252, 16,  8, 64,  2,  16,  8, 64,  2,   16,  8,  64,  2,   16,
     8, 64,  2,   16,  8, 64,  2,  16,  8, 192, 2,   16,  10, 192, 2,   16,
     8, 192, 2,   144, 8, 192, 2,  16,  7, 255, 255, 224, 0,  0,   0,   0,
     0, 0,   0,   0,   0, 0,   0,  0,   0, 0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,  15, 255, 255, 0,
     8,  64,  3,   128, 8,  64,  50,  192, 8,  64,  114, 96, 8,  64,  114, 32,
     8,  64,  114, 16,  8,  64,  114, 16,  8,  64,  2,   16, 8,  96,  6,   16,
     8,  63,  252, 16,  8,  0,   0,   16,  8,  0,   0,   16, 8,  0,   0,   16,
     8,  127, 254, 16,  8,  64,  2,   16,  8,  64,  2,   16, 8,  64,  2,   16,
     8,  64,  2,   16,  8,  192, 2,   16,  24, 192, 2,   16, 24, 192, 2,   16,
     24, 192, 2,   16,  26, 192, 2,   80,  8,  192, 2,   16, 15, 255, 255, 240,
     0,  0,   0,   0,   0,  0,   0,   0,   0,  0,   0,   0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,   0,  0,  0,   0,   0,   15, 255, 255, 0,
     24, 192, 2,   128, 24, 192, 50,  64, 24, 192, 114, 32,  24, 192, 114, 16,
     24, 192, 114, 24,  24, 192, 114, 8,  24, 192, 2,   8,   24, 120, 6,   8,
     24, 63,  248, 8,   24, 0,   0,   8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,   24, 16, 192, 3,   24,  16, 192, 3,   24,
     16, 192, 3,   24,  16, 192, 3,   24, 16, 192, 3,   24,  16, 192, 3,   24,
     18, 192, 3,   88,  16, 192, 3,   24, 15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,   0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0,   15, 255, 255, 0,
     16, 192, 2,   128, 16, 192, 50, 64, 16, 192, 114, 32,  16, 192, 82,  16,
     16, 192, 114, 24,  16, 192, 50, 8,  16, 192, 2,   8,   16, 127, 254, 8,
     16, 63,  248, 8,   16, 0,   0,  8,  16, 0,   0,   8,   16, 0,   0,   8,
     16, 127, 254, 8,   16, 192, 2,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     16, 192, 3,   8,   16, 192, 3,  8,  16, 192, 3,   8,   16, 192, 3,   8,
     18, 192, 3,   72,  16, 192, 3,  8,  15, 255, 255, 240, 7,  255, 255, 224,
     0,  0,   0,   0,   0,  0,   0,  0,  0,  0,   0,   0},
    {0,   0,   0,   0,   0,   0,   0,   0,   0,  0,   0,   0,   15,  255, 255,
     0,   16,  192, 2,   128, 16,  192, 50,  64, 16,  192, 114, 32,  16,  192,
     82,  16,  16,  192, 114, 24,  16,  192, 50, 8,   16,  192, 2,   8,   16,
     127, 254, 8,   16,  63,  248, 8,   16,  0,  0,   8,   16,  0,   0,   8,
     16,  0,   0,   8,   16,  127, 254, 8,   16, 192, 2,   8,   16,  192, 3,
     8,   16,  192, 3,   8,   16,  192, 3,   8,  16,  192, 3,   8,   16,  192,
     3,   8,   18,  192, 3,   72,  16,  192, 3,  8,   15,  255, 255, 240, 7,
     255, 255, 224, 0,   0,   0,   0,   0,   0,  0,   0,   0,   0,   0,   0}};

// ==========================================
// 8. ANIMACJA KARMIENIA (PORRIDGE) - 20 KLATEK (PROCEDURALNA)
// ==========================================
// UĹĽywamy proceduralnego podejĹ›cia (rybki i kropki) zdefiniowanego w metodzie
// drawFeedingScreen

#define FRAME_DELAY (42)
#define FRAME_WIDTH (32)
#define FRAME_HEIGHT (32)
// Liczba klatek w tablicy frames_save
#define SAVE_FRAME_COUNT (sizeof(frames_save) / sizeof(frames_save[0]))

AquariumAnimation::AquariumAnimation(U8G2 *u8g2_instance) {
  this->display = u8g2_instance;

  snprintf(timeBuffer, sizeof(timeBuffer), "--:--:--");
  snprintf(tempBuffer, sizeof(tempBuffer), "T:--.-'C");
  snprintf(dateBuffer, sizeof(dateBuffer), "--.--.--");
  snprintf(valBuffer, sizeof(valBuffer), "0%%");
  snprintf(batteryVoltageBuffer, sizeof(batteryVoltageBuffer), "--.-V");
  snprintf(feedTimeBuffer, sizeof(feedTimeBuffer), "--:--");

  batteryPercent = 0;
  isFilterOn = false;
  isLightOn = false;
  isHeaterOn = false;
  feedFreq = 1;
  feedDaysPassed = 0;

  isFeedingAnim = false;
  fish1_x = 90;
  fish1_dir = 1;
  fish2_x = 120;
  fish2_dir = -1;
  for (int i = 0; i < 10; i++)
    particles[i].active = false;

  initFishObjs(); // Inicjalizacja rybek proceduralnych

  menuSelection = 0;
  menuScrollOffset = 0;
  scheduleSelection = 0;

  scheduleHourOn = 9;
  scheduleMinOn = 30;
  scheduleHourOff = 23;
  scheduleMinOff = 0;
  aerationHourOn = 8;
  aerationMinOn = 0;
  aerationHourOff = 20;
  aerationMinOff = 0;
  filterHourOn = 8;
  filterMinOn = 0;
  filterHourOff = 18;
  filterMinOff = 0;
  targetTemp = 24;

  feedHour = 15;
  feedMinute = 0; // DomyĹ›lne karmienie

  currentHour = 0;
  currentMinute = 0;
  currentSecond = 0;
  currentDay = 1;
  currentMonth = 1;
  currentYear = 2025;

  scheduleChangePending = false;
  timeChangePending = false;
  activeScheduleId = 0;

  isEditing = false;
  editState = 0;

  logCount = 0;
  logScroll = 0;

  testSelection = 0;
  testLight = false;
  testHeater = false;
  testFilter = false;
  testAerationVal = 0;

  confirmAnimActive = false;
  confirmAnimFrame = 0;
  confirmAnimLastStep = 0;
}

// --- METODY POMOCNICZE DO ANIMACJI ---
void AquariumAnimation::initFishObjs() {
  for (int i = 0; i < NUM_FISH; i++) {
    fish[i].x = random(10, 118);
    fish[i].y = random(6, 26);
    fish[i].dir = random(0, 2) ? 1 : -1;
    fish[i].speed = random(8, 14) / 10.0;
    fish[i].targetY = random(6, 26);
  }
}

void AquariumAnimation::dropFood() {
  for (int i = 0; i < NUM_FOOD; i++) {
    food[i].x = random(5, 123);
    food[i].y = 0;
    food[i].visible = true;
  }
  feedingActive = true;
  lastFeedTime = millis();
}

void AquariumAnimation::drawFishObj(int x, int y, int dir) {
  if (dir > 0) {
    display->drawTriangle(x - 3, y, x - 6, y - 2, x - 6, y + 2);
    display->drawDisc(x, y, 3);
    display->setDrawColor(0);
    display->drawPixel(x + 1, y - 1);
    display->setDrawColor(1);
  } else {
    display->drawTriangle(x + 3, y, x + 6, y - 2, x + 6, y + 2);
    display->drawDisc(x, y, 3);
    display->setDrawColor(0);
    display->drawPixel(x - 1, y - 1);
    display->setDrawColor(1);
  }
}

void AquariumAnimation::drawBubbles() {
  for (int i = 0; i < NUM_BUBBLES; i++) {
    if (bubbles[i].active) {
      if (!bubbles[i].popping) {
        display->drawCircle(bubbles[i].x, bubbles[i].y, (int)bubbles[i].size);
        bubbles[i].y -= 0.7;
        if (bubbles[i].size < 3)
          bubbles[i].size += 0.05;
        if (bubbles[i].y < -bubbles[i].size)
          bubbles[i].popping = true;
      } else {
        display->drawPixel(bubbles[i].x, bubbles[i].y);
        display->drawPixel(bubbles[i].x - 1, bubbles[i].y);
        display->drawPixel(bubbles[i].x + 1, bubbles[i].y);
        display->drawPixel(bubbles[i].x, bubbles[i].y - 1);
        display->drawPixel(bubbles[i].x, bubbles[i].y + 1);
        bubbles[i].active = false;
      }
    }
  }
  if (millis() - bubbleTimer > 300) {
    bubbleTimer = millis();
    for (int i = 0; i < NUM_BUBBLES; i++) {
      if (!bubbles[i].active) {
        bubbles[i].x = random(0, 128);
        bubbles[i].y = 31;
        bubbles[i].size = 1;
        bubbles[i].active = true;
        bubbles[i].popping = false;
        break;
      }
    }
  }
}

void AquariumAnimation::moveFish() {
  for (int i = 0; i < NUM_FISH; i++) {
    Fish &f = fish[i];
    if (f.x <= 5)
      f.dir = 1;
    if (f.x >= 123)
      f.dir = -1;
    f.x += f.dir * f.speed;
    if (abs(f.y - f.targetY) > 1)
      f.y += (f.y < f.targetY) ? 0.5 : -0.5;
    else if (random(0, 50) == 0)
      f.targetY = random(6, 26);

    if (feedingActive) {
      for (int j = 0; j < NUM_FOOD; j++) {
        if (food[j].visible && abs(f.x - food[j].x) < 20) {
          f.targetY = food[j].y;
          f.dir = (food[j].x > f.x) ? 1 : -1;
        }
        if (food[j].visible && abs(f.x - food[j].x) < 5 &&
            abs(f.y - food[j].y) < 3) {
          food[j].visible = false;
        }
      }
    }
  }
}

void AquariumAnimation::drawFood() {
  for (int i = 0; i < NUM_FOOD; i++) {
    if (food[i].visible) {
      display->drawPixel(food[i].x, food[i].y);
      food[i].y += 1;
      if (food[i].y > 30)
        food[i].visible = false;
    }
  }
}

void AquariumAnimation::drawWaves(int offset) {
  for (int x = 0; x < 128; x++) {
    int waveY = 2 + sin((x + offset) * 0.2) * 1.5;
    display->drawPixel(x, waveY);
  }
}

// --- STANDARDOWE SETTERY ---
void AquariumAnimation::setTime(int hour, int minute, int second) {
  currentHour = hour;
  currentMinute = minute;
  currentSecond = second;
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour, minute,
           second);
}
void AquariumAnimation::setTemperature(float temp) {
  if (temp < -50.0f || temp > 100.0f || temp != temp) {
    snprintf(tempBuffer, sizeof(tempBuffer), "T:--.-'C");
  } else {
    snprintf(tempBuffer, sizeof(tempBuffer), "T:%.1f'C", temp);
  }
}
void AquariumAnimation::setDate(int day, int month, int year) {
  currentDay = day;
  currentMonth = month;
  currentYear = year;
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d.%02d.%d", day, month, year);
}
void AquariumAnimation::setAeration(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  snprintf(valBuffer, sizeof(valBuffer), "%d%%", percent);
}
void AquariumAnimation::setBattery(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  batteryPercent = percent;
}
void AquariumAnimation::setBatteryVoltage(float voltage) {
  if (isnan(voltage) || voltage < 0.5f || voltage > 5.0f) {
    snprintf(batteryVoltageBuffer, sizeof(batteryVoltageBuffer), "--.-V");
  } else {
    snprintf(batteryVoltageBuffer, sizeof(batteryVoltageBuffer), "%.1fV",
             voltage);
  }
}
void AquariumAnimation::setFilterStatus(bool status) { isFilterOn = status; }
void AquariumAnimation::setLightStatus(bool status) { isLightOn = status; }
void AquariumAnimation::setHeaterStatus(bool status) { isHeaterOn = status; }

void AquariumAnimation::setFeedingSchedule(const char *time, uint8_t frequency,
                                           uint8_t daysPassed) {
  if (time != nullptr) {
    snprintf(feedTimeBuffer, sizeof(feedTimeBuffer), "%s", time);
    int parsedHour = -1;
    int parsedMinute = -1;
    if (sscanf(time, "%d:%d", &parsedHour, &parsedMinute) == 2) {
      feedHour = constrain(parsedHour, 0, 23);
      feedMinute = constrain(parsedMinute, 0, 59);
    }
  }

  feedFreq = constrain(frequency, 0, 3);
  feedDaysPassed = daysPassed;
  if (feedDaysPassed > feedFreq)
    feedDaysPassed = feedFreq;
}

void AquariumAnimation::setFeedingAnimation(bool active) {
  if (active && !isFeedingAnim) {
    feedAnimStart = millis();
    initFishObjs();
    if (active)
      dropFood();
  }
  isFeedingAnim = active;
}

void AquariumAnimation::updatePhysics() {
  // Nie uĹĽywane - logika przeniesiona do moveFish()
}

// --- LOGIKA NAWIGACJI MENU ---
void AquariumAnimation::menuNext() {
  menuSelection++;
  if (menuSelection > 4) {
    menuSelection = 0;
    menuScrollOffset = 0;
  } else if (menuSelection > menuScrollOffset + 2) {
    menuScrollOffset++;
  }
}

uint8_t AquariumAnimation::getMenuSelection() { return menuSelection; }

void AquariumAnimation::setActiveScheduleId(uint8_t id) {
  if (id > 5)
    id = 0;
  activeScheduleId = id;
  scheduleSelection = 0;
}

uint8_t AquariumAnimation::getScheduleSelection() { return scheduleSelection; }

void AquariumAnimation::setLightSchedule(uint8_t hourOn, uint8_t minuteOn,
                                         uint8_t hourOff, uint8_t minuteOff) {
  scheduleHourOn = constrain(hourOn, 0, 24);
  scheduleMinOn = constrain(minuteOn, 0, 59);
  scheduleHourOff = constrain(hourOff, 0, 24);
  scheduleMinOff = constrain(minuteOff, 0, 59);
}

void AquariumAnimation::setAerationSchedule(uint8_t hourOn, uint8_t minuteOn,
                                            uint8_t hourOff,
                                            uint8_t minuteOff) {
  aerationHourOn = constrain(hourOn, 0, 23);
  aerationMinOn = constrain(minuteOn, 0, 59);
  aerationHourOff = constrain(hourOff, 0, 23);
  aerationMinOff = constrain(minuteOff, 0, 59);
}

void AquariumAnimation::setFilterSchedule(uint8_t hourOn, uint8_t minuteOn,
                                       uint8_t hourOff, uint8_t minuteOff) {
  filterHourOn = constrain(hourOn, 0, 23);
  filterMinOn = constrain(minuteOn, 0, 59);
  filterHourOff = constrain(hourOff, 0, 23);
  filterMinOff = constrain(minuteOff, 0, 59);
}

void AquariumAnimation::setTargetTempSetting(uint8_t value) {
  targetTemp = constrain(value, 0, 99);
}

void AquariumAnimation::scheduleNext() {
  if (!isEditing) {
    scheduleSelection++;
    if (scheduleSelection > 1)
      scheduleSelection = 0;
  }
}

// --- LOGIKA EDYCJI ---
void AquariumAnimation::startEditing() {
  isEditing = true;
  editState = 1;

  if (activeScheduleId == 0) { // Light
    if (scheduleSelection == 0) {
      tempHour = scheduleHourOn;
      tempMinute = scheduleMinOn;
    } else {
      tempHour = scheduleHourOff;
      tempMinute = scheduleMinOff;
    }
  } else if (activeScheduleId == 1) { // Aeration
    if (scheduleSelection == 0) {
      tempHour = aerationHourOn;
      tempMinute = aerationMinOn;
    } else {
      tempHour = aerationHourOff;
      tempMinute = aerationMinOff;
    }
  } else if (activeScheduleId == 2) { // Filter
    if (scheduleSelection == 0) {
      tempHour = filterHourOn;
      tempMinute = filterMinOn;
    } else {
      tempHour = filterHourOff;
      tempMinute = filterMinOff;
    }
  } else if (activeScheduleId == 3) { // Temp
    tempHour = targetTemp;
  } else if (activeScheduleId == 4) { // Feeding
    if (scheduleSelection == 0) {
      tempHour = feedHour;
      tempMinute = feedMinute;
    } else {
      tempHour = feedFreq;
    }
  } else if (activeScheduleId == 5) { // Date/time
    if (scheduleSelection == 0) {
      tempHour = currentHour;
      tempMinute = currentMinute;
      tempSecond = currentSecond;
    } else {
      tempDay = currentDay;
      tempMonth = currentMonth;
      tempYear = currentYear;
    }
  }
}
void AquariumAnimation::scheduleEditIncrement() {
  if (activeScheduleId == 0) {
    if (editState == 1) {
      tempHour++;
      if (tempHour > 24)
        tempHour = 0;
      if (tempHour == 24)
        tempMinute = 0;
    } else if (editState == 2) {
      if (tempHour != 24) {
        tempMinute += 5;
        if (tempMinute > 59)
          tempMinute = 0;
      }
    }
  } else if (activeScheduleId == 1 || activeScheduleId == 2) {
    if (editState == 1) {
      tempHour++;
      if (tempHour > 23)
        tempHour = 0;
    } else if (editState == 2) {
      tempMinute += 5;
      if (tempMinute > 59)
        tempMinute = 0;
    }
  } else if (activeScheduleId == 3) {
    if (tempHour == 0)
      tempHour = 18;
    else {
      tempHour++;
      if (tempHour > 28)
        tempHour = 0;
    }
  } else if (activeScheduleId == 4) {
    if (scheduleSelection == 0) {
      if (editState == 1) {
        tempHour++;
        if (tempHour > 23)
          tempHour = 0;
      } else if (editState == 2) {
        tempMinute += 5;
        if (tempMinute > 59)
          tempMinute = 0;
      }
    } else {
      tempHour++;
      if (tempHour > 3)
        tempHour = 0;
    }
  } else if (activeScheduleId == 5) {
    if (scheduleSelection == 0) {
      if (editState == 1) {
        tempHour++;
        if (tempHour > 23)
          tempHour = 0;
      } else if (editState == 2) {
        tempMinute++;
        if (tempMinute > 59)
          tempMinute = 0;
      } else if (editState == 3) {
        tempSecond++;
        if (tempSecond > 59)
          tempSecond = 0;
      }
    } else {
      if (editState == 1) {
        tempDay++;
        if (tempDay > 31)
          tempDay = 1;
      } else if (editState == 2) {
        tempMonth++;
        if (tempMonth > 12)
          tempMonth = 1;
      } else if (editState == 3) {
        tempYear++;
        if (tempYear > 2099)
          tempYear = 2024;
      }
    }
  }
}
void AquariumAnimation::nextEditStep() {
  // One-step edits: target temperature or feeding frequency.
  if (activeScheduleId == 3 ||
      (activeScheduleId == 4 && scheduleSelection == 1)) {
    if (activeScheduleId == 3)
      targetTemp = tempHour;
    else
      feedFreq = tempHour;

    scheduleChangePending = true;
    isEditing = false;
    editState = 0;
    playConfirmAnimation();
    return;
  }

  // Date/time has 3 editing steps.
  if (activeScheduleId == 5) {
    if (editState < 3) {
      editState++;
    } else {
      timeChangePending = true;
      isEditing = false;
      editState = 0;
      playConfirmAnimation();
    }
    return;
  }

  // Two-step edits: time on/off schedules and feeding time.
  if (editState == 1) {
    editState = 2;
    return;
  }

  if (editState == 2) {
    if (activeScheduleId == 0) {
      if (scheduleSelection == 0) {
        scheduleHourOn = tempHour;
        scheduleMinOn = tempMinute;
      } else {
        scheduleHourOff = tempHour;
        scheduleMinOff = tempMinute;
      }
    } else if (activeScheduleId == 1) {
      if (scheduleSelection == 0) {
        aerationHourOn = tempHour;
        aerationMinOn = tempMinute;
      } else {
        aerationHourOff = tempHour;
        aerationMinOff = tempMinute;
      }
    } else if (activeScheduleId == 2) {
      if (scheduleSelection == 0) {
        filterHourOn = tempHour;
        filterMinOn = tempMinute;
      } else {
        filterHourOff = tempHour;
        filterMinOff = tempMinute;
      }
    } else if (activeScheduleId == 4) {
      feedHour = tempHour;
      feedMinute = tempMinute;
    }

    scheduleChangePending = true;
    isEditing = false;
    editState = 0;
    playConfirmAnimation();
  }
}
bool AquariumAnimation::isEditingActive() { return isEditing; }

// --- GETTERY ---
bool AquariumAnimation::hasScheduleChanged() {
  if (scheduleChangePending) {
    scheduleChangePending = false;
    return true;
  }
  return false;
}
bool AquariumAnimation::hasTimeChanged() {
  if (timeChangePending) {
    timeChangePending = false;
    return true;
  }
  return false;
}
uint8_t AquariumAnimation::getScheduleHourOn() { return scheduleHourOn; }
uint8_t AquariumAnimation::getScheduleMinOn() { return scheduleMinOn; }
uint8_t AquariumAnimation::getScheduleHourOff() { return scheduleHourOff; }
uint8_t AquariumAnimation::getScheduleMinOff() { return scheduleMinOff; }
uint8_t AquariumAnimation::getAerationHourOn() { return aerationHourOn; }
uint8_t AquariumAnimation::getAerationMinOn() { return aerationMinOn; }
uint8_t AquariumAnimation::getAerationHourOff() { return aerationHourOff; }
uint8_t AquariumAnimation::getAerationMinOff() { return aerationMinOff; }
uint8_t AquariumAnimation::getFilterHourOn() { return filterHourOn; }
uint8_t AquariumAnimation::getFilterMinOn() { return filterMinOn; }
uint8_t AquariumAnimation::getFilterHourOff() { return filterHourOff; }
uint8_t AquariumAnimation::getFilterMinOff() { return filterMinOff; }
uint8_t AquariumAnimation::getTargetTemp() { return targetTemp; }
uint8_t AquariumAnimation::getFeedHour() { return feedHour; }
uint8_t AquariumAnimation::getFeedMinute() { return feedMinute; }
uint8_t AquariumAnimation::getFeedFreq() { return feedFreq; }

uint8_t AquariumAnimation::getNewHour() { return tempHour; }
uint8_t AquariumAnimation::getNewMinute() { return tempMinute; }
uint8_t AquariumAnimation::getNewSecond() { return tempSecond; }
uint8_t AquariumAnimation::getNewDay() { return tempDay; }
uint8_t AquariumAnimation::getNewMonth() { return tempMonth; }
uint16_t AquariumAnimation::getNewYear() { return tempYear; }

// --- ANIMACJA POTWIERDZENIA (ZAPIS) - DUĹ»A ANIMACJA "SAVE" ---
void AquariumAnimation::playConfirmAnimation() {
  confirmAnimActive = true;
  confirmAnimFrame = 0;
  confirmAnimLastStep = millis();
}

bool AquariumAnimation::drawConfirmAnimationFrame() {
  if (!confirmAnimActive || !display)
    return false;

  display->setBitmapMode(0);
  display->drawBitmap(48, 0, 4, 32, frames_save[confirmAnimFrame]);

  unsigned long now = millis();
  if (now - confirmAnimLastStep >= FRAME_DELAY) {
    confirmAnimLastStep = now;
    confirmAnimFrame++;
    if (confirmAnimFrame >= SAVE_FRAME_COUNT) {
      confirmAnimActive = false;
      confirmAnimFrame = 0;
    }
  }

  return true;
}

// --- ANIMACJA KARMIENIA (PROCEDURALNA) ---
void AquariumAnimation::initFeeding() {
  initFishObjs();
  dropFood();
}

void AquariumAnimation::drawFeedingScreen() {
  HomeRenderer::drawFeedingScreen(this);
}

// LOGI
void AquariumAnimation::addLog(const char *message, const char *time) {
  if (logCount >= 20) {
    for (int i = 0; i < 19; i++) {
      logs[i] = logs[i + 1];
    }
    logCount = 19;
  }
  strncpy(logs[logCount].message, message, 19);
  logs[logCount].message[19] = '\0';
  strncpy(logs[logCount].time, time, 5);
  logs[logCount].time[5] = '\0';
  logCount++;
  if (logCount > 3)
    logScroll = logCount - 3;
}
void AquariumAnimation::logScrollNext() {
  if (logCount > 3) {
    logScroll++;
    if (logScroll > logCount - 3)
      logScroll = 0;
  }
}
void AquariumAnimation::clearLogs() {
  logCount = 0;
  logScroll = 0;
}
void AquariumAnimation::drawLogs(bool btnBackState, bool btnSelectState,
                                 bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawLine(98, 0, 98, 31);
  display->drawLine(18, 0, 18, 31);
  display->drawLine(19, 10, 127, 10);
  display->drawLine(19, 21, 127, 21);
  display->drawXBMP(1, 8, 16, 16, image_operation_warning_bits);
  display->setFont(u8g2_font_profont10_tr);
  if (logCount == 0) {
    display->drawStr(26, 8, "Brak logow");
  } else {
    for (int i = 0; i < 3; i++) {
      int index = logScroll + i;
      if (index < logCount) {
        int yPos = 8 + (i * 11);
        char msgShort[13];
        strncpy(msgShort, logs[index].message, 12);
        msgShort[12] = '\0';
        display->drawStr(26, yPos, msgShort);
        display->drawStr(101, yPos, logs[index].time);
        int arrowY = 3 + (i * 11);
        display->drawXBMP(21, arrowY, 3, 5, image_ButtonRightSmall_copy_bits);
      }
    }
  }
}

// MENU
void AquariumAnimation::drawMenu(bool btnBackState, bool btnSelectState,
                                 bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->setFont(u8g2_font_6x10_tr);
  display->drawStr(3, 7, "M");
  display->drawStr(3, 15, "E");
  display->drawStr(3, 23, "N");
  display->drawStr(3, 32, "U");
  display->drawLine(10, 0, 10, 32);
  display->drawLine(0, 0, 0, 31);
  display->drawLine(10, 10, 127, 10);
  display->drawLine(10, 21, 127, 21);
  display->drawLine(127, 1, 127, 32);
  display->drawLine(114, 0, 114, 31);
  const char *items[] = {"Harmonogramy", "Logi", "Data i Czas", "Test", "Wifi"};
  for (int i = 0; i < 3; i++) {
    int itemIndex = menuScrollOffset + i;
    int yPos = 9 + (i * 11);
    display->drawStr(12, yPos, items[itemIndex]);
  }
  int visualCursorPos = menuSelection - menuScrollOffset;
  if (visualCursorPos == 0)
    display->drawXBMP(106, 2, 4, 7, image_ButtonLeft_bits);
  else if (visualCursorPos == 1)
    display->drawXBMP(106, 13, 4, 7, image_ButtonLeft_copy_bits);
  else if (visualCursorPos == 2)
    display->drawXBMP(106, 24, 4, 7, image_ButtonLeft_copy_bits);
  if (!btnBackState)
    display->drawXBMP(116, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(116, 13, 10, 7, image_MenuCheck_bits);
  if (!btnNextState)
    display->drawXBMP(119, 24, 5, 7, image_arrow_down_bits);
}

// HARMONOGRAM ĹšWIATĹO
void AquariumAnimation::drawSchedule(bool btnBackState, bool btnSelectState,
                                     bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawLine(0, 15, 113, 15);
  display->drawXBMP(2, 0, 10, 14, image_Layer_4_1_bits);
  display->drawXBMP(3, 17, 10, 14, image_Layer_4_copy_bits);
  display->drawLine(15, 1, 15, 32);
  display->setFont(u8g2_font_6x13_tr);
  char bufOn[14], bufOff[14];
  if (isEditing && scheduleSelection == 0 && activeScheduleId == 0) {
    if (tempHour == 24) {
      if (editState == 1 && (millis() / 500) % 2 == 0)
        sprintf(bufOn, "");
      else
        sprintf(bufOn, "ON");
    } else {
      if (editState == 1) {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOn, "  :%02d", tempMinute);
        else
          sprintf(bufOn, "%02d:%02d", tempHour, tempMinute);
      } else {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOn, "%02d:  ", tempHour);
        else
          sprintf(bufOn, "%02d:%02d", tempHour, tempMinute);
      }
    }
  } else {
    if (scheduleHourOn == 24)
      sprintf(bufOn, "ON");
    else
      sprintf(bufOn, "%02d:%02d", scheduleHourOn, scheduleMinOn);
  }
  if (isEditing && scheduleSelection == 1 && activeScheduleId == 0) {
    if (tempHour == 24) {
      if (editState == 1 && (millis() / 500) % 2 == 0)
        sprintf(bufOff, "");
      else
        sprintf(bufOff, "OFF");
    } else {
      if (editState == 1) {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOff, "  :%02d", tempMinute);
        else
          sprintf(bufOff, "%02d:%02d", tempHour, tempMinute);
      } else {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOff, "%02d:  ", tempHour);
        else
          sprintf(bufOff, "%02d:%02d", tempHour, tempMinute);
      }
    }
  } else {
    if (scheduleHourOff == 24)
      sprintf(bufOff, "OFF");
    else
      sprintf(bufOff, "%02d:%02d", scheduleHourOff, scheduleMinOff);
  }
  display->drawStr(23, 13, bufOn);
  display->drawStr(23, 30, bufOff);
  if (scheduleSelection == 0)
    display->drawXBMP(108, 4, 4, 7, image_ButtonLeft_bits);
  else
    display->drawXBMP(108, 21, 4, 7, image_ButtonLeft_copy_bits);
  display->drawLine(114, 0, 114, 31);
  display->drawLine(127, 0, 127, 31);
  display->drawLine(0, 0, 0, 31);
  display->drawLine(115, 10, 126, 10);
  display->drawLine(115, 20, 126, 20);
  if (!btnBackState)
    display->drawXBMP(116, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(116, 12, 10, 7, image_Layer_11_bits);
  if (!btnNextState)
    display->drawXBMP(119, 23, 5, 7, image_arrow_down_bits);
}

void AquariumAnimation::drawScheduleAeration(bool btnBackState,
                                             bool btnSelectState,
                                             bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawLine(0, 15, 113, 15);
  display->drawXBMP(1, 0, 15, 16, image_schedule_aeration_wind_bits);
  display->drawXBMP(1, 19, 15, 10, image_schedule_aeration_off_bits);
  display->drawLine(15, 1, 15, 32);

  display->setFont(u8g2_font_6x13_tr);
  char bufOn[8];
  char bufOff[8];

  if (isEditing && scheduleSelection == 0 && activeScheduleId == 1) {
    if (editState == 1 && (millis() / 500) % 2 == 0)
      sprintf(bufOn, "  :%02d", tempMinute);
    else if (editState == 2 && (millis() / 500) % 2 == 0)
      sprintf(bufOn, "%02d:  ", tempHour);
    else
      sprintf(bufOn, "%02d:%02d", tempHour, tempMinute);
  } else {
    sprintf(bufOn, "%02d:%02d", aerationHourOn, aerationMinOn);
  }

  if (isEditing && scheduleSelection == 1 && activeScheduleId == 1) {
    if (editState == 1 && (millis() / 500) % 2 == 0)
      sprintf(bufOff, "  :%02d", tempMinute);
    else if (editState == 2 && (millis() / 500) % 2 == 0)
      sprintf(bufOff, "%02d:  ", tempHour);
    else
      sprintf(bufOff, "%02d:%02d", tempHour, tempMinute);
  } else {
    sprintf(bufOff, "%02d:%02d", aerationHourOff, aerationMinOff);
  }

  display->drawStr(22, 14, bufOn);
  display->drawStr(22, 30, bufOff);

  if (scheduleSelection == 0)
    display->drawXBMP(108, 4, 4, 7, image_ButtonLeft_bits);
  else
    display->drawXBMP(108, 21, 4, 7, image_ButtonLeft_copy_bits);
  display->drawLine(114, 0, 114, 31);
  display->drawLine(127, 0, 127, 31);
  display->drawLine(0, 0, 0, 31);
  display->drawLine(115, 10, 126, 10);
  display->drawLine(115, 20, 126, 20);
  if (!btnBackState)
    display->drawXBMP(116, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(116, 12, 10, 7, image_Layer_11_bits);
  if (!btnNextState)
    display->drawXBMP(119, 23, 5, 7, image_arrow_down_bits);
}

// HARMONOGRAM FILTRA
void AquariumAnimation::drawScheduleFilter(bool btnBackState, bool btnSelectState,
                                        bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawLine(0, 15, 113, 15);
  display->drawXBMP(2, 0, 11, 14, image_weather_humidity_1_bits);
  display->drawXBMP(2, 17, 11, 14, image_weather_humidity_white_bits);
  display->drawLine(15, 1, 15, 32);
  display->setFont(u8g2_font_6x13_tr);
  char bufOn[10], bufOff[10];
  if (isEditing && scheduleSelection == 0 && activeScheduleId == 2) {
    if (tempHour == 0 && tempMinute == 0) {
      if ((millis() / 500) % 2 == 0)
        sprintf(bufOn, "");
      else
        sprintf(bufOn, "OFF");
    } else {
      if (editState == 1) {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOn, "  :%02d", tempMinute);
        else
          sprintf(bufOn, "%02d:%02d", tempHour, tempMinute);
      } else {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOn, "%02d:  ", tempHour);
        else
          sprintf(bufOn, "%02d:%02d", tempHour, tempMinute);
      }
    }
  } else {
    if (filterHourOn == 0 && filterMinOn == 0)
      sprintf(bufOn, "OFF");
    else
      sprintf(bufOn, "%02d:%02d", filterHourOn, filterMinOn);
  }
  if (isEditing && scheduleSelection == 1 && activeScheduleId == 2) {
    if (tempHour == 0 && tempMinute == 0) {
      if ((millis() / 500) % 2 == 0)
        sprintf(bufOff, "");
      else
        sprintf(bufOff, "OFF");
    } else {
      if (editState == 1) {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOff, "  :%02d", tempMinute);
        else
          sprintf(bufOff, "%02d:%02d", tempHour, tempMinute);
      } else {
        if ((millis() / 500) % 2 == 0)
          sprintf(bufOff, "%02d:  ", tempHour);
        else
          sprintf(bufOff, "%02d:%02d", tempHour, tempMinute);
      }
    }
  } else {
    if (filterHourOff == 0 && filterMinOff == 0)
      sprintf(bufOff, "OFF");
    else
      sprintf(bufOff, "%02d:%02d", filterHourOff, filterMinOff);
  }
  display->drawStr(23, 13, bufOn);
  display->drawStr(23, 29, bufOff);
  if (scheduleSelection == 0)
    display->drawXBMP(108, 4, 4, 7, image_ButtonLeft_bits);
  else
    display->drawXBMP(108, 21, 4, 7, image_ButtonLeft_copy_bits);
  display->drawLine(114, 0, 114, 31);
  display->drawLine(127, 0, 127, 31);
  display->drawLine(0, 0, 0, 31);
  display->drawLine(115, 10, 126, 10);
  display->drawLine(115, 20, 126, 20);
  display->drawXBMP(0, 0, 0, 0, image_Layer_15_bits);
  if (!btnBackState)
    display->drawXBMP(116, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(116, 12, 10, 7, image_Layer_11_bits);
  if (!btnNextState)
    display->drawXBMP(119, 23, 5, 7, image_arrow_down_bits);
}

// TEMPERATURA
void AquariumAnimation::drawScheduleTemp(bool btnBackState, bool btnSelectState,
                                         bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawLine(15, 1, 15, 32);
  display->drawXBMP(3, 4, 13, 25, image_weather_temperature_bits);
  display->setFont(u8g2_font_timR24_tr);
  char tempBuf[10];
  uint8_t valToShow;
  if (isEditing)
    valToShow = tempHour;
  else
    valToShow = targetTemp;
  if (valToShow == 0)
    sprintf(tempBuf, "OFF");
  else
    sprintf(tempBuf, "%d*C", valToShow);
  if (isEditing && (millis() / 500) % 2 == 0) {
  } else {
    display->drawStr(27, 27, tempBuf);
  }
  display->drawLine(114, 0, 114, 31);
  display->drawLine(115, 10, 126, 10);
  display->drawLine(115, 20, 126, 20);
  display->drawXBMP(108, 12, 4, 7, image_ButtonLeft_bits);
  display->drawLine(0, 0, 113, 0);
  display->drawLine(127, 0, 127, 31);
  display->drawLine(113, 31, 1, 31);
  display->drawLine(0, 0, 0, 31);
  display->drawXBMP(0, 0, 0, 0, image_Layer_15_bits);
  if (!btnBackState)
    display->drawXBMP(116, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(116, 12, 10, 7, image_Layer_11_bits);
  if (!btnNextState)
    display->drawXBMP(119, 23, 5, 7, image_arrow_down_bits);
}

// HARMONOGRAM KARMIENIA - ZAKTUALIZOWANA FUNKCJA
void AquariumAnimation::drawScheduleFeeding(bool btnBackState,
                                            bool btnSelectState,
                                            bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // Ikona karmienia
  display->drawXBMP(2, 1, 25, 29, image_Layer_1_bits);

  // Pionowa linia oddzielajÄ…ca ikonÄ™
  display->drawLine(29, 0, 29, 31);

  // --- WIERSZ 1: CZAS KARMIENIA ---
  display->setFont(u8g2_font_timR14_tr);
  char bufTime[10];
  if (isEditing && scheduleSelection == 0) {
    if (editState == 1 && (millis() / 500) % 2 == 0)
      sprintf(bufTime, "  :%02d", tempMinute);
    else if (editState == 2 && (millis() / 500) % 2 == 0)
      sprintf(bufTime, "%02d:  ", tempHour);
    else
      sprintf(bufTime, "%02d:%02d", tempHour, tempMinute);
  } else {
    sprintf(bufTime, "%02d:%02d", feedHour, feedMinute);
  }
  display->drawStr(33, 14, bufTime); // Nowa pozycja tekstu

  // --- WIERSZ 2: CZÄSTOTLIWOĹšÄ† ---
  display->setFont(u8g2_font_6x10_tr); // Mniejsza czcionka dla tekstu dolnego
  char bufFreq[20];
  uint8_t val = (isEditing && scheduleSelection == 1) ? tempHour : feedFreq;

  // Zmiana logiki wyĹ›wietlania czÄ™stotliwoĹ›ci
  if (val == 0)
    sprintf(bufFreq, "OFF");
  else if (val == 1)
    sprintf(bufFreq, "Codziennie");
  else if (val == 2)
    sprintf(bufFreq, "Co 2 dni");
  else if (val == 3)
    sprintf(bufFreq, "Co 3 dni");
  else
    sprintf(bufFreq, "1/%d", val);

  if (isEditing && scheduleSelection == 1 && (millis() / 500) % 2 == 0) {
    // Miganie
  } else {
    display->drawStr(33, 31, bufFreq); // Nowa pozycja tekstu
  }

  // --- KURSOR ---
  if (scheduleSelection == 0)
    display->drawXBMP(109, 5, 4, 7, image_ButtonLeft_bits); // Kursor gĂłra
  else
    display->drawXBMP(109, 21, 4, 7,
                      image_ButtonLeft_bits); // Kursor dĂłĹ‚ (taka sama bitmapa)

  // --- LINIE DEKORACYJNE ---
  display->drawLine(0, 0, 0, 31);
  display->drawLine(0, 0, 0, 0);
  display->drawLine(29, 15, 114, 15); // Pozioma

  // Pasek boczny nawigacji
  display->drawLine(115, 0, 115, 31);
  display->drawLine(127, 10, 116, 10);
  display->drawLine(127, 21, 116, 21);

  // --- IKONY NAWIGACJI ---
  if (!btnBackState)
    display->drawXBMP(117, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnSelectState)
    display->drawXBMP(117, 13, 10, 7, image_Layer_15_check_bits); // Checkmark
  if (!btnNextState)
    display->drawXBMP(120, 23, 5, 7, image_arrow_down_bits);
}

// DATA I CZAS
void AquariumAnimation::drawSettingsDateTime(bool btnBackState,
                                             bool btnSelectState,
                                             bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);
  display->drawXBMP(1, 0, 15, 16, image_clock_quarters_bits);
  display->drawXBMP(1, 17, 15, 16, image_date_day_bits);
  display->drawLine(0, 16, 114, 16);
  display->drawLine(19, 0, 19, 31);
  display->setFont(u8g2_font_profont15_tr);
  char bufTime[12];
  if (isEditing && scheduleSelection == 0) {
    if (editState == 1 && (millis() / 500) % 2 == 0)
      sprintf(bufTime, "  :%02d:%02d", tempMinute, tempSecond);
    else if (editState == 2 && (millis() / 500) % 2 == 0)
      sprintf(bufTime, "%02d:  :%02d", tempHour, tempSecond);
    else if (editState == 3 && (millis() / 500) % 2 == 0)
      sprintf(bufTime, "%02d:%02d:  ", tempHour, tempMinute);
    else
      sprintf(bufTime, "%02d:%02d:%02d", tempHour, tempMinute, tempSecond);
  } else {
    sprintf(bufTime, "%02d:%02d:%02d", currentHour, currentMinute,
            currentSecond);
  }
  display->drawStr(26, 13, bufTime);
  char bufDate[14];
  if (isEditing && scheduleSelection == 1) {
    if (editState == 1 && (millis() / 500) % 2 == 0)
      sprintf(bufDate, "  .%02d.%d", tempMonth, tempYear);
    else if (editState == 2 && (millis() / 500) % 2 == 0)
      sprintf(bufDate, "%02d.  .%d", tempDay, tempYear);
    else if (editState == 3 && (millis() / 500) % 2 == 0)
      sprintf(bufDate, "%02d.%02d.    ", tempDay, tempMonth);
    else
      sprintf(bufDate, "%02d.%02d.%d", tempDay, tempMonth, tempYear);
  } else {
    sprintf(bufDate, "%02d.%02d.%d", currentDay, currentMonth, currentYear);
  }
  display->drawStr(25, 30, bufDate);
  if (scheduleSelection == 0)
    display->drawXBMP(108, 4, 4, 7, image_ButtonLeft_bits);
  else
    display->drawXBMP(108, 21, 4, 7, image_ButtonLeft_copy_bits);
  display->drawLine(115, 0, 115, 31);
  display->drawLine(127, 10, 116, 10);
  display->drawLine(127, 21, 116, 21);
  display->drawXBMP(117, 13, 10, 7, image_Layer_12_bits);
  if (!btnBackState)
    display->drawXBMP(117, 1, 10, 8, image_Pin_back_arrow_bits);
  if (!btnNextState)
    display->drawXBMP(120, 24, 5, 7, image_arrow_down_bits);
}

// TESTY
void AquariumAnimation::drawTests(bool btnBackState, bool btnSelectState,
                                  bool btnNextState) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // Ikony
  display->drawXBMP(0, -2, 15, 16, image_test_wind_bits);
  display->drawXBMP(0, 17, 16, 15, image_test_temperature_bits);
  display->drawXBMP(66, -1, 16, 16, image_test_light_bits);
  display->drawXBMP(68, 17, 11, 15, image_test_humidity_white_bits);

  // Linie
  display->drawLine(0, 15, 127, 15);
  display->drawLine(64, 0, 64, 31);
  display->drawLine(17, 31, 17, 0);
  display->drawLine(82, 0, 82, 31);

  // WskaĹşniki
  if (testLight)
    display->drawDisc(91, 7, 3, U8G2_DRAW_ALL);
  else
    display->drawCircle(91, 7, 3, U8G2_DRAW_ALL);
  if (testHeater)
    display->drawDisc(25, 24, 3, U8G2_DRAW_ALL);
  else
    display->drawCircle(25, 24, 3, U8G2_DRAW_ALL);
  if (testFilter)
    display->drawDisc(91, 24, 3, U8G2_DRAW_ALL);
  else
    display->drawCircle(91, 24, 3, U8G2_DRAW_ALL);

  // Napowietrzanie
  display->setFont(u8g2_font_t0_17_tr);
  char buf[6];
  sprintf(buf, "%d%%", testAerationVal);
  display->drawStr(20, 13, buf);

  // Kursor
  int cursorX = 0, cursorY = 0;
  if (testSelection == 0) {
    cursorX = 58;
    cursorY = 4;
  } else if (testSelection == 1) {
    cursorX = 58;
    cursorY = 21;
  } else if (testSelection == 2) {
    cursorX = 122;
    cursorY = 4;
  } else if (testSelection == 3) {
    cursorX = 122;
    cursorY = 21;
  }

  display->drawXBMP(cursorX, cursorY, 4, 7, image_ButtonLeft_bits);

  // Miganie edycji
  if (isEditing && testSelection == 0 && (millis() / 500) % 2 == 0) {
    display->setDrawColor(0);
    display->drawBox(18, 0, 44, 14);
    display->setDrawColor(1);
  }
}

// AccessPoint
void AquariumAnimation::drawAccessPointScreen(const char *apName,
                                              const char *apPass,
                                              const char *ip, uint8_t clients) {
  if (!display)
    return;
  display->setFontMode(1);
  display->setBitmapMode(1);

  // === Ramka ===
  display->drawFrame(0, 0, 128, 32);

  // === Linia pozioma po naglowku ===
  display->drawLine(1, 11, 126, 11);

  // --- NAGLOWEK: "AP" + spinner + paski WiFi + info ---
  display->setFont(u8g2_font_6x10_tr);
  display->drawStr(2, 9, "AP");

  // Spinner
  static const char spinChars[] = {'|', '/', '-', '\\'};
  uint8_t spinIdx = (millis() / 300) % 4;
  char spinBuf[2] = {spinChars[spinIdx], '\0'};
  display->drawStr(18, 9, spinBuf);

  // Paski WiFi (animowane) - zmieniona pozycja
  uint8_t barPhase = (millis() / 500) % 4;
  for (uint8_t b = 0; b < 3; b++) {
    uint8_t bx = 26 + b * 5;
    uint8_t bh = 3 + b * 3; // 3, 6, 9 px
    uint8_t by = 10 - bh;
    bool lit = (b <= barPhase || barPhase == 3);
    if (lit)
      display->drawBox(bx, by, 4, bh);
    else
      display->drawFrame(bx, by, 4, bh);
  }

  // IP na pasku gĂłrnym
  char ipBufHead[18];
  snprintf(ipBufHead, sizeof(ipBufHead), "%.15s", ip ? ip : "");
  display->setFont(u8g2_font_4x6_tr);
  display->drawStr(44, 9, ipBufHead);

  // Klienci (prawa strona naglowka)
  display->setFont(u8g2_font_6x10_tr);
  char clientsBuf[6];
  snprintf(clientsBuf, sizeof(clientsBuf), "[%d]", clients);
  display->drawStr(108, 9, clientsBuf);

  // --- SSID (srodkowy wiersz, font 4x6) ---
  display->setFont(u8g2_font_4x6_tr);
  char ssidBuf[32];
  snprintf(ssidBuf, sizeof(ssidBuf), "N:%.24s", apName ? apName : "");
  display->drawStr(3, 20, ssidBuf);

  // --- HASLO (dolny wiersz, font 4x6) ---
  char passBuf[32];
  snprintf(passBuf, sizeof(passBuf), "H:%.24s",
           apPass ? apPass : ""); // 'H:' dla hasla
  display->drawStr(3, 30, passBuf);
}

// --- METODY OBSĹUGI TESTĂ“W ---
void AquariumAnimation::enterTestMode() {
  testLight = isLightOn;
  testHeater = isHeaterOn;
  testFilter = isFilterOn;
  testAerationVal = 0;
  testSelection = 0;
  isEditing = false;
}

void AquariumAnimation::testNext() {
  if (!isEditing) {
    testSelection++;
    if (testSelection > 3)
      testSelection = 0;
  }
}

void AquariumAnimation::toggleTestOption() {
  if (testSelection == 0) {
    isEditing = !isEditing;
  } else if (testSelection == 1) {
    testHeater = !testHeater;
  } else if (testSelection == 2) {
    testLight = !testLight;
  } else if (testSelection == 3) {
    testFilter = !testFilter;
  }
}

void AquariumAnimation::incrementTestValue() {
  if (testSelection == 0 && isEditing) {
    testAerationVal += 10;
    if (testAerationVal > 100)
      testAerationVal = 0;
  }
}

bool AquariumAnimation::getTestLight() { return testLight; }
bool AquariumAnimation::getTestHeater() { return testHeater; }
bool AquariumAnimation::getTestFilter() { return testFilter; }
uint8_t AquariumAnimation::getTestAeration() { return testAerationVal; }

// EKRAN GĹĂ“WNY (FRAME)
void AquariumAnimation::drawFrame() { HomeRenderer::drawFrame(this); }

