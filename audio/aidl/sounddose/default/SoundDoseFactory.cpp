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

#define LOG_TAG "AHAL_SoundDoseFactory"

#include "SoundDoseFactory.h"

#include <android-base/logging.h>
#include <core-impl/SoundDose.h>

namespace aidl::android::hardware::audio::sounddose {

using ::aidl::android::hardware::audio::core::sounddose::SoundDose;

ndk::ScopedAStatus SoundDoseFactory::getSoundDose(const std::string& in_module,
                                                  std::shared_ptr<ISoundDose>* _aidl_return) {
    auto soundDoseIt = mSoundDoseBinderMap.find(in_module);
    if (soundDoseIt != mSoundDoseBinderMap.end()) {
        *_aidl_return = ISoundDose::fromBinder(soundDoseIt->second);

        LOG(DEBUG) << __func__
                   << ": returning cached instance of ISoundDose: " << _aidl_return->get()
                   << " for module " << in_module;
        return ndk::ScopedAStatus::ok();
    }

    auto soundDose = ndk::SharedRefBase::make<SoundDose>();
    mSoundDoseBinderMap[in_module] = soundDose->asBinder();
    *_aidl_return = soundDose;

    LOG(DEBUG) << __func__ << ": returning new instance of ISoundDose: " << _aidl_return->get()
               << " for module " << in_module;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::sounddose
