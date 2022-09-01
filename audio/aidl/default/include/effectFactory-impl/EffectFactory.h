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

#pragma once

#include <optional>
#include <vector>

#include <aidl/android/hardware/audio/effect/BnFactory.h>

namespace aidl::android::hardware::audio::effect {

class Factory : public BnFactory {
  public:
    Factory();
    /**
     * @brief Get identity of all effects supported by the device, with the optional filter by type
     * and/or by instance UUID.
     *
     * @param in_type Type UUID.
     * @param in_instance Instance UUID.
     * @param out_descriptor List of identities .
     * @return ndk::ScopedAStatus
     */
    ndk::ScopedAStatus queryEffects(
            const std::optional<::aidl::android::media::audio::common::AudioUuid>& in_type,
            const std::optional<::aidl::android::media::audio::common::AudioUuid>& in_instance,
            std::vector<Descriptor::Identity>* out_descriptor) override;

  private:
    // List of effect descriptors supported by the devices.
    std::vector<Descriptor::Identity> mIdentityList;
};
}  // namespace aidl::android::hardware::audio::effect
