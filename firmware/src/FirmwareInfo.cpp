#include "FirmwareInfo.h"

#include <esp_ota_ops.h>

namespace {

#ifndef IDF_VER
#define IDF_VER "unknown"
#endif

// This marker is embedded into the firmware image so native clients can read
// package metadata from a .bin file before OTA starts.
__attribute__((used)) static const char AQUARIUM_FIRMWARE_BINARY_METADATA[] =
    "AQFWMETA|project=" AQUARIUM_FIRMWARE_NAME
    "|version=" AQUARIUM_FIRMWARE_VERSION
    "|ref=" AQUARIUM_FIRMWARE_BUILD_REF
    "|date=" __DATE__
    "|time=" __TIME__
    "|idf=" IDF_VER;

const char *safeLabel(const esp_partition_t *partition) {
  return (partition != nullptr && partition->label[0] != '\0') ? partition->label
                                                                : "-";
}

} // namespace

FirmwareRuntimeInfo FirmwareInfo::getRuntimeInfo() {
  const esp_app_desc_t *appDesc = esp_ota_get_app_description();
  const esp_partition_t *runningPartition = esp_ota_get_running_partition();
  const esp_partition_t *bootPartition = esp_ota_get_boot_partition();
  const esp_partition_t *nextPartition = esp_ota_get_next_update_partition(nullptr);

  FirmwareRuntimeInfo info = {};
  info.firmwareName = AQUARIUM_FIRMWARE_NAME;
  info.firmwareVersion = AQUARIUM_FIRMWARE_VERSION;
  info.buildRef = AQUARIUM_FIRMWARE_BUILD_REF;
  info.buildDate = (appDesc != nullptr && appDesc->date[0] != '\0') ? appDesc->date : __DATE__;
  info.buildTime = (appDesc != nullptr && appDesc->time[0] != '\0') ? appDesc->time : __TIME__;
  info.idfVersion =
      (appDesc != nullptr && appDesc->idf_ver[0] != '\0') ? appDesc->idf_ver : IDF_VER;
  info.runningPartitionLabel = safeLabel(runningPartition);
  info.bootPartitionLabel = safeLabel(bootPartition);
  info.nextPartitionLabel = safeLabel(nextPartition);
  info.nextPartitionSizeBytes =
      nextPartition != nullptr ? static_cast<size_t>(nextPartition->size) : 0U;
  return info;
}

bool FirmwareInfo::supportsBleOta() { return true; }

bool FirmwareInfo::supportsHttpOta() { return true; }
