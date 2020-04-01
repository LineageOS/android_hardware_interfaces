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

#pragma once

#include <android/hardware/biometrics/face/1.1/IBiometricsFace.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <random>

namespace android::hardware::biometrics::face::implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::biometrics::face::V1_0::Feature;
using ::android::hardware::biometrics::face::V1_0::IBiometricsFaceClientCallback;
using ::android::hardware::biometrics::face::V1_0::Status;

class BiometricsFace : public V1_1::IBiometricsFace {
  public:
    BiometricsFace();

    // Methods from ::android::hardware::biometrics::face::V1_0::IBiometricsFace follow.
    Return<void> setCallback(const sp<IBiometricsFaceClientCallback>& clientCallback,
                             setCallback_cb _hidl_cb) override;

    Return<Status> setActiveUser(int32_t userId, const hidl_string& storePath) override;

    Return<void> generateChallenge(uint32_t challengeTimeoutSec,
                                   generateChallenge_cb _hidl_cb) override;

    Return<Status> enroll(const hidl_vec<uint8_t>& hat, uint32_t timeoutSec,
                          const hidl_vec<Feature>& disabledFeatures) override;

    Return<Status> revokeChallenge() override;

    Return<Status> setFeature(Feature feature, bool enabled, const hidl_vec<uint8_t>& hat,
                              uint32_t faceId) override;

    Return<void> getFeature(Feature feature, uint32_t faceId, getFeature_cb _hidl_cb) override;

    Return<void> getAuthenticatorId(getAuthenticatorId_cb _hidl_cb) override;

    Return<Status> cancel() override;

    Return<Status> enumerate() override;

    Return<Status> remove(uint32_t faceId) override;

    Return<Status> authenticate(uint64_t operationId) override;

    Return<Status> userActivity() override;

    Return<Status> resetLockout(const hidl_vec<uint8_t>& hat) override;

    // Methods from ::android::hardware::biometrics::face::V1_1::IBiometricsFace follow.
    Return<Status> enroll_1_1(const hidl_vec<uint8_t>& hat, uint32_t timeoutSec,
                              const hidl_vec<Feature>& disabledFeatures,
                              const hidl_handle& windowId) override;

    Return<Status> enrollRemotely(const hidl_vec<uint8_t>& hat, uint32_t timeoutSec,
                                  const hidl_vec<Feature>& disabledFeatures) override;

  private:
    std::mt19937 mRandom;
    int32_t mUserId;
    sp<IBiometricsFaceClientCallback> mClientCallback;
};

}  // namespace android::hardware::biometrics::face::implementation
