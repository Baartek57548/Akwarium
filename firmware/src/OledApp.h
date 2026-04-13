#ifndef OLED_APP_H
#define OLED_APP_H

class AquariumAnimation;

namespace OledApp {

void suppressUiTimeSyncForManualFeed(unsigned long nowMs,
                                     AquariumAnimation *animation);
void captureUiChanges(AquariumAnimation *animation, bool allowTimeUpdate);
void applyPendingUiChanges();
void consumePendingUiSaveConfirmationAnimation(AquariumAnimation *animation);

} // namespace OledApp

#endif // OLED_APP_H
