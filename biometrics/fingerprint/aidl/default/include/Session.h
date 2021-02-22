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

#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>

#include "FakeFingerprintEngine.h"
#include "WorkerThread.h"

namespace aidl::android::hardware::biometrics::fingerprint {

namespace common = aidl::android::hardware::biometrics::common;
namespace keymaster = aidl::android::hardware::keymaster;

class Session : public BnSession {
  public:
    Session(int sensorId, int userId, std::shared_ptr<ISessionCallback> cb,
            FakeFingerprintEngine* engine, WorkerThread* worker);

    ndk::ScopedAStatus generateChallenge(int32_t cookie, int32_t timeoutSec) override;

    ndk::ScopedAStatus revokeChallenge(int32_t cookie, int64_t challenge) override;

    ndk::ScopedAStatus enroll(int32_t cookie, const keymaster::HardwareAuthToken& hat,
                              std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus authenticate(int32_t cookie, int64_t operationId,
                                    std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus detectInteraction(
            int32_t cookie, std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus enumerateEnrollments(int32_t cookie) override;

    ndk::ScopedAStatus removeEnrollments(int32_t cookie,
                                         const std::vector<int32_t>& enrollmentIds) override;

    ndk::ScopedAStatus getAuthenticatorId(int32_t cookie) override;

    ndk::ScopedAStatus invalidateAuthenticatorId(int32_t cookie) override;

    ndk::ScopedAStatus resetLockout(int32_t cookie,
                                    const keymaster::HardwareAuthToken& hat) override;

    ndk::ScopedAStatus close(int32_t cookie) override;

    ndk::ScopedAStatus onPointerDown(int32_t pointerId, int32_t x, int32_t y, float minor,
                                     float major) override;

    ndk::ScopedAStatus onPointerUp(int32_t pointerId) override;

    ndk::ScopedAStatus onUiReady() override;

    bool isClosed();

  private:
    // Crashes the HAL if it's not currently idling because that would be an invalid state machine
    // transition. Otherwise, sets the scheduled state to the given state.
    void scheduleStateOrCrash(SessionState state);

    // Crashes the HAL if the provided state doesn't match the previously scheduled state.
    // Otherwise, transitions into the provided state, clears the scheduled state, and notifies
    // the client about the transition by calling ISessionCallback#onStateChanged.
    void enterStateOrCrash(int cookie, SessionState state);

    // Sets the current state to SessionState::IDLING and notifies the client about the transition
    // by calling ISessionCallback#onStateChanged.
    void enterIdling(int cookie);

    int32_t mSensorId;
    int32_t mUserId;
    std::shared_ptr<ISessionCallback> mCb;
    FakeFingerprintEngine* mEngine;
    WorkerThread* mWorker;
    SessionState mScheduledState;
    SessionState mCurrentState;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
