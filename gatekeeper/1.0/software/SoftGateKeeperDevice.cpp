/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "SoftGateKeeperDevice.h"
#include "SoftGateKeeper.h"

using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::gatekeeper::V1_0::GatekeeperStatusCode;
using ::gatekeeper::EnrollRequest;
using ::gatekeeper::EnrollResponse;
using ::gatekeeper::ERROR_INVALID;
using ::gatekeeper::ERROR_MEMORY_ALLOCATION_FAILED;
using ::gatekeeper::ERROR_NONE;
using ::gatekeeper::ERROR_RETRY;
using ::gatekeeper::SizedBuffer;
using ::gatekeeper::VerifyRequest;
using ::gatekeeper::VerifyResponse;

#include <limits>

namespace android {

inline SizedBuffer hidl_vec2sized_buffer(const hidl_vec<uint8_t>& vec) {
    if (vec.size() == 0 || vec.size() > std::numeric_limits<uint32_t>::max()) return {};
    auto dummy = new uint8_t[vec.size()];
    std::copy(vec.begin(), vec.end(), dummy);
    return {dummy, static_cast<uint32_t>(vec.size())};
}

Return<void> SoftGateKeeperDevice::enroll(uint32_t uid,
                                          const hidl_vec<uint8_t>& currentPasswordHandle,
                                          const hidl_vec<uint8_t>& currentPassword,
                                          const hidl_vec<uint8_t>& desiredPassword,
                                          enroll_cb _hidl_cb) {
    if (desiredPassword.size() == 0) {
        _hidl_cb({GatekeeperStatusCode::ERROR_GENERAL_FAILURE, 0, {}});
        return {};
    }

    EnrollRequest request(uid, hidl_vec2sized_buffer(currentPasswordHandle),
                          hidl_vec2sized_buffer(desiredPassword),
                          hidl_vec2sized_buffer(currentPassword));
    EnrollResponse response;
    impl_->Enroll(request, &response);

    if (response.error == ERROR_RETRY) {
        _hidl_cb({GatekeeperStatusCode::ERROR_RETRY_TIMEOUT, response.retry_timeout, {}});
    } else if (response.error != ERROR_NONE) {
        _hidl_cb({GatekeeperStatusCode::ERROR_GENERAL_FAILURE, 0, {}});
    } else {
        hidl_vec<uint8_t> new_handle(response.enrolled_password_handle.Data<uint8_t>(),
                                     response.enrolled_password_handle.Data<uint8_t>() +
                                             response.enrolled_password_handle.size());
        _hidl_cb({GatekeeperStatusCode::STATUS_OK, response.retry_timeout, new_handle});
    }
    return {};
}

Return<void> SoftGateKeeperDevice::verify(
        uint32_t uid, uint64_t challenge,
        const ::android::hardware::hidl_vec<uint8_t>& enrolledPasswordHandle,
        const ::android::hardware::hidl_vec<uint8_t>& providedPassword, verify_cb _hidl_cb) {
    if (enrolledPasswordHandle.size() == 0) {
        _hidl_cb({GatekeeperStatusCode::ERROR_GENERAL_FAILURE, 0, {}});
        return {};
    }

    VerifyRequest request(uid, challenge, hidl_vec2sized_buffer(enrolledPasswordHandle),
                          hidl_vec2sized_buffer(providedPassword));
    VerifyResponse response;

    impl_->Verify(request, &response);

    if (response.error == ERROR_RETRY) {
        _hidl_cb({GatekeeperStatusCode::ERROR_RETRY_TIMEOUT, response.retry_timeout, {}});
    } else if (response.error != ERROR_NONE) {
        _hidl_cb({GatekeeperStatusCode::ERROR_GENERAL_FAILURE, 0, {}});
    } else {
        hidl_vec<uint8_t> auth_token(
                response.auth_token.Data<uint8_t>(),
                response.auth_token.Data<uint8_t>() + response.auth_token.size());

        _hidl_cb({response.request_reenroll ? GatekeeperStatusCode::STATUS_REENROLL
                                            : GatekeeperStatusCode::STATUS_OK,
                  response.retry_timeout, auth_token});
    }
    return {};
}

Return<void> SoftGateKeeperDevice::deleteUser(uint32_t /*uid*/, deleteUser_cb _hidl_cb) {
    _hidl_cb({GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED, 0, {}});
    return {};
}

Return<void> SoftGateKeeperDevice::deleteAllUsers(deleteAllUsers_cb _hidl_cb) {
    _hidl_cb({GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED, 0, {}});
    return {};
}

}  // namespace android
