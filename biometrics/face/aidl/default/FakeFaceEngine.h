/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <aidl/android/hardware/biometrics/face/BnSession.h>
#include <aidl/android/hardware/biometrics/face/FaceSensorType.h>
#include <aidl/android/hardware/biometrics/face/ISessionCallback.h>

#include <future>
#include <random>
#include <vector>

#include "FakeLockoutTracker.h"

namespace aidl::android::hardware::biometrics::face {

namespace face = aidl::android::hardware::biometrics::face;
namespace common = aidl::android::hardware::biometrics::common;
namespace keymaster = aidl::android::hardware::keymaster;

using aidl::android::hardware::common::NativeHandle;
// A fake engine that is backed by system properties instead of hardware.
class FakeFaceEngine {
  public:
    FakeFaceEngine() : mRandom(std::mt19937::default_seed) {}
    virtual ~FakeFaceEngine() {}

    static face::FaceSensorType GetSensorType();
    static common::SensorStrength GetSensorStrength();
    void generateChallengeImpl(ISessionCallback* cb);
    void revokeChallengeImpl(ISessionCallback* cb, int64_t challenge);
    void getEnrollmentConfigImpl(ISessionCallback* cb,
                                 std::vector<EnrollmentStageConfig>* return_val);
    void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                    EnrollmentType enrollmentType, const std::vector<Feature>& features,
                    const std::future<void>& cancel);
    void authenticateImpl(ISessionCallback* cb, int64_t operationId,
                          const std::future<void>& cancel);
    void detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel);
    void enumerateEnrollmentsImpl(ISessionCallback* cb);
    void removeEnrollmentsImpl(ISessionCallback* cb, const std::vector<int32_t>& enrollmentIds);
    void getFeaturesImpl(ISessionCallback* cb);
    void setFeatureImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                        Feature feature, bool enabled);
    void getAuthenticatorIdImpl(ISessionCallback* cb);
    void invalidateAuthenticatorIdImpl(ISessionCallback* cb);
    void resetLockoutImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& /*hat*/);

    virtual std::string toString() const {
        std::ostringstream os;
        os << "----- FakeFaceEngine:: -----" << std::endl;
        os << mLockoutTracker.toString();
        return os.str();
    }

    std::mt19937 mRandom;

  private:
    static constexpr int32_t FACE_ACQUIRED_VENDOR_BASE = 1000;
    static constexpr int32_t FACE_ERROR_VENDOR_BASE = 1000;
    std::pair<AcquiredInfo, int32_t> convertAcquiredInfo(int32_t code);
    std::pair<Error, int32_t> convertError(int32_t code);
    FakeLockoutTracker mLockoutTracker;
};

}  // namespace aidl::android::hardware::biometrics::face
