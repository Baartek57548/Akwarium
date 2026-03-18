#include "SystemControllerCompat.h"

int SystemController::getLastResetReason() { return 0; }

const char *SystemController::getLastResetLabel() { return nullptr; }
