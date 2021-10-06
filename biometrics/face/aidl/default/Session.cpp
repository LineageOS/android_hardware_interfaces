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

namespace aidl::android::hardware::biometrics::face {

class CancellationSignal : public common::BnCancellationSignal {
  private:
    std::shared_ptr<ISessionCallback> cb_;

  public:
    explicit CancellationSignal(std::shared_ptr<ISessionCallback> cb) : cb_(std::move(cb)) {}

    ndk::ScopedAStatus cancel() override {
        cb_->onError(Error::CANCELED, 0 /* vendorCode */);
        return ndk::ScopedAStatus::ok();
    }
};

Session::Session(std::shared_ptr<ISessionCallback> cb)
    : cb_(std::move(cb)), mRandom(std::mt19937::default_seed) {}

ndk::ScopedAStatus Session::generateChallenge() {
    LOG(INFO) << "generateChallenge";
    if (cb_) {
        std::uniform_int_distribution<int64_t> dist;
        auto challenge = dist(mRandom);
        cb_->onChallengeGenerated(challenge);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    LOG(INFO) << "revokeChallenge";
    if (cb_) {
        cb_->onChallengeRevoked(challenge);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getEnrollmentConfig(EnrollmentType /*enrollmentType*/,
                                                std::vector<EnrollmentStageConfig>* return_val) {
    *return_val = {};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(
        const keymaster::HardwareAuthToken& /*hat*/, EnrollmentType /*enrollmentType*/,
        const std::vector<Feature>& /*features*/,
        const std::optional<NativeHandle>& /*previewSurface*/,
        std::shared_ptr<biometrics::common::ICancellationSignal>* /*return_val*/) {
    LOG(INFO) << "enroll";
    if (cb_) {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int64_t /*keystoreOperationId*/,
                                         std::shared_ptr<common::ICancellationSignal>* return_val) {
    LOG(INFO) << "authenticate";
    if (cb_) {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    }
    *return_val = SharedRefBase::make<CancellationSignal>(cb_);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(
        std::shared_ptr<common::ICancellationSignal>* /*return_val*/) {
    LOG(INFO) << "detectInteraction";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {
    LOG(INFO) << "enumerateEnrollments";
    if (cb_) {
        cb_->onEnrollmentsEnumerated(std::vector<int32_t>());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& /*enrollmentIds*/) {
    LOG(INFO) << "removeEnrollments";
    if (cb_) {
        cb_->onEnrollmentsRemoved(std::vector<int32_t>());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getFeatures() {
    LOG(INFO) << "getFeatures";
    if (cb_) {
        // Must error out with UNABLE_TO_PROCESS when no faces are enrolled.
        cb_->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::setFeature(const keymaster::HardwareAuthToken& /*hat*/,
                                       Feature /*feature*/, bool /*enabled*/) {
    LOG(INFO) << "setFeature";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    LOG(INFO) << "getAuthenticatorId";
    if (cb_) {
        cb_->onAuthenticatorIdRetrieved(0 /* authenticatorId */);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    LOG(INFO) << "invalidateAuthenticatorId";
    if (cb_) {
        cb_->onAuthenticatorIdInvalidated(0);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const keymaster::HardwareAuthToken& /*hat*/) {
    LOG(INFO) << "resetLockout";
    if (cb_) {
        cb_->onLockoutCleared();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close() {
    if (cb_) {
        cb_->onSessionClosed();
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::face
