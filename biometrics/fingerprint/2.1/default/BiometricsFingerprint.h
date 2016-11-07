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

#ifndef HIDL_GENERATED_android_hardware_biometrics_fingerprint_V2_1_BiometricsFingerprint_H_
#define HIDL_GENERATED_android_hardware_biometrics_fingerprint_V2_1_BiometricsFingerprint_H_

#include <utils/Log.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_1 {
namespace implementation {

using ::android::hardware::biometrics::fingerprint::V2_1::HwAuthToken;
using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct BiometricsFingerprint : public IBiometricsFingerprint {
public:
    BiometricsFingerprint(fingerprint_device_t *device);
    ~BiometricsFingerprint();
    // Methods from ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint follow.
    Return<void> setNotify(const sp<IBiometricsFingerprintClientCallback>& clientCallback, setNotify_cb _hidl_cb)  override;
    Return<uint64_t> preEnroll()  override;
    Return<void> enroll(const HwAuthToken& hat, uint32_t gid, uint32_t timeoutSec, enroll_cb _hidl_cb)  override;
    Return<void> postEnroll(postEnroll_cb _hidl_cb)  override;
    Return<uint64_t> getAuthenticatorId()  override;
    Return<void> cancel(cancel_cb _hidl_cb)  override;
    Return<void> enumerate(enumerate_cb _hidl_cb)  override;
    Return<void> remove(uint32_t gid, uint32_t fid, remove_cb _hidl_cb)  override;
    Return<void> setActiveGroup(uint32_t gid, const hidl_string& storePath, setActiveGroup_cb _hidl_cb)  override;
    Return<void> authenticate(uint64_t operationId, uint32_t gid, authenticate_cb _hidl_cb)  override;
    static void notify(const fingerprint_msg_t *notify_msg) {
        if (mClientCallback == nullptr) {
            ALOGE("Receiving callbacks before the client callback is registered.");
            return;
        }
        FingerprintMsg msg = {};
        memcpy(&msg, notify_msg, sizeof(msg));
        mClientCallback->notify(msg);
    }
private:
    static sp<IBiometricsFingerprintClientCallback> mClientCallback;
    fingerprint_device_t *mDevice;
};

extern "C" IBiometricsFingerprint* HIDL_FETCH_IBiometricsFingerprint(const char* name);

}  // namespace implementation
}  // namespace V2_1
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_biometrics_fingerprint_V2_1_BiometricsFingerprint_H_
