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

    static constexpr int32_t uiReadyTimeoutInMs = 5000;

    FakeFingerprintEngineUdfps();
    ~FakeFingerprintEngineUdfps() {}

    ndk::ScopedAStatus onPointerDownImpl(int32_t pointerId, int32_t x, int32_t y, float minor,
                                         float major) override;

    ndk::ScopedAStatus onPointerUpImpl(int32_t pointerId) override;

    ndk::ScopedAStatus onUiReadyImpl() override;

    SensorLocation defaultSensorLocation() override;

    void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                    const std::future<void>& cancel);
    void authenticateImpl(ISessionCallback* cb, int64_t operationId,
                          const std::future<void>& cancel);
    void detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel);

    enum class WorkMode : int8_t { kIdle = 0, kAuthenticate, kEnroll, kDetectInteract };

    WorkMode getWorkMode() { return mWorkMode; }

    std::string toString() const {
        std::ostringstream os;
        os << FakeFingerprintEngine::toString();
        os << "----- FakeFingerprintEngineUdfps -----" << std::endl;
        os << "mWorkMode:" << (int)mWorkMode;
        os << ", mUiReadyTime:" << mUiReadyTime;
        os << ", mPointerDownTime:" << mPointerDownTime << std::endl;
        return os.str();
    }

  private:
    void onAuthenticateFingerDown();
    void onEnrollFingerDown();
    void onDetectInteractFingerDown();
    void fingerDownAction();
    void updateContext(WorkMode mode, ISessionCallback* cb, std::future<void>& cancel,
                       int64_t operationId, const keymaster::HardwareAuthToken& hat);

    WorkMode mWorkMode;
    ISessionCallback* mCb;
    keymaster::HardwareAuthToken mHat;
    std::vector<std::future<void>> mCancelVec;
    int64_t mOperationId;
    int64_t mPointerDownTime;
    int64_t mUiReadyTime;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
