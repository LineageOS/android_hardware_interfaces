#ifndef ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROLCALLBACK_H
#define ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROLCALLBACK_H

#include <android/hardware/automotive/audiocontrol/1.0/IAudioControlCallback.h>
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

// TODO:  Move this into packages/services/Car...
struct AudioControlCallback : public IAudioControlCallback {
    // Methods from ::android::hardware::automotive::audiocontrol::V1_0::IAudioControlCallback follow.
    Return<void> suggestPausePlayers() override;
    Return<void> suggestStopPlayers() override;
    Return<void> resumePlayers() override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace audiocontrol
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V1_0_AUDIOCONTROLCALLBACK_H
