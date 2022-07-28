/*
 * Copyright (C) 2021 The Android Open Source Project
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
#include "FakeFingerprintEngine.h"

using namespace ::aidl::android::hardware::biometrics::common;

namespace aidl::android::hardware::biometrics::fingerprint {

// A fake engine that is backed by system properties instead of hardware.
class FakeFingerprintEngineUdfps : public FakeFingerprintEngine {
  public:
    static constexpr int32_t defaultSensorLocationX = 400;
    static constexpr int32_t defaultSensorLocationY = 1600;
    static constexpr int32_t defaultSensorRadius = 150;

    FakeFingerprintEngineUdfps() : FakeFingerprintEngine() {}
    ~FakeFingerprintEngineUdfps() {}

    virtual ndk::ScopedAStatus onPointerDownImpl(int32_t pointerId, int32_t x, int32_t y,
                                                 float minor, float major) override;

    virtual ndk::ScopedAStatus onPointerUpImpl(int32_t pointerId) override;

    virtual ndk::ScopedAStatus onUiReadyImpl() override;

    virtual SensorLocation defaultSensorLocation() override;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
