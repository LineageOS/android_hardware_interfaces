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

#include <OffloadControlTestV1_0.h>
#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>

class OffloadControlTestV1_1_HalNotStarted : public OffloadControlTestV1_0_HalNotStarted {
  public:
    virtual sp<android::hardware::tetheroffload::control::V1_0::IOffloadControl> createControl(
            const std::string& serviceName) override {
        return android::hardware::tetheroffload::control::V1_1::IOffloadControl::getService(
                serviceName);
    }

    sp<android::hardware::tetheroffload::control::V1_1::IOffloadControl> getControlV1_1() {
        // The cast is safe since only devices with V1.1+ HAL will be enumerated and pass in to the
        // test.
        return android::hardware::tetheroffload::control::V1_1::IOffloadControl::castFrom(control)
                .withDefault(nullptr);
    }
};

class OffloadControlTestV1_1_HalStarted : public OffloadControlTestV1_1_HalNotStarted {
  public:
    virtual void SetUp() override {
        setupConfigHal();
        setupControlHal();
    }
};