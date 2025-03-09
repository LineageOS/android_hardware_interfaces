/*
 * Copyright (C) 2024 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUDIOCONTROL_INTERNAL_CONFIGURATION_UTILS_H
#define ANDROID_HARDWARE_AUDIOCONTROL_INTERNAL_CONFIGURATION_UTILS_H

#include <string>

#include <aidl/android/hardware/automotive/audiocontrol/AudioZoneContext.h>
#include <aidl/android/hardware/automotive/audiocontrol/AudioZoneContextInfo.h>

namespace android {
namespace hardware {
namespace audiocontrol {
namespace internal {

::aidl::android::hardware::automotive::audiocontrol::AudioZoneContext getDefaultCarAudioContext();

}  // namespace internal
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIOCONTROL_INTERNAL_CONFIGURATION_UTILS_H
