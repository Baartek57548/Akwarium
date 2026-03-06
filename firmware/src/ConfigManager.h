#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "ConfigData.h"

class ConfigManager {
public:
  static void init();
  static void save();
  static bool updateAndSave(const Config &cfg);
  static Config getCopy();

  // Compatibility wrappers for older call sites.
  static void saveConfig(const Config &cfg);
  static Config getConfigSnapshot();
  static void resetToDefault();

private:
  static uint32_t calculateCrc32(const Config &cfg);
  static void loadDefaultConfig();
  static Config sysConfig;
};

#endif // CONFIG_MANAGER_H
