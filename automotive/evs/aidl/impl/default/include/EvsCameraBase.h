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

#include <aidl/android/hardware/automotive/evs/BnEvsCamera.h>

namespace aidl::android::hardware::automotive::evs::implementation {

class EvsCameraBase : public evs::BnEvsCamera {
  private:
    using Base = evs::BnEvsCamera;
    using Self = EvsCameraBase;

  public:
    using Base::Base;

    ~EvsCameraBase() override = default;

    virtual void shutdown() = 0;

  protected:
    // This is used for the derived classes and it prevents constructors from direct access
    // while it allows this class to be instantiated via ndk::SharedRefBase::make<>.
    struct Sigil {
        explicit Sigil() = default;
    };
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
