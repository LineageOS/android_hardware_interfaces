/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include "BiometricsFingerprint.h"

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_1 {
namespace implementation {

sp<IBiometricsFingerprintClientCallback>
    BiometricsFingerprint::mClientCallback = nullptr;

BiometricsFingerprint::BiometricsFingerprint(fingerprint_device_t *device)
    : mDevice(device) {}

BiometricsFingerprint::~BiometricsFingerprint() {
    ALOG(LOG_VERBOSE, LOG_TAG, "nativeCloseHal()\n");
    if (mDevice == NULL) {
        ALOGE("No valid device");
        return;
    }
    int err;
    if (0 != (err = mDevice->common.close(
            reinterpret_cast<hw_device_t*>(mDevice)))) {
        ALOGE("Can't close fingerprint module, error: %d", err);
        return;
    }
    mDevice = NULL;
}

Return<void> BiometricsFingerprint::setNotify(
        const sp<IBiometricsFingerprintClientCallback>& clientCallback,
        setNotify_cb cb)  {
    mClientCallback = clientCallback;
    int32_t debugErrno = mDevice->set_notify(mDevice, notify);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<uint64_t> BiometricsFingerprint::preEnroll()  {
    return mDevice->pre_enroll(mDevice);
}

Return<void> BiometricsFingerprint::enroll(const HwAuthToken& hat, uint32_t gid,
        uint32_t timeoutSec, enroll_cb cb)  {
    const hw_auth_token_t* authToken =
        reinterpret_cast<const hw_auth_token_t*>(&hat);
    int32_t debugErrno = mDevice->enroll(mDevice, authToken, gid, timeoutSec);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<void> BiometricsFingerprint::postEnroll(postEnroll_cb cb) {
    int32_t debugErrno = mDevice->post_enroll(mDevice);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<uint64_t> BiometricsFingerprint::getAuthenticatorId() {
    return mDevice->get_authenticator_id(mDevice);
}

Return<void> BiometricsFingerprint::cancel(cancel_cb cb) {
    int32_t debugErrno = mDevice->cancel(mDevice);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<void> BiometricsFingerprint::enumerate(enumerate_cb cb)  {
    int32_t debugErrno = mDevice->enumerate(mDevice);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<void> BiometricsFingerprint::remove(uint32_t gid, uint32_t fid,
        remove_cb cb)  {
    int32_t debugErrno = mDevice->remove(mDevice, gid, fid);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<void> BiometricsFingerprint::setActiveGroup(uint32_t gid,
        const hidl_string& storePath, setActiveGroup_cb cb)  {
    if (storePath.size() >= PATH_MAX || storePath.size() <= 0) {
        ALOGE("Bad path length: %zd", storePath.size());
    }
    int32_t debugErrno = mDevice->set_active_group(mDevice, gid,
        storePath.c_str());
    cb(debugErrno == 0, debugErrno);
    return Void();
}

Return<void> BiometricsFingerprint::authenticate(uint64_t operationId,
        uint32_t gid, authenticate_cb cb)  {
    int32_t debugErrno = mDevice->authenticate(mDevice, operationId, gid);
    cb(debugErrno == 0, debugErrno);
    return Void();
}

IBiometricsFingerprint* HIDL_FETCH_IBiometricsFingerprint(const char*) {
    int err;
    const hw_module_t *hw_mdl = NULL;
    if (0 != (err = hw_get_module(FINGERPRINT_HARDWARE_MODULE_ID, &hw_mdl))) {
        ALOGE("Can't open fingerprint HW Module, error: %d", err);
        return nullptr;
    }
    if (hw_mdl == NULL) {
        ALOGE("No valid fingerprint module");
        return nullptr;
    }

    fingerprint_module_t const *module =
        reinterpret_cast<const fingerprint_module_t*>(hw_mdl);
    if (module->common.methods->open == NULL) {
        ALOGE("No valid open method");
        return nullptr;
    }

    hw_device_t *device = NULL;

    if (0 != (err = module->common.methods->open(hw_mdl, NULL, &device))) {
        ALOGE("Can't open fingerprint methods, error: %d", err);
        return nullptr;
    }

    return new BiometricsFingerprint(
        reinterpret_cast<fingerprint_device_t*>(device));
}

} // namespace implementation
}  // namespace V2_1
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
