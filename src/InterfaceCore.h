#ifndef INTERFACE_CORE_H
#define INTERFACE_CORE_H

#include "ConfigValidation.h"

enum class InterfaceRuleCode {
  OK = 0,
  OK_PARTIAL,
  INVALID_VALUE,
  SAVE_FAILED
};

struct InterfaceRuleResult {
  InterfaceRuleCode code = InterfaceRuleCode::INVALID_VALUE;

  bool isOk() const {
    return code == InterfaceRuleCode::OK || code == InterfaceRuleCode::OK_PARTIAL;
  }

  bool isPartial() const { return code == InterfaceRuleCode::OK_PARTIAL; }
};

namespace InterfaceCore {

InterfaceRuleResult applyConfigPatchAndSave(const ConfigPatch &patch,
                                            uint8_t parseInvalidFields = 0);

InterfaceRuleResult setManualServoAngle(long angle);

void clearManualServo();
void triggerFeedNow();
void clearCriticalLogs();

} // namespace InterfaceCore

#endif // INTERFACE_CORE_H
