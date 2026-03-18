#ifndef SYSTEM_CONTROLLER_COMPAT_H
#define SYSTEM_CONTROLLER_COMPAT_H

class SystemController {
public:
  static int getLastResetReason();
  static const char *getLastResetLabel();
};

#endif
