#ifndef AQUARIUM_ANIMATION_H
#define AQUARIUM_ANIMATION_H

#include <U8g2lib.h>

struct LogEntry {
  char message[20];
  char time[6];
};

// Struktury do animacji proceduralnej (rybki)
struct Fish {
  float x, y;
  int dir;
  float speed;
  float targetY;
};

struct Food {
  int x, y;
  bool visible;
};

struct Bubble {
  float x, y;
  float size;
  bool active;
  bool popping;
};

class AquariumAnimation {
  friend class HomeRenderer;
  friend class MenuRenderer;
  friend class ScheduleRenderer;
  friend class TestRenderer;
  friend class OtaRenderer;

private:
  U8G2 *display;

  // Bufory tekstowe
  char timeBuffer[16];
  char tempBuffer[16];
  char dateBuffer[16];
  char valBuffer[10];
  char batteryVoltageBuffer[10];
  char feedTimeBuffer[6];

  uint8_t batteryPercent;

  // Statusy urzÄ…dzeĹ„
  bool isFilterOn;
  bool isLightOn;
  bool isHeaterOn;

  // Harmonogram Karmienia
  uint8_t feedFreq; // 0=OFF, 1=Codziennie, 2=Co 2 dni, 3=Co 3 dni
  uint8_t feedDaysPassed;
  uint8_t feedHour, feedMinute; // Godzina karmienia

  // Animacja tĹ‚a (podstawowa)
  bool isFeedingAnim;
  unsigned long feedAnimStart;

// --- ZMIENNE ANIMACJI KARMIENIA (Proceduralnej) ---
#define NUM_FISH 3
#define NUM_FOOD 5
#define NUM_BUBBLES 8

  Fish fish[NUM_FISH];
  Food food[NUM_FOOD];
  Bubble bubbles[NUM_BUBBLES];

  bool feedingActive;
  unsigned long lastFeedTime;
  unsigned long bubbleTimer;
  int waveOffset;

  // Fizyka (stara - czÄ…steczki tĹ‚a)
  struct FoodParticle {
    float x, y, speed;
    bool active;
  };
  FoodParticle particles[10];
  float fish1_x, fish2_x;
  int fish1_dir, fish2_dir;
  void updatePhysics();

  // Nawigacja
  uint8_t menuSelection;
  uint8_t menuScrollOffset;
  uint8_t scheduleSelection;

  // Edycja
  bool isEditing;
  uint8_t editState;
  uint8_t tempHour, tempMinute, tempSecond;
  uint8_t tempDay, tempMonth;
  uint16_t tempYear;

  // Dane harmonogramow (swiatlo, filtr, temperatura)
  uint8_t scheduleHourOn, scheduleMinOn;
  uint8_t scheduleHourOff, scheduleMinOff;
  uint8_t aerationHourOn, aerationMinOn;
  uint8_t aerationHourOff, aerationMinOff;
  uint8_t filterHourOn, filterMinOn;
  uint8_t filterHourOff, filterMinOff;
  uint8_t targetTemp;

  // Czas systemowy
  uint8_t currentHour, currentMinute, currentSecond;
  uint8_t currentDay, currentMonth;
  uint16_t currentYear;

  uint8_t activeScheduleId;
  bool scheduleChangePending;
  bool timeChangePending;

  // Logi
  LogEntry logs[20];
  uint8_t logCount;
  uint8_t logScroll;

  // Testy
  uint8_t testSelection;
  bool testLight;
  bool testHeater;
  bool testFilter;
  uint8_t testAerationVal;

  // Animacja zapisu (non-blocking, renderowana w gĹ‚Ăłwnej pÄ™tli rysowania)
  bool confirmAnimActive;
  uint16_t confirmAnimFrame;
  unsigned long confirmAnimLastStep;

  // --- METODY POMOCNICZE ANIMACJI ---
  void initFishObjs();
  void dropFood();
  void drawFishObj(int x, int y, int dir);
  void drawBubbles();
  void moveFish();
  void drawFood();
  void drawWaves(int offset);

public:
  AquariumAnimation(U8G2 *u8g2_instance);

  // Settery
  void setTime(int hour, int minute, int second);
  void setDate(int day, int month, int year);
  void setTemperature(float temp);
  void setAeration(uint8_t percent);
  void setBattery(uint8_t percent);
  void setBatteryVoltage(float voltage);
  void setFilterStatus(bool status);
  void setLightStatus(bool status);
  void setHeaterStatus(bool status);
  void setFeedingSchedule(const char *time, uint8_t frequency,
                          uint8_t daysPassed);
  void setFeedingAnimation(bool active);

  // Rysowanie EKRANĂ“W
  void drawFrame(); // Ekran GĹ‚Ăłwny
  void drawMenu(bool btnBack, bool btnSelect, bool btnNext);
  void drawSchedule(bool btnBack, bool btnSelect, bool btnNext); // ĹšwiatĹ‚o
  void drawScheduleAeration(bool btnBack, bool btnSelect,
                            bool btnNext); // Napowietrzanie
  void drawScheduleFilter(bool btnBack, bool btnSelect, bool btnNext);
  inline void drawScheduleCO2(bool btnBack, bool btnSelect, bool btnNext) {
    drawScheduleFilter(btnBack, btnSelect, btnNext);
  }
  void drawScheduleTemp(bool btnBack, bool btnSelect, bool btnNext); // Temp

  // NOWE: Ekran ustawieĹ„ karmienia
  void drawScheduleFeeding(bool btnBack, bool btnSelect, bool btnNext);

  void drawSettingsDateTime(bool btnBack, bool btnSelect, bool btnNext);
  void drawLogs(bool btnBack, bool btnSelect, bool btnNext);
  void drawTests(bool btnBack, bool btnSelect, bool btnNext);
  void drawAccessPointScreen(const char *apName, const char *apPass,
                             const char *ip, uint8_t clients);

  // Rysowanie samej animacji proceduralnej (Akcja karmienia)
  void drawFeedingScreen();

  // Nawigacja
  void menuNext();
  uint8_t getMenuSelection();
  void scheduleNext();
  void setActiveScheduleId(uint8_t id);
  uint8_t getScheduleSelection();
  void setLightSchedule(uint8_t hourOn, uint8_t minuteOn, uint8_t hourOff,
                        uint8_t minuteOff);
  void setAerationSchedule(uint8_t hourOn, uint8_t minuteOn, uint8_t hourOff,
                           uint8_t minuteOff);
  void setFilterSchedule(uint8_t hourOn, uint8_t minuteOn, uint8_t hourOff,
                         uint8_t minuteOff);
  inline void setCO2Schedule(uint8_t hourOn, uint8_t minuteOn, uint8_t hourOff,
                             uint8_t minuteOff) {
    setFilterSchedule(hourOn, minuteOn, hourOff, minuteOff);
  }
  void setTargetTempSetting(uint8_t value);
  void logScrollNext();
  void addLog(const char *message, const char *time);
  void clearLogs();
  void scheduleEditIncrement();
  void startEditing();
  void nextEditStep();
  bool isEditingActive();

  // Animacje
  void playConfirmAnimation();
  bool drawConfirmAnimationFrame();
  void initFeeding();

  // Metody TestĂłw
  void enterTestMode();
  void testNext();
  void toggleTestOption();
  void incrementTestValue();
  bool getTestLight();
  bool getTestHeater();
  bool getTestFilter();
  uint8_t getTestAeration();

  // Gettery
  bool hasScheduleChanged();
  bool hasTimeChanged();
  uint8_t getScheduleHourOn();
  uint8_t getScheduleMinOn();
  uint8_t getScheduleHourOff();
  uint8_t getScheduleMinOff();
  uint8_t getAerationHourOn();
  uint8_t getAerationMinOn();
  uint8_t getAerationHourOff();
  uint8_t getAerationMinOff();
  uint8_t getFilterHourOn();
  uint8_t getFilterMinOn();
  uint8_t getFilterHourOff();
  uint8_t getFilterMinOff();
  inline uint8_t getCO2HourOn() { return getFilterHourOn(); }
  inline uint8_t getCO2MinOn() { return getFilterMinOn(); }
  inline uint8_t getCO2HourOff() { return getFilterHourOff(); }
  inline uint8_t getCO2MinOff() { return getFilterMinOff(); }
  uint8_t getTargetTemp();

  // Gettery Karmienia (do synchronizacji z main)
  uint8_t getFeedHour();
  uint8_t getFeedMinute();
  uint8_t getFeedFreq();

  uint8_t getNewHour();
  uint8_t getNewMinute();
  uint8_t getNewSecond();
  uint8_t getNewDay();
  uint8_t getNewMonth();
  uint16_t getNewYear();
};

#endif
