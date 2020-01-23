#include "AudioControl.h"

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include "CloseHandle.h"

namespace android::hardware::automotive::audiocontrol::V2_0::implementation {

AudioControl::AudioControl() {}

Return<sp<ICloseHandle>> AudioControl::registerFocusListener(const sp<IFocusListener>& listener) {
    LOG(DEBUG) << "registering focus listener";
    sp<ICloseHandle> closeHandle(nullptr);

    if (listener) {
        mFocusListener = listener;

        closeHandle = new CloseHandle([this, listener]() {
            if (mFocusListener == listener) {
                mFocusListener = nullptr;
            }
        });
    } else {
        LOG(ERROR) << "Unexpected nullptr for listener resulting in no-op.";
    }

    return closeHandle;
}

Return<void> AudioControl::setBalanceTowardRight(float value) {
    // For completeness, lets bounds check the input...
    if (isValidValue(value)) {
        LOG(ERROR) << "Balance value out of range -1 to 1 at " << value;
    } else {
        // Just log in this default mock implementation
        LOG(INFO) << "Balance set to " << value;
    }
    return Void();
}

Return<void> AudioControl::setFadeTowardFront(float value) {
    // For completeness, lets bounds check the input...
    if (isValidValue(value)) {
        LOG(ERROR) << "Fader value out of range -1 to 1 at " << value;
    } else {
        // Just log in this default mock implementation
        LOG(INFO) << "Fader set to " << value;
    }
    return Void();
}

Return<void> AudioControl::onAudioFocusChange(hidl_bitfield<AudioUsage> usage, int zoneId,
                                              hidl_bitfield<AudioFocusChange> focusChange) {
    LOG(INFO) << "Focus changed: " << static_cast<int>(focusChange) << " for usage "
              << static_cast<int>(usage) << " in zone " << zoneId;
    return Void();
}

}  // namespace android::hardware::automotive::audiocontrol::V2_0::implementation
