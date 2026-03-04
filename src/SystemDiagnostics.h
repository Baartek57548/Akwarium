#ifndef SYSTEM_DIAGNOSTICS_H
#define SYSTEM_DIAGNOSTICS_H

#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_system.h>

struct SystemDiagSnapshot {
  uint32_t bootCount;
  uint32_t brownoutCount;
  uint32_t wdtCount;
  uint32_t panicCount;
  char lastResetReason[24];
  char lastWakeupCause[24];
};

class SystemDiagnostics {
public:
  static void init(esp_reset_reason_t resetReason,
                   esp_sleep_wakeup_cause_t wakeupCause);
  static SystemDiagSnapshot getSnapshot();

  static const char *resetReasonToString(esp_reset_reason_t reason);
  static const char *wakeupCauseToString(esp_sleep_wakeup_cause_t cause);

private:
  static void persist();
  static SystemDiagSnapshot snapshot;
};

#endif // SYSTEM_DIAGNOSTICS_H
