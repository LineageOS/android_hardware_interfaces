/*
 * Copyright (C) 2022 The Android Open Source Project
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
#define LOG_TAG "AHAL_Module"
#include <android-base/logging.h>

#include "core-impl/Config.h"

namespace aidl::android::hardware::audio::core {
ndk::ScopedAStatus Config::getSurroundSoundConfig(SurroundSoundConfig* _aidl_return) {
    SurroundSoundConfig surroundSoundConfig;
    // TODO: parse from XML; for now, use empty config as default
    *_aidl_return = std::move(surroundSoundConfig);
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::audio::core
