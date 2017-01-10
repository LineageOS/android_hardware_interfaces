#ifndef ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H
#define ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H

#include <android/hardware/gnss/1.0/IGnssBatching.h>
#include <hardware/fused_location.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>


namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssBatching;
using ::android::hardware::gnss::V1_0::IGnssBatchingCallback;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct GnssBatching : public IGnssBatching {
    GnssBatching(const FlpLocationInterface* flpLocationIface);

    // Methods from ::android::hardware::gnss::V1_0::IGnssBatching follow.
    Return<bool> init(const sp<IGnssBatchingCallback>& callback) override;
    Return<uint16_t> getBatchSize() override;
    Return<bool> start(const IGnssBatching::Options& options ) override;
    Return<void> flush() override;
    Return<bool> stop() override;
    Return<void> cleanup() override;

 private:
    const FlpLocationInterface* mFlpLocationIface = nullptr;
};

extern "C" IGnssBatching* HIDL_FETCH_IGnssBatching(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V1_0_GNSSBATCHING_H
