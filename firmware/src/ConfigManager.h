#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "ConfigData.h"

enum class ConfigSaveStatus : uint8_t {
  Ok = 0,
  OkVerifiedAfterWriteMismatch = 1,
  LockTimeout = 2,
  WriteFailed = 3,
  VerifyReadFailed = 4,
  VerifyMismatch = 5
};

struct ConfigSaveResult {
  bool ok = false;
  bool sanitizedChanged = false;
  size_t bytesWritten = 0;
  size_t bytesReadBack = 0;
  ConfigSaveStatus status = ConfigSaveStatus::WriteFailed;
  Config appliedConfig = {};
};

class ConfigManager {
public:
  static void init();
  static void save();
  static bool updateAndSave(const Config &cfg);
  static ConfigSaveResult updateAndSaveDetailed(const Config &cfg);
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
