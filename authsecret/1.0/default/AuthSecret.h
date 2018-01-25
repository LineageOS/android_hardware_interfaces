#ifndef ANDROID_HARDWARE_AUTHSECRET_V1_0_AUTHSECRET_H
#define ANDROID_HARDWARE_AUTHSECRET_V1_0_AUTHSECRET_H

#include <android/hardware/authsecret/1.0/IAuthSecret.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace authsecret {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct AuthSecret : public IAuthSecret {
    // Methods from ::android::hardware::authsecret::V1_0::IAuthSecret follow.
    Return<void> primaryUserCredential(const hidl_vec<uint8_t>& secret) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace authsecret
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUTHSECRET_V1_0_AUTHSECRET_H
