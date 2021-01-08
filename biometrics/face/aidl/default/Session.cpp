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

#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>

#include "Session.h"

namespace aidl::android::hardware::biometrics::face {

class CancellationSignal : public common::BnCancellationSignal {
  private:
    std::shared_ptr<ISessionCallback> cb_;

  public:
    explicit CancellationSignal(std::shared_ptr<ISessionCallback> cb) : cb_(std::move(cb)) {}

    ndk::ScopedAStatus cancel() override {
        cb_->onError(Error::CANCELED, 0 /* vendorCode */);
        cb_->onStateChanged(0, SessionState::IDLING);
        return ndk::ScopedAStatus::ok();
    }
};

Session::Session(std::shared_ptr<ISessionCallback> cb) : cb_(std::move(cb)) {}

ndk::ScopedAStatus Session::generateChallenge(int32_t /*cookie*/, int32_t /*timeoutSec*/) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::GENERATING_CHALLENGE);
        cb_->onChallengeGenerated(0);
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int32_t /*cookie*/, int64_t challenge) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::REVOKING_CHALLENGE);
        cb_->onChallengeRevoked(challenge);
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(
        int32_t /*cookie*/, biometrics::face::EnrollmentType /*enrollmentType*/,
        const keymaster::HardwareAuthToken& /*hat*/, const NativeHandle& /*previewSurface*/,
        std::shared_ptr<biometrics::common::ICancellationSignal>* /*return_val*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int32_t /*cookie*/, int64_t /*keystoreOperationId*/,
                                         std::shared_ptr<common::ICancellationSignal>* return_val) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::AUTHENTICATING);
    }
    *return_val = SharedRefBase::make<CancellationSignal>(cb_);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(
        int32_t /*cookie*/, std::shared_ptr<common::ICancellationSignal>* /*return_val*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments(int32_t /*cookie*/) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::ENUMERATING_ENROLLMENTS);
        cb_->onEnrollmentsEnumerated(std::vector<int32_t>());
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(int32_t /*cookie*/,
                                              const std::vector<int32_t>& /*enrollmentIds*/) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::REMOVING_ENROLLMENTS);
        cb_->onEnrollmentsRemoved(std::vector<int32_t>());
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId(int32_t /*cookie*/) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::GETTING_AUTHENTICATOR_ID);
        cb_->onAuthenticatorIdRetrieved(0 /* authenticatorId */);
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId(int32_t /*cookie*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(int32_t /*cookie*/,
                                         const keymaster::HardwareAuthToken& /*hat*/) {
    if (cb_) {
        cb_->onStateChanged(0, SessionState::RESETTING_LOCKOUT);
        cb_->onLockoutCleared();
        cb_->onStateChanged(0, SessionState::IDLING);
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::face
