#include "ConfigService.h"

#include <string.h>

namespace {

static void setResponseCode(ConfigApplyResult &result, const char *code) {
  snprintf(result.responseCode, sizeof(result.responseCode), "%s",
           (code != nullptr && code[0] != '\0') ? code : "unknown");
}

} // namespace

ConfigApplyResult ConfigService::applyPatch(const ConfigPatch &patch) {
  ConfigApplyResult result = {};

  Config cfg = ConfigManager::getCopy();
  if (!ConfigValidation::applyRuntimePatch(cfg, patch, result.validation)) {
    result.appliedConfig = ConfigManager::getCopy();
    result.status = ConfigApplyStatus::ValidationFailed;
    setResponseCode(result,
                    result.validation.errorCode[0] != '\0'
                        ? result.validation.errorCode
                        : "invalid_values");
    return result;
  }

  result.saveResult = ConfigManager::updateAndSaveDetailed(cfg);
  result.appliedConfig = result.saveResult.appliedConfig;
  result.partial = result.validation.hasInvalidFields() ||
                   result.saveResult.sanitizedChanged;

  if (!result.saveResult.ok) {
    result.status = ConfigApplyStatus::SaveFailed;
    setResponseCode(result, "save_failed");
    return result;
  }

  result.ok = true;
  result.status = ConfigApplyStatus::Ok;
  setResponseCode(result, result.partial ? "settings_partial"
                                         : "settings_saved");
  return result;
}
