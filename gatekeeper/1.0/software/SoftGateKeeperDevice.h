/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef SOFT_GATEKEEPER_DEVICE_H_
#define SOFT_GATEKEEPER_DEVICE_H_

#include <android/hardware/gatekeeper/1.0/IGatekeeper.h>
#include <hidl/Status.h>

#include <memory>
#include "SoftGateKeeper.h"

namespace android {

/**
 * Software based GateKeeper implementation
 */
class SoftGateKeeperDevice : public ::android::hardware::gatekeeper::V1_0::IGatekeeper {
  public:
    SoftGateKeeperDevice() { impl_.reset(new ::gatekeeper::SoftGateKeeper()); }

    // Wrappers to translate the gatekeeper HAL API to the Kegyuard Messages API.

    /**
     * Enrolls password_payload, which should be derived from a user selected pin or password,
     * with the authentication factor private key used only for enrolling authentication
     * factor data.
     *
     * Returns: 0 on success or an error code less than 0 on error.
     * On error, enrolled_password_handle will not be allocated.
     */
    ::android::hardware::Return<void> enroll(
            uint32_t uid, const ::android::hardware::hidl_vec<uint8_t>& currentPasswordHandle,
            const ::android::hardware::hidl_vec<uint8_t>& currentPassword,
            const ::android::hardware::hidl_vec<uint8_t>& desiredPassword,
            enroll_cb _hidl_cb) override;

    /**
     * Verifies provided_password matches enrolled_password_handle.
     *
     * Implementations of this module may retain the result of this call
     * to attest to the recency of authentication.
     *
     * On success, writes the address of a verification token to auth_token,
     * usable to attest password verification to other trusted services. Clients
     * may pass NULL for this value.
     *
     * Returns: 0 on success or an error code less than 0 on error
     * On error, verification token will not be allocated
     */
    ::android::hardware::Return<void> verify(
            uint32_t uid, uint64_t challenge,
            const ::android::hardware::hidl_vec<uint8_t>& enrolledPasswordHandle,
            const ::android::hardware::hidl_vec<uint8_t>& providedPassword,
            verify_cb _hidl_cb) override;

    ::android::hardware::Return<void> deleteUser(uint32_t uid, deleteUser_cb _hidl_cb) override;

    ::android::hardware::Return<void> deleteAllUsers(deleteAllUsers_cb _hidl_cb) override;

  private:
    std::unique_ptr<::gatekeeper::SoftGateKeeper> impl_;
};

}  // namespace android

#endif  // SOFT_GATEKEEPER_DEVICE_H_
