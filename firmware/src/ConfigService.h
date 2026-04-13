#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "ConfigManager.h"
#include "ConfigValidation.h"

enum class ConfigApplyStatus : uint8_t {
  Ok = 0,
  ValidationFailed = 1,
  SaveFailed = 2
};

struct ConfigApplyResult {
  bool ok = false;
  bool partial = false;
  ConfigApplyStatus status = ConfigApplyStatus::ValidationFailed;
  ConfigValidationResult validation = {};
  ConfigSaveResult saveResult = {};
  Config appliedConfig = {};
  char responseCode[40] = "";
};

namespace ConfigService {
ConfigApplyResult applyPatch(const ConfigPatch &patch);
}

#endif // CONFIG_SERVICE_H
