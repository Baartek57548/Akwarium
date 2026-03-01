#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "ConfigData.h"

class ConfigManager {
public:
  static void init();
  static void save();
  static void saveConfig(const Config &cfg);
  static void resetToDefault();
  static Config getConfigSnapshot();
  static Config &getConfig();

private:
  static uint32_t calculateCrc32(const Config &cfg);
  static void loadDefaultConfig();
  static Config sysConfig;
};

#endif // CONFIG_MANAGER_H
