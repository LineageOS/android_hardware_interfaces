#include "AudioControl.h"

#include <hidl/HidlTransportSupport.h>
#include <log/log.h>


namespace android {
namespace hardware {
namespace automotive {
namespace audiocontrol {
namespace V1_0 {
namespace implementation {


// This is the static map we're using to associate a ContextNumber with a
// bus number from the audio_policy_configuration.xml setup.  Every valid context needs
// to be mapped to a bus address that actually exists in the platforms configuration.
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))  // Would be nice if this were common...
static int sContextToBusMap[] = {
    -1,     // INVALID
     0,     // MUSIC_CONTEXT
     1,     // NAVIGATION_CONTEXT
     2,     // VOICE_COMMAND_CONTEXT
     3,     // CALL_RING_CONTEXT
     4,     // CALL_CONTEXT
     5,     // ALARM_CONTEXT
     6,     // NOTIFICATION_CONTEXT
     7,     // SYSTEM_SOUND_CONTEXT
};
static const unsigned sContextMapSize = ARRAY_SIZE(sContextToBusMap);
static const unsigned sContextCount = sContextMapSize - 1;  // Less one for the INVALID entry
static const unsigned sContextNumberMax = sContextCount;    // contextNumber is counted from 1


AudioControl::AudioControl() {
};


Return<int32_t> AudioControl::getBusForContext(ContextNumber ctxt) {
    unsigned contextNumber = static_cast<unsigned>(ctxt);
    if (contextNumber > sContextNumberMax) {
        ALOGE("Unexpected context number %d (max expected is %d)", contextNumber, sContextCount);
        return -1;
    } else {
        return sContextToBusMap[contextNumber];
    }
}


Return<void> AudioControl::setBalanceTowardRight(float value) {
    // For completeness, lets bounds check the input...
    if ((value > 1.0f) || (value < -1.0f)) {
        ALOGE("Balance value out of range -1 to 1 at %0.2f", value);
    } else {
        // Just log in this default mock implementation
        ALOGI("Balance set to %0.2f", value);
    }
    return Void();
}


Return<void> AudioControl::setFadeTowardFront(float value) {
    // For completeness, lets bounds check the input...
    if ((value > 1.0f) || (value < -1.0f)) {
        ALOGE("Fader value out of range -1 to 1 at %0.2f", value);
    } else {
        // Just log in this default mock implementation
        ALOGI("Fader set to %0.2f", value);
    }
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace audiocontrol
}  // namespace automotive
}  // namespace hardware
}  // namespace android
