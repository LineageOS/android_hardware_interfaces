/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V2_0_AUDIOCONTROL_H
#define ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V2_0_AUDIOCONTROL_H

#include <android/hardware/audio/common/6.0/types.h>
#include <android/hardware/automotive/audiocontrol/2.0/IAudioControl.h>
#include <android/hardware/automotive/audiocontrol/2.0/ICloseHandle.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

using android::hardware::audio::common::V6_0::AudioUsage;

namespace android::hardware::automotive::audiocontrol::V2_0::implementation {

class AudioControl : public IAudioControl {
  public:
    // Methods from ::android::hardware::automotive::audiocontrol::V2_0::IAudioControl follow.
    Return<sp<ICloseHandle>> registerFocusListener(const sp<IFocusListener>& listener);
    Return<void> onAudioFocusChange(hidl_bitfield<AudioUsage> usage, int zoneId,
                                    hidl_bitfield<AudioFocusChange> focusChange);
    Return<void> setBalanceTowardRight(float value) override;
    Return<void> setFadeTowardFront(float value) override;

    // Implementation details
    AudioControl();

  private:
    sp<IFocusListener> mFocusListener;
    static bool isValidValue(float value) { return (value > 1.0f) || (value < -1.0f); }
};

}  // namespace android::hardware::automotive::audiocontrol::V2_0::implementation

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_AUDIOCONTROL_V2_0_AUDIOCONTROL_H
