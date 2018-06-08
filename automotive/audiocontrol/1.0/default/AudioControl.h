#ifndef ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROL_H
#define ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROL_H

#include <android/hardware/automotive/audiocontrol/1.0/IAudioControl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace automotive {
namespace audiocontrol {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct AudioControl : public IAudioControl {
public:
    // Methods from ::android::hardware::automotive::audiocontrol::V1_0::IAudioControl follow.
    Return<int32_t> getBusForContext(ContextNumber contextNumber) override;
    Return<void> setBalanceTowardRight(float value) override;
    Return<void> setFadeTowardFront(float value) override;

    // Implementation details
    AudioControl();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace audiocontrol
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROL_H
