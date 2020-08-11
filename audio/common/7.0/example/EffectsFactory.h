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

#pragma once

#include <android/hardware/audio/effect/7.0/IEffectsFactory.h>

namespace android::hardware::audio::effect::V7_0::implementation {

class EffectsFactory : public IEffectsFactory {
  public:
    EffectsFactory() = default;

    ::android::hardware::Return<void> getAllDescriptors(getAllDescriptors_cb _hidl_cb) override;
    ::android::hardware::Return<void> getDescriptor(
            const ::android::hardware::audio::common::V7_0::Uuid& uuid,
            getDescriptor_cb _hidl_cb) override;
    ::android::hardware::Return<void> createEffect(
            const ::android::hardware::audio::common::V7_0::Uuid& uuid, int32_t session,
            int32_t ioHandle, int32_t device, createEffect_cb _hidl_cb) override;
    ::android::hardware::Return<void>
    debug(const ::android::hardware::hidl_handle& fd,
          const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;
};

}  // namespace android::hardware::audio::effect::V7_0::implementation
