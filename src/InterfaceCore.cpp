#include "InterfaceCore.h"

#include "ConfigManager.h"
#include "LogManager.h"
#include "SystemController.h"

namespace InterfaceCore {

InterfaceRuleResult applyConfigPatchAndSave(const ConfigPatch &patch,
                                            uint8_t parseInvalidFields) {
  InterfaceRuleResult result = {};

  Config cfg = ConfigManager::getCopy();
  ConfigValidationResult validation = {};
  ConfigValidation::applyPatchAndClamp(cfg, patch, validation);

  if (!validation.hasAnyApplied()) {
    result.code = InterfaceRuleCode::INVALID_VALUE;
    return result;
  }

  if (!ConfigManager::updateAndSave(cfg)) {
    result.code = InterfaceRuleCode::SAVE_FAILED;
    return result;
  }

  if (validation.hasInvalidFields() || parseInvalidFields > 0) {
    result.code = InterfaceRuleCode::OK_PARTIAL;
  } else {
    result.code = InterfaceRuleCode::OK;
  }

  return result;
}

InterfaceRuleResult setManualServoAngle(long angle) {
  InterfaceRuleResult result = {};
  if (angle < 0 || angle > 90) {
    result.code = InterfaceRuleCode::INVALID_VALUE;
    return result;
  }

  SystemController::setManualServo(static_cast<int>(angle));
  result.code = InterfaceRuleCode::OK;
  return result;
}

void clearManualServo() { SystemController::clearManualServo(); }

void triggerFeedNow() { SystemController::feedNow(); }

void clearCriticalLogs() { LogManager::clearCriticalLogs(); }

} // namespace InterfaceCore
