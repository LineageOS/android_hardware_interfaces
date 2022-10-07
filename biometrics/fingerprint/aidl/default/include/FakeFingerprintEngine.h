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
#include <aidl/android/hardware/biometrics/common/SensorStrength.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>

#include <random>

#include <aidl/android/hardware/biometrics/fingerprint/SensorLocation.h>
#include <future>
#include <vector>

using namespace ::aidl::android::hardware::biometrics::common;

namespace aidl::android::hardware::biometrics::fingerprint {

// A fake engine that is backed by system properties instead of hardware.
class FakeFingerprintEngine {
  public:
    FakeFingerprintEngine() : mRandom(std::mt19937::default_seed) {}
    virtual ~FakeFingerprintEngine() {}

    void generateChallengeImpl(ISessionCallback* cb);
    void revokeChallengeImpl(ISessionCallback* cb, int64_t challenge);
    void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                    const std::future<void>& cancel);
    void authenticateImpl(ISessionCallback* cb, int64_t operationId,
                          const std::future<void>& cancel);
    void detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel);
    void enumerateEnrollmentsImpl(ISessionCallback* cb);
    void removeEnrollmentsImpl(ISessionCallback* cb, const std::vector<int32_t>& enrollmentIds);
    void getAuthenticatorIdImpl(ISessionCallback* cb);
    void invalidateAuthenticatorIdImpl(ISessionCallback* cb);
    void resetLockoutImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& /*hat*/);
    bool getSensorLocationConfig(SensorLocation& out);

    virtual ndk::ScopedAStatus onPointerDownImpl(int32_t pointerId, int32_t x, int32_t y,
                                                 float minor, float major);

    virtual ndk::ScopedAStatus onPointerUpImpl(int32_t pointerId);

    virtual ndk::ScopedAStatus onUiReadyImpl();

    virtual SensorLocation getSensorLocation();

    virtual SensorLocation defaultSensorLocation();

    std::vector<int32_t> parseIntSequence(const std::string& str, const std::string& sep = ",");

    std::vector<std::vector<int32_t>> parseEnrollmentCapture(const std::string& str);

    std::mt19937 mRandom;

  private:
    static constexpr int32_t FINGERPRINT_ACQUIRED_VENDOR_BASE = 1000;
    static constexpr int32_t FINGERPRINT_ERROR_VENDOR_BASE = 1000;
    std::pair<AcquiredInfo, int32_t> convertAcquiredInfo(int32_t code);
    std::pair<Error, int32_t> convertError(int32_t code);
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
