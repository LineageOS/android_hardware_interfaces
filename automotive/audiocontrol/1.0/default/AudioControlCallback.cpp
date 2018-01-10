#include "AudioControlCallback.h"

namespace android {
namespace hardware {
namespace automotive {
namespace audiocontrol {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::automotive::audiocontrol::V1_0::IAudioControlCallback follow.
Return<void> AudioControlCallback::suggestPausePlayers() {
    // TODO implement in framework (this is called by the HAL implementation when needed)
    return Void();
}

Return<void> AudioControlCallback::suggestStopPlayers() {
    // TODO implement in framework (this is called by the HAL implementation when needed)
    return Void();
}

Return<void> AudioControlCallback::resumePlayers() {
    // TODO implement in framework (this is called by the HAL implementation when needed)
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace audiocontrol
}  // namespace automotive
}  // namespace hardware
}  // namespace android
