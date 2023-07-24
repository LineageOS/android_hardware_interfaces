/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

#include "alsa/Mixer.h"

namespace aidl::android::hardware::audio::core::primary {

class PrimaryMixer : public alsa::Mixer {
  public:
    static constexpr int kAlsaCard = 0;
    static constexpr int kAlsaDevice = 0;

    static PrimaryMixer& getInstance();

  private:
    PrimaryMixer() : alsa::Mixer(kAlsaCard) {}
};

}  // namespace aidl::android::hardware::audio::core::primary
