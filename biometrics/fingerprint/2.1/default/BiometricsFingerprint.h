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

#ifndef ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_H
#define ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_H

#include <log/log.h>
#include <android/log.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_1 {
namespace implementation {

using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using ::android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct BiometricsFingerprint : public IBiometricsFingerprint {
public:
    BiometricsFingerprint(fingerprint_device_t *device);
    ~BiometricsFingerprint();

    // Method to wrap legacy HAL with BiometricsFingerprint class
    static IBiometricsFingerprint* getInstance();

    // Methods from ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint follow.
    Return<uint64_t> setNotify(const sp<IBiometricsFingerprintClientCallback>& clientCallback) override;
    Return<uint64_t> preEnroll() override;
    Return<RequestStatus> enroll(const hidl_array<uint8_t, 69>& hat, uint32_t gid, uint32_t timeoutSec) override;
    Return<RequestStatus> postEnroll() override;
    Return<uint64_t> getAuthenticatorId() override;
    Return<RequestStatus> cancel() override;
    Return<RequestStatus> enumerate() override;
    Return<RequestStatus> remove(uint32_t gid, uint32_t fid) override;
    Return<RequestStatus> setActiveGroup(uint32_t gid, const hidl_string& storePath) override;
    Return<RequestStatus> authenticate(uint64_t operationId, uint32_t gid) override;
    static void notify(const fingerprint_msg_t *msg) {
        if (mClientCallback == nullptr) {
            ALOGE("Receiving callbacks before the client callback is registered.");
            return;
        }
        const uint64_t devId = reinterpret_cast<uint64_t>(sDevice);
        switch (msg->type) {
            case FINGERPRINT_ERROR: {
                int32_t vendorCode = 0;
                FingerprintError result =
                    VendorErrorFilter(msg->data.error, &vendorCode);
                mClientCallback->onError(devId, result, vendorCode);
                }
                break;
            case FINGERPRINT_ACQUIRED: {
                int32_t vendorCode = 0;
                FingerprintAcquiredInfo result =
                    VendorAcquiredFilter(msg->data.acquired.acquired_info,
                                         &vendorCode);
                mClientCallback->onAcquired(devId, result, vendorCode);
                }
                break;
            case FINGERPRINT_TEMPLATE_ENROLLING:
                mClientCallback->onEnrollResult(devId,
                    msg->data.enroll.finger.fid,
                    msg->data.enroll.finger.gid,
                    msg->data.enroll.samples_remaining);
                break;
            case FINGERPRINT_TEMPLATE_REMOVED:
                mClientCallback->onRemoved(devId,
                    msg->data.removed.finger.fid,
                    msg->data.removed.finger.gid,
                    msg->data.removed.remaining_templates);
                break;
            case FINGERPRINT_AUTHENTICATED:
                mClientCallback->onAuthenticated(devId,
                    msg->data.authenticated.finger.fid,
                    msg->data.authenticated.finger.gid);
                break;
            case FINGERPRINT_TEMPLATE_ENUMERATING:
                mClientCallback->onEnumerate(devId,
                    msg->data.enumerated.finger.fid,
                    msg->data.enumerated.finger.gid,
                    msg->data.enumerated.remaining_templates);
                break;
        }
    }
private:
    Return<RequestStatus> ErrorFilter(int32_t error);
    static FingerprintError VendorErrorFilter(int32_t error,
            int32_t* vendorCode);
    static FingerprintAcquiredInfo VendorAcquiredFilter(int32_t error,
            int32_t* vendorCode);
    static sp<IBiometricsFingerprintClientCallback> mClientCallback;
    fingerprint_device_t *mDevice;
    static fingerprint_device_t *sDevice; // TODO: allow multiple drivers
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_H
