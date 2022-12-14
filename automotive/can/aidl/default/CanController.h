/*
 * Copyright 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <aidl/android/hardware/automotive/can/BnCanController.h>

#include "CanBus.h"

#include <aidl/android/hardware/automotive/can/Result.h>

#include <map>
#include <string>

namespace aidl::android::hardware::automotive::can {

class CanController : public BnCanController {
  public:
    ndk::ScopedAStatus getSupportedInterfaceTypes(
            std::vector<InterfaceType>* supportedTypes) override;

    ndk::ScopedAStatus getInterfaceName(const std::string& busName,
                                        std::string* ifaceName) override;

    ndk::ScopedAStatus upBus(const BusConfig& config, std::string* ifaceName) override;

    ndk::ScopedAStatus downBus(const std::string& busName) override;

  private:
    std::map<std::string, std::unique_ptr<CanBus>> mBusesByName = {};
};
}  // namespace aidl::android::hardware::automotive::can
