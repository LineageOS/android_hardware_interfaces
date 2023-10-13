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

#include "core-impl/Module.h"

namespace aidl::android::hardware::audio::core {

// This class is intended to be used as a base class for implementations
// that use TinyAlsa. This can be either a primary module or a USB Audio
// module. This class does not define a complete module implementation,
// and should never be used on its own. Derived classes are expected to
// provide necessary overrides for all interface methods omitted here.
class ModuleAlsa : public Module {
  public:
    explicit ModuleAlsa(Module::Type type) : Module(type) {}

  protected:
    // Extension methods of 'Module'.
    ndk::ScopedAStatus populateConnectedDevicePort(
            ::aidl::android::media::audio::common::AudioPort* audioPort) override;
};

}  // namespace aidl::android::hardware::audio::core
