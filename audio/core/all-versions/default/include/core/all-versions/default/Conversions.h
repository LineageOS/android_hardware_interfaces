/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <common/all-versions/IncludeGuard.h>

#include <string>

#include <system/audio.h>

namespace android {
namespace hardware {
namespace audio {
namespace AUDIO_HAL_VERSION {
namespace implementation {

using ::android::hardware::audio::AUDIO_HAL_VERSION::DeviceAddress;

std::string deviceAddressToHal(const DeviceAddress& address);

}  // namespace implementation
}  // namespace AUDIO_HAL_VERSION
}  // namespace audio
}  // namespace hardware
}  // namespace android
