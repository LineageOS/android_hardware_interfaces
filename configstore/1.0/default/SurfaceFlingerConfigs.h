#ifndef ANDROID_HARDWARE_CONFIGSTORE_V1_0_SURFACEFLINGERCONFIGS_H
#define ANDROID_HARDWARE_CONFIGSTORE_V1_0_SURFACEFLINGERCONFIGS_H

#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_0 {
namespace implementation {

using ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs;
using ::android::hardware::configstore::V1_0::OptionalBool;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct SurfaceFlingerConfigs : public ISurfaceFlingerConfigs {
    // Methods from ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs follow.
    Return<void> vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) override;
    Return<void> useTripleFramebuffer(useTripleFramebuffer_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace configstore
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONFIGSTORE_V1_0_SURFACEFLINGERCONFIGS_H
