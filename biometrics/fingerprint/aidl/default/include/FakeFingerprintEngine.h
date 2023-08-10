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

#define LOG_TAG "FingerprintVirtualHal"

#include <aidl/android/hardware/biometrics/common/SensorStrength.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <android/binder_to_string.h>
#include <string>

#include <random>

#include <aidl/android/hardware/biometrics/fingerprint/SensorLocation.h>
#include <future>
#include <vector>

#include "FakeLockoutTracker.h"

using namespace ::aidl::android::hardware::biometrics::common;

namespace aidl::android::hardware::biometrics::fingerprint {

// A fake engine that is backed by system properties instead of hardware.
class FakeFingerprintEngine {
  public:
    FakeFingerprintEngine();
    virtual ~FakeFingerprintEngine() {}

    void generateChallengeImpl(ISessionCallback* cb);
    void revokeChallengeImpl(ISessionCallback* cb, int64_t challenge);
    virtual void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                            const std::future<void>& cancel);
    virtual void authenticateImpl(ISessionCallback* cb, int64_t operationId,
                                  const std::future<void>& cancel);
    virtual void detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel);
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

    virtual void fingerDownAction();

    std::vector<int32_t> parseIntSequence(const std::string& str, const std::string& sep = ",");

    std::vector<std::vector<int32_t>> parseEnrollmentCapture(const std::string& str);

    int32_t getLatency(const std::vector<std::optional<std::int32_t>>& latencyVec);

    std::mt19937 mRandom;

    enum class WorkMode : int8_t { kIdle = 0, kAuthenticate, kEnroll, kDetectInteract };

    WorkMode getWorkMode() { return mWorkMode; }

    virtual std::string toString() const {
        std::ostringstream os;
        os << "----- FakeFingerprintEngine:: -----" << std::endl;
        os << "mWorkMode:" << (int)mWorkMode;
        os << "acquiredVendorInfoBase:" << FINGERPRINT_ACQUIRED_VENDOR_BASE;
        os << ", errorVendorBase:" << FINGERPRINT_ERROR_VENDOR_BASE << std::endl;
        os << mLockoutTracker.toString();
        return os.str();
    }

  protected:
    virtual void updateContext(WorkMode mode, ISessionCallback* cb, std::future<void>& cancel,
                               int64_t operationId, const keymaster::HardwareAuthToken& hat);

    bool onEnrollFingerDown(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                            const std::future<void>& cancel);
    bool onAuthenticateFingerDown(ISessionCallback* cb, int64_t, const std::future<void>& cancel);
    bool onDetectInteractFingerDown(ISessionCallback* cb, const std::future<void>& cancel);

    WorkMode mWorkMode;
    ISessionCallback* mCb;
    keymaster::HardwareAuthToken mHat;
    std::future<void> mCancel;
    int64_t mOperationId;

  private:
    static constexpr int32_t FINGERPRINT_ACQUIRED_VENDOR_BASE = 1000;
    static constexpr int32_t FINGERPRINT_ERROR_VENDOR_BASE = 1000;
    std::pair<AcquiredInfo, int32_t> convertAcquiredInfo(int32_t code);
    std::pair<Error, int32_t> convertError(int32_t code);
    bool parseEnrollmentCaptureSingle(const std::string& str,
                                      std::vector<std::vector<int32_t>>& res);
    int32_t getRandomInRange(int32_t bound1, int32_t bound2);
    bool checkSensorLockout(ISessionCallback*);
    void clearLockout(ISessionCallback* cb);

    FakeLockoutTracker mLockoutTracker;

  protected:
    // lockout timer
    void lockoutTimerExpired(ISessionCallback* cb);
    bool isLockoutTimerSupported;
    bool isLockoutTimerStarted;
    bool isLockoutTimerAborted;

  public:
    void startLockoutTimer(int64_t timeout, ISessionCallback* cb);
    bool getLockoutTimerStarted() { return isLockoutTimerStarted; }
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
