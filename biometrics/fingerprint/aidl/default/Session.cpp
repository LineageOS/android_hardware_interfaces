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
#include <android-base/logging.h>

#include "Session.h"

namespace aidl::android::hardware::biometrics::fingerprint {

class CancellationSignal : public common::BnCancellationSignal {
  public:
    ndk::ScopedAStatus cancel() override { return ndk::ScopedAStatus::ok(); }
};

Session::Session(std::shared_ptr<ISessionCallback> cb) : cb_(std::move(cb)) {}

ndk::ScopedAStatus Session::generateChallenge(int32_t /*cookie*/, int32_t /*timeoutSec*/) {
    LOG(INFO) << "generateChallenge";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int32_t /*cookie*/, int64_t /*challenge*/) {
    LOG(INFO) << "revokeChallenge";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(int32_t /*cookie*/, const keymaster::HardwareAuthToken& /*hat*/,
                                   std::shared_ptr<common::ICancellationSignal>* /*return_val*/) {
    LOG(INFO) << "enroll";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int32_t /*cookie*/, int64_t /*keystoreOperationId*/,
                                         std::shared_ptr<common::ICancellationSignal>* return_val) {
    LOG(INFO) << "authenticate";
    if (cb_) {
        cb_->onStateChanged(0, SessionState::AUTHENTICATING);
    }
    *return_val = SharedRefBase::make<CancellationSignal>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(
        int32_t /*cookie*/, std::shared_ptr<common::ICancellationSignal>* /*return_val*/) {
    LOG(INFO) << "detectInteraction";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments(int32_t /*cookie*/) {
    LOG(INFO) << "enumerateEnrollments";
    if (cb_) {
        cb_->onStateChanged(0, SessionState::ENUMERATING_ENROLLMENTS);
        cb_->onEnrollmentsEnumerated(std::vector<int32_t>());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(int32_t /*cookie*/,
                                              const std::vector<int32_t>& /*enrollmentIds*/) {
    LOG(INFO) << "removeEnrollments";
    if (cb_) {
        cb_->onStateChanged(0, SessionState::REMOVING_ENROLLMENTS);
        cb_->onEnrollmentsRemoved(std::vector<int32_t>());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId(int32_t /*cookie*/) {
    LOG(INFO) << "getAuthenticatorId";
    if (cb_) {
        cb_->onStateChanged(0, SessionState::GETTING_AUTHENTICATOR_ID);
        cb_->onAuthenticatorIdRetrieved(0 /* authenticatorId */);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId(int32_t /*cookie*/) {
    LOG(INFO) << "invalidateAuthenticatorId";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(int32_t /*cookie*/,
                                         const keymaster::HardwareAuthToken& /*hat*/) {
    LOG(INFO) << "resetLockout";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerDown(int32_t /*pointerId*/, int32_t /*x*/, int32_t /*y*/,
                                          float /*minor*/, float /*major*/) {
    LOG(INFO) << "onPointerDown";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onPointerUp(int32_t /*pointerId*/) {
    LOG(INFO) << "onPointerUp";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::onUiReady() {
    LOG(INFO) << "onUiReady";
    return ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
