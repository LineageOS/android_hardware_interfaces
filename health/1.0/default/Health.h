#ifndef HIDL_GENERATED_android_hardware_health_V1_0_Health_H_
#define HIDL_GENERATED_android_hardware_health_V1_0_Health_H_

#include <android/hardware/health/1.0/IHealth.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <healthd/healthd.h>
#include <utils/String8.h>

namespace android {
namespace hardware {
namespace health {
namespace V1_0 {
namespace implementation {

using ::android::hardware::health::V1_0::HealthInfo;
using ::android::hardware::health::V1_0::HealthConfig;
using ::android::hardware::health::V1_0::IHealth;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Health : public IHealth {
    // Methods from ::android::hardware::health::V1_0::IHealth follow.
    Return<void> init(const HealthConfig& config, init_cb _hidl_cb)  override;
    Return<void> update(const HealthInfo& info, update_cb _hidl_cb)  override;
    Return<void> energyCounter(energyCounter_cb _hidl_cb) override;
private:
    std::function<int(int64_t *)> mGetEnergyCounter;
};

extern "C" IHealth* HIDL_FETCH_IHealth(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_health_V1_0_Health_H_
