#include "SystemDiagnostics.h"

#include <Preferences.h>
#include <string.h>

namespace {

static Preferences diagPrefs;
static const char *DIAG_NAMESPACE = "Diag";

static void copyStringSafe(char *dst, size_t dstSize, const char *src) {
  if (dstSize == 0) {
    return;
  }
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static bool isWdtResetReason(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_INT_WDT:
  case ESP_RST_TASK_WDT:
  case ESP_RST_WDT:
    return true;
#ifdef ESP_RST_RTC_WDT
  case ESP_RST_RTC_WDT:
    return true;
#endif
  default:
    return false;
  }
}

} // namespace

SystemDiagSnapshot SystemDiagnostics::snapshot = {};

const char *SystemDiagnostics::resetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_UNKNOWN:
    return "UNKNOWN";
  case ESP_RST_POWERON:
    return "POWERON";
  case ESP_RST_EXT:
    return "EXT";
  case ESP_RST_SW:
    return "SW";
  case ESP_RST_PANIC:
    return "PANIC";
  case ESP_RST_INT_WDT:
    return "INT_WDT";
  case ESP_RST_TASK_WDT:
    return "TASK_WDT";
  case ESP_RST_WDT:
    return "WDT";
  case ESP_RST_DEEPSLEEP:
    return "DEEPSLEEP";
  case ESP_RST_BROWNOUT:
    return "BROWNOUT";
  case ESP_RST_SDIO:
    return "SDIO";
#ifdef ESP_RST_USB
  case ESP_RST_USB:
    return "USB";
#endif
#ifdef ESP_RST_JTAG
  case ESP_RST_JTAG:
    return "JTAG";
#endif
#ifdef ESP_RST_EFUSE
  case ESP_RST_EFUSE:
    return "EFUSE";
#endif
#ifdef ESP_RST_PWR_GLITCH
  case ESP_RST_PWR_GLITCH:
    return "PWR_GLITCH";
#endif
#ifdef ESP_RST_CPU_LOCKUP
  case ESP_RST_CPU_LOCKUP:
    return "CPU_LOCKUP";
#endif
  default:
    return "UNMAPPED";
  }
}

const char *
SystemDiagnostics::wakeupCauseToString(esp_sleep_wakeup_cause_t cause) {
  switch (cause) {
  case ESP_SLEEP_WAKEUP_UNDEFINED:
    return "UNDEFINED";
  case ESP_SLEEP_WAKEUP_ALL:
    return "ALL";
  case ESP_SLEEP_WAKEUP_EXT0:
    return "EXT0";
  case ESP_SLEEP_WAKEUP_EXT1:
    return "EXT1";
  case ESP_SLEEP_WAKEUP_TIMER:
    return "TIMER";
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    return "TOUCHPAD";
  case ESP_SLEEP_WAKEUP_ULP:
    return "ULP";
  case ESP_SLEEP_WAKEUP_GPIO:
    return "GPIO";
  case ESP_SLEEP_WAKEUP_UART:
    return "UART";
#ifdef ESP_SLEEP_WAKEUP_WIFI
  case ESP_SLEEP_WAKEUP_WIFI:
    return "WIFI";
#endif
#ifdef ESP_SLEEP_WAKEUP_COCPU
  case ESP_SLEEP_WAKEUP_COCPU:
    return "COCPU";
#endif
#ifdef ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG
  case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
    return "COCPU_TRAP";
#endif
  default:
    return "WAKE_OTHER";
  }
}

void SystemDiagnostics::persist() {
  diagPrefs.putUInt("bootCount", snapshot.bootCount);
  diagPrefs.putUInt("brownout", snapshot.brownoutCount);
  diagPrefs.putUInt("wdtCount", snapshot.wdtCount);
  diagPrefs.putUInt("panicCnt", snapshot.panicCount);
  diagPrefs.putString("lastReset", snapshot.lastResetReason);
  diagPrefs.putString("lastWake", snapshot.lastWakeupCause);
}

void SystemDiagnostics::init(esp_reset_reason_t resetReason,
                             esp_sleep_wakeup_cause_t wakeupCause) {
  diagPrefs.begin(DIAG_NAMESPACE, false);

  snapshot.bootCount = diagPrefs.getUInt("bootCount", 0);
  snapshot.brownoutCount = diagPrefs.getUInt("brownout", 0);
  snapshot.wdtCount = diagPrefs.getUInt("wdtCount", 0);
  snapshot.panicCount = diagPrefs.getUInt("panicCnt", 0);

  String lastReset = diagPrefs.getString("lastReset", "UNKNOWN");
  String lastWake = diagPrefs.getString("lastWake", "UNDEFINED");
  copyStringSafe(snapshot.lastResetReason, sizeof(snapshot.lastResetReason),
                 lastReset.c_str());
  copyStringSafe(snapshot.lastWakeupCause, sizeof(snapshot.lastWakeupCause),
                 lastWake.c_str());

  snapshot.bootCount++;
  copyStringSafe(snapshot.lastResetReason, sizeof(snapshot.lastResetReason),
                 resetReasonToString(resetReason));
  copyStringSafe(snapshot.lastWakeupCause, sizeof(snapshot.lastWakeupCause),
                 wakeupCauseToString(wakeupCause));

  if (resetReason == ESP_RST_BROWNOUT) {
    snapshot.brownoutCount++;
  }
  if (isWdtResetReason(resetReason)) {
    snapshot.wdtCount++;
  }
  if (resetReason == ESP_RST_PANIC) {
    snapshot.panicCount++;
  }

  persist();
}

SystemDiagSnapshot SystemDiagnostics::getSnapshot() { return snapshot; }
