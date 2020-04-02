/*
 * Copyright 2020 The Android Open Source Project
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

#include "BiometricsFace.h"

namespace android::hardware::biometrics::face::implementation {
using android::hardware::biometrics::face::V1_0::FaceError;
using android::hardware::biometrics::face::V1_0::OptionalUint64;

// Arbitrary value.
constexpr uint64_t kDeviceId = 123;
// Arbitrary value.
constexpr uint64_t kAuthenticatorId = 987;
// Arbitrary value.
constexpr uint64_t kLockoutDuration = 555;

BiometricsFace::BiometricsFace() : mRandom(std::mt19937::default_seed) {}

// Methods from IBiometricsFace follow.
Return<void> BiometricsFace::setCallback(const sp<IBiometricsFaceClientCallback>& clientCallback,
                                         setCallback_cb _hidl_cb) {
    mClientCallback = clientCallback;
    _hidl_cb({Status::OK, kDeviceId});
    return Void();
}

Return<Status> BiometricsFace::setActiveUser(int32_t userId, const hidl_string& storePath) {
    if (userId < 0 || storePath.empty() || std::string(storePath).find("/data") != 0) {
        return Status::ILLEGAL_ARGUMENT;
    }
    mUserId = userId;
    mClientCallback->onLockoutChanged(kLockoutDuration);
    return Status::OK;
}

Return<void> BiometricsFace::generateChallenge(uint32_t /* challengeTimeoutSec */,
                                               generateChallenge_cb _hidl_cb) {
    std::uniform_int_distribution<uint64_t> dist;
    _hidl_cb({Status::OK, dist(mRandom)});
    return Void();
}

Return<Status> BiometricsFace::enroll(const hidl_vec<uint8_t>& /* hat */, uint32_t /* timeoutSec */,
                                      const hidl_vec<Feature>& /* disabledFeatures */) {
    // hat can never be valid in this implementation.
    mClientCallback->onError(kDeviceId, mUserId, FaceError::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    return Status::OK;
}

Return<Status> BiometricsFace::revokeChallenge() {
    return Status::OK;
}

Return<Status> BiometricsFace::setFeature(Feature /* feature */, bool /* enabled */,
                                          const hidl_vec<uint8_t>& /* hat */,
                                          uint32_t /* faceId */) {
    // hat can never be valid in this implementation.
    return Status::ILLEGAL_ARGUMENT;
}

Return<void> BiometricsFace::getFeature(Feature /* feature */, uint32_t /* faceId */,
                                        getFeature_cb _hidl_cb) {
    // hat can never be valid in this implementation.
    _hidl_cb({Status::ILLEGAL_ARGUMENT, false});
    return Void();
}

Return<void> BiometricsFace::getAuthenticatorId(getAuthenticatorId_cb _hidl_cb) {
    _hidl_cb({Status::OK, kAuthenticatorId});
    return Void();
}

Return<Status> BiometricsFace::cancel() {
    mClientCallback->onError(kDeviceId, mUserId, FaceError::CANCELED, 0 /* vendorCode */);
    return Status::OK;
}

Return<Status> BiometricsFace::enumerate() {
    mClientCallback->onEnumerate(kDeviceId, {}, mUserId);
    return Status::OK;
}

Return<Status> BiometricsFace::remove(uint32_t /* faceId */) {
    return Status::OK;
}

Return<Status> BiometricsFace::authenticate(uint64_t /* operationId */) {
    mClientCallback->onError(kDeviceId, mUserId, FaceError::HW_UNAVAILABLE, 0 /* vendorCode */);
    return Status::OK;
}

Return<Status> BiometricsFace::userActivity() {
    return Status::OK;
}

Return<Status> BiometricsFace::resetLockout(const hidl_vec<uint8_t>& /* hat */) {
    return Status::OK;
}

// Methods from ::android::hardware::biometrics::face::V1_1::IBiometricsFace follow.
Return<Status> BiometricsFace::enroll_1_1(const hidl_vec<uint8_t>& /* hat */,
                                          uint32_t /* timeoutSec */,
                                          const hidl_vec<Feature>& /* disabledFeatures */,
                                          const hidl_handle& /* windowId */) {
    mClientCallback->onError(kDeviceId, mUserId, FaceError::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    return Status::OK;
}

Return<Status> BiometricsFace::enrollRemotely(const hidl_vec<uint8_t>& /* hat */,
                                              uint32_t /* timeoutSec */,
                                              const hidl_vec<Feature>& /* disabledFeatures */) {
    mClientCallback->onError(kDeviceId, mUserId, FaceError::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    return Status::OK;
}

}  // namespace android::hardware::biometrics::face::implementation
