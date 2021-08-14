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
#define LOG_TAG "android.hardware.biometrics.fingerprint@2.2-service"
#define LOG_VERBOSE "android.hardware.biometrics.fingerprint@2.2-service"

#include <hardware/hw_auth_token.h>

#include <android/log.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include "BiometricsFingerprint.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_2 {
namespace implementation {

using RequestStatus = android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using FingerprintError = android::hardware::biometrics::fingerprint::V2_1::FingerprintError;

constexpr uint64_t kDeviceId = 1;

BiometricsFingerprint::BiometricsFingerprint() {

}

BiometricsFingerprint::~BiometricsFingerprint() {

}

Return<uint64_t> BiometricsFingerprint::setNotify(
        const sp<IBiometricsFingerprintClientCallback>& clientCallback) {
    mClientCallback = clientCallback;
    return kDeviceId;
}

Return<uint64_t> BiometricsFingerprint::preEnroll()  {
    // On a real implementation, this must be generated and stored in the TEE or its equivalent.
    return rand();
}

Return<RequestStatus> BiometricsFingerprint::enroll(const hidl_array<uint8_t, 69>&  /* hat */,
        uint32_t /* gid */, uint32_t /* timeoutSec */) {
    // On a real implementation, the HAT must be checked in the TEE or its equivalent.
    mClientCallback->onError(kDeviceId, FingerprintError::ERROR_UNABLE_TO_PROCESS,
            0 /* vendorCode */);
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::postEnroll() {
    return RequestStatus::SYS_OK;
}

Return<uint64_t> BiometricsFingerprint::getAuthenticatorId() {
    return 1;
}

Return<RequestStatus> BiometricsFingerprint::cancel() {
    mClientCallback->onError(kDeviceId, FingerprintError::ERROR_CANCELED, 0 /* vendorCode */);
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::enumerate()  {
    mClientCallback->onEnumerate(kDeviceId, 0 /* fingerId */, 0 /* groupId */,
            0 /* remaining */);
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::remove(uint32_t gid, uint32_t fid) {
    mClientCallback->onRemoved(kDeviceId, fid, gid, 0 /* remaining */);
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::setActiveGroup(uint32_t /* gid */,
        const hidl_string& storePath) {
    // Return invalid for paths that the HAL is unable to write to.
    std::string path = storePath.c_str();
    if (path.compare("") == 0 || path.compare("/") == 0) {
        return RequestStatus::SYS_EINVAL;
    }
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::authenticate(uint64_t /* operationId */,
        uint32_t /* gid */) {
    return RequestStatus::SYS_OK;
}

} // namespace implementation
}  // namespace V2_2
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
