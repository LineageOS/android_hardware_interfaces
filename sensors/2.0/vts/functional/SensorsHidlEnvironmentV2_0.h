/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H
#define ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H

#include "sensors-vts-utils/SensorsHidlEnvironmentBase.h"

#include <android/hardware/sensors/1.0/types.h>
#include <android/hardware/sensors/2.0/ISensors.h>
#include <utils/StrongPointer.h>

#include <atomic>
#include <memory>

using ::android::sp;

class SensorsHidlTest;
class SensorsHidlEnvironmentV2_0 : public SensorsHidlEnvironmentBase {
   public:
    using Event = ::android::hardware::sensors::V1_0::Event;
    // get the test environment singleton
    static SensorsHidlEnvironmentV2_0* Instance() {
        static SensorsHidlEnvironmentV2_0* instance = new SensorsHidlEnvironmentV2_0();
        return instance;
    }

    virtual void registerTestServices() override {
        registerTestService<android::hardware::sensors::V2_0::ISensors>();
    }

   private:
    friend SensorsHidlTest;
    // sensors hidl service
    sp<android::hardware::sensors::V2_0::ISensors> sensors;

    SensorsHidlEnvironmentV2_0() {}

    bool resetHal() override;
    void startPollingThread() override;
    static void pollingThread(SensorsHidlEnvironmentV2_0* env, std::atomic_bool& stop);

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SensorsHidlEnvironmentV2_0);
};

#endif  // ANDROID_SENSORS_HIDL_ENVIRONMENT_V2_0_H
