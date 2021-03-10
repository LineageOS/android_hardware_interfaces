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

#pragma once

#include <aidl/android/hardware/biometrics/face/BnSession.h>
#include <aidl/android/hardware/biometrics/face/ISessionCallback.h>

namespace aidl::android::hardware::biometrics::face {

namespace common = aidl::android::hardware::biometrics::common;
namespace keymaster = aidl::android::hardware::keymaster;

using aidl::android::hardware::common::NativeHandle;

class Session : public BnSession {
  public:
    explicit Session(std::shared_ptr<ISessionCallback> cb);

    ndk::ScopedAStatus generateChallenge(int32_t cookie) override;

    ndk::ScopedAStatus revokeChallenge(int32_t cookie, int64_t challenge) override;

    ndk::ScopedAStatus enroll(int32_t cookie, const keymaster::HardwareAuthToken& hat,
                              EnrollmentType enrollmentType, const std::vector<Feature>& features,
                              const NativeHandle& previewSurface,
                              std::shared_ptr<common::ICancellationSignal>* return_val) override;

    ndk::ScopedAStatus authenticate(
            int32_t cookie, int64_t keystoreOperationId,
            std::shared_ptr<common::ICancellationSignal>* returnVal) override;

    ndk::ScopedAStatus detectInteraction(
            int32_t cookie, std::shared_ptr<common::ICancellationSignal>* returnVal) override;

    ndk::ScopedAStatus enumerateEnrollments(int32_t cookie) override;

    ndk::ScopedAStatus removeEnrollments(int32_t cookie,
                                         const std::vector<int32_t>& enrollmentIds) override;

    ndk::ScopedAStatus getFeatures(int32_t cookie, int32_t enrollmentId) override;

    ndk::ScopedAStatus setFeature(int32_t cookie, const keymaster::HardwareAuthToken& hat,
                                  int32_t enrollmentId, Feature feature, bool enabled) override;

    ndk::ScopedAStatus getAuthenticatorId(int32_t cookie) override;

    ndk::ScopedAStatus invalidateAuthenticatorId(int32_t cookie) override;

    ndk::ScopedAStatus resetLockout(int32_t cookie,
                                    const keymaster::HardwareAuthToken& hat) override;

    ndk::ScopedAStatus close(int32_t cookie) override;

  private:
    std::shared_ptr<ISessionCallback> cb_;
};

}  // namespace aidl::android::hardware::biometrics::face
