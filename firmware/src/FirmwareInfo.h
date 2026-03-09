#ifndef FIRMWARE_INFO_H
#define FIRMWARE_INFO_H

#include <stddef.h>

#ifndef AQUARIUM_FIRMWARE_NAME
#define AQUARIUM_FIRMWARE_NAME "Aquarium Controller"
#endif

#ifndef AQUARIUM_FIRMWARE_VERSION
#define AQUARIUM_FIRMWARE_VERSION "dev"
#endif

#ifndef AQUARIUM_FIRMWARE_BUILD_REF
#define AQUARIUM_FIRMWARE_BUILD_REF "local"
#endif

struct FirmwareRuntimeInfo {
  const char *firmwareName;
  const char *firmwareVersion;
  const char *buildRef;
  const char *buildDate;
  const char *buildTime;
  const char *idfVersion;
  const char *runningPartitionLabel;
  const char *bootPartitionLabel;
  const char *nextPartitionLabel;
  size_t nextPartitionSizeBytes;
};

class FirmwareInfo {
public:
  static FirmwareRuntimeInfo getRuntimeInfo();
  static bool supportsBleOta();
  static bool supportsHttpOta();
};

#endif // FIRMWARE_INFO_H
