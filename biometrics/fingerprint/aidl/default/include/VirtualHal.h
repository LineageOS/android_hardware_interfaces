/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/hardware/biometrics/fingerprint/BnVirtualHal.h>

#include "Fingerprint.h"

namespace aidl::android::hardware::biometrics::fingerprint {

class VirtualHal : public BnVirtualHal {
  public:
    VirtualHal(Fingerprint* fp) : mFp(fp) {}

    ::ndk::ScopedAStatus setEnrollments(const std::vector<int32_t>& in_id) override;
    ::ndk::ScopedAStatus setEnrollmentHit(int32_t in_hit_id) override;
    ::ndk::ScopedAStatus setNextEnrollment(
            const ::aidl::android::hardware::biometrics::fingerprint::NextEnrollment&
                    in_next_enrollment) override;
    ::ndk::ScopedAStatus setAuthenticatorId(int64_t in_id) override;
    ::ndk::ScopedAStatus setChallenge(int64_t in_challenge) override;
    ::ndk::ScopedAStatus setOperationAuthenticateFails(bool in_fail) override;
    ::ndk::ScopedAStatus setOperationAuthenticateLatency(
            const std::vector<int32_t>& in_latency) override;
    ::ndk::ScopedAStatus setOperationAuthenticateDuration(int32_t in_duration) override;
    ::ndk::ScopedAStatus setOperationAuthenticateError(int32_t in_error) override;
    ::ndk::ScopedAStatus setOperationAuthenticateAcquired(
            const std::vector<AcquiredInfoAndVendorCode>& in_acquired) override;
    ::ndk::ScopedAStatus setOperationEnrollError(int32_t in_error) override;
    ::ndk::ScopedAStatus setOperationEnrollLatency(const std::vector<int32_t>& in_latency) override;
    ::ndk::ScopedAStatus setOperationDetectInteractionLatency(
            const std::vector<int32_t>& in_latency) override;
    ::ndk::ScopedAStatus setOperationDetectInteractionError(int32_t in_error) override;
    ::ndk::ScopedAStatus setOperationDetectInteractionDuration(int32_t in_duration) override;
    ::ndk::ScopedAStatus setOperationDetectInteractionAcquired(
            const std::vector<AcquiredInfoAndVendorCode>& in_acquired) override;
    ::ndk::ScopedAStatus setLockout(bool in_lockout) override;
    ::ndk::ScopedAStatus setLockoutEnable(bool in_enable) override;
    ::ndk::ScopedAStatus setLockoutTimedThreshold(int32_t in_threshold) override;
    ::ndk::ScopedAStatus setLockoutTimedDuration(int32_t in_duration) override;
    ::ndk::ScopedAStatus setLockoutPermanentThreshold(int32_t in_threshold) override;
    ::ndk::ScopedAStatus resetConfigurations() override;
    ::ndk::ScopedAStatus setType(
            ::aidl::android::hardware::biometrics::fingerprint::FingerprintSensorType in_type)
            override;
    ::ndk::ScopedAStatus setSensorId(int32_t in_id) override;
    ::ndk::ScopedAStatus setSensorStrength(SensorStrength in_strength) override;
    ::ndk::ScopedAStatus setMaxEnrollmentPerUser(int32_t in_max) override;
    ::ndk::ScopedAStatus setSensorLocation(
            const ::aidl::android::hardware::biometrics::fingerprint::SensorLocation& in_loc)
            override;
    ::ndk::ScopedAStatus setNavigationGuesture(bool in_v) override;
    ::ndk::ScopedAStatus setDetectInteraction(bool in_v) override;
    ::ndk::ScopedAStatus setDisplayTouch(bool in_v) override;
    ::ndk::ScopedAStatus setControlIllumination(bool in_v) override;

  private:
    OptIntVec intVec2OptIntVec(const std::vector<int32_t>& intVec);
    OptIntVec acquiredInfoVec2OptIntVec(const std::vector<AcquiredInfoAndVendorCode>& intVec);
    ::ndk::ScopedAStatus sanityCheckLatency(const std::vector<int32_t>& in_latency);
    Fingerprint* mFp;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
