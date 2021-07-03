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

#include <android-base/logging.h>
#include <random>

namespace aidl::android::hardware::biometrics::fingerprint {

class FakeFingerprintEngine {
  public:
    FakeFingerprintEngine() : mRandom(std::mt19937::default_seed) {}

    void generateChallengeImpl(ISessionCallback* cb) {
        LOG(INFO) << "generateChallengeImpl";
        std::uniform_int_distribution<int64_t> dist;
        auto challenge = dist(mRandom);
        cb->onChallengeGenerated(challenge);
    }

    void revokeChallengeImpl(ISessionCallback* cb, int64_t challenge) {
        LOG(INFO) << "revokeChallengeImpl";
        cb->onChallengeRevoked(challenge);
    }

    void enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat) {
        LOG(INFO) << "enrollImpl";
        // Do proper HAT verification in the real implementation.
        if (hat.mac.empty()) {
            cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
            return;
        }
        cb->onEnrollmentProgress(0 /* enrollmentId */, 0 /* remaining */);
    }

    void authenticateImpl(ISessionCallback* cb, int64_t /* operationId */) {
        LOG(INFO) << "authenticateImpl";
        cb->onAuthenticationSucceeded(0 /* enrollmentId */, {} /* hat */);
    }

    void detectInteractionImpl(ISessionCallback* cb) {
        LOG(INFO) << "detectInteractionImpl";
        cb->onInteractionDetected();
    }

    void enumerateEnrollmentsImpl(ISessionCallback* cb) {
        LOG(INFO) << "enumerateEnrollmentsImpl";
        cb->onEnrollmentsEnumerated({} /* enrollmentIds */);
    }

    void removeEnrollmentsImpl(ISessionCallback* cb, const std::vector<int32_t>& enrollmentIds) {
        LOG(INFO) << "removeEnrollmentsImpl";
        cb->onEnrollmentsRemoved(enrollmentIds);
    }

    void getAuthenticatorIdImpl(ISessionCallback* cb) {
        LOG(INFO) << "getAuthenticatorIdImpl";
        cb->onAuthenticatorIdRetrieved(0 /* authenticatorId */);
    }

    void invalidateAuthenticatorIdImpl(ISessionCallback* cb) {
        LOG(INFO) << "invalidateAuthenticatorIdImpl";
        cb->onAuthenticatorIdInvalidated(0 /* newAuthenticatorId */);
    }

    void resetLockoutImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& /*hat*/) {
        LOG(INFO) << "resetLockoutImpl";
        cb->onLockoutCleared();
    }

    std::mt19937 mRandom;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
