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

#include <unordered_map>

#include "VirtualHal.h"

#include <android-base/logging.h>

#include "util/CancellationSignal.h"

#undef LOG_TAG
#define LOG_TAG "FingerprintVirtualHalAidl"

namespace aidl::android::hardware::biometrics::fingerprint {

using Tag = AcquiredInfoAndVendorCode::Tag;

::ndk::ScopedAStatus VirtualHal::setEnrollments(const std::vector<int32_t>& enrollments) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", intVec2OptIntVec(enrollments));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setEnrollmentHit(int32_t enrollment_hit) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", enrollment_hit);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setNextEnrollment(
        const ::aidl::android::hardware::biometrics::fingerprint::NextEnrollment& next_enrollment) {
    Fingerprint::cfg().sourcedFromAidl();
    std::ostringstream os;
    os << next_enrollment.id << ":";

    int stepSize = next_enrollment.progressSteps.size();
    for (int i = 0; i < stepSize; i++) {
        auto& step = next_enrollment.progressSteps[i];
        os << step.durationMs;
        int acSize = step.acquiredInfoAndVendorCodes.size();
        for (int j = 0; j < acSize; j++) {
            if (j == 0) os << "-[";
            auto& acquiredInfoAndVendorCode = step.acquiredInfoAndVendorCodes[j];
            if (acquiredInfoAndVendorCode.getTag() == AcquiredInfoAndVendorCode::vendorCode)
                os << acquiredInfoAndVendorCode.get<Tag::vendorCode>();
            else if (acquiredInfoAndVendorCode.getTag() == AcquiredInfoAndVendorCode::acquiredInfo)
                os << (int)acquiredInfoAndVendorCode.get<Tag::acquiredInfo>();
            else
                LOG(FATAL) << "ERROR: wrong AcquiredInfoAndVendorCode union tag";
            if (j == acSize - 1)
                os << "]";
            else
                os << ",";
        }
        if (i == stepSize - 1)
            os << ":";
        else
            os << ",";
    }

    os << (next_enrollment.result ? "true" : "false");
    Fingerprint::cfg().set<std::string>("next_enrollment", os.str());
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setAuthenticatorId(int64_t in_id) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int64_t>("authenticator_id", in_id);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setChallenge(int64_t in_challenge) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int64_t>("challenge", in_challenge);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationAuthenticateFails(bool in_fail) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("operation_authenticate_fails", in_fail);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationAuthenticateLatency(
        const std::vector<int32_t>& in_latency) {
    ndk::ScopedAStatus status = sanityCheckLatency(in_latency);
    if (!status.isOk()) {
        return status;
    }

    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("operation_authenticate_latency",
                                         intVec2OptIntVec(in_latency));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationAuthenticateDuration(int32_t in_duration) {
    if (in_duration < 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER, "Error: duration can not be negative"));
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("operation_authenticate_duration", in_duration);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationAuthenticateError(int32_t in_error) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("operation_authenticate_error", in_error);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationAuthenticateAcquired(
        const std::vector<AcquiredInfoAndVendorCode>& in_acquired) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("operation_authenticate_acquired",
                                         acquiredInfoVec2OptIntVec(in_acquired));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationEnrollError(int32_t in_error) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("operation_enroll_error", in_error);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationEnrollLatency(const std::vector<int32_t>& in_latency) {
    ndk::ScopedAStatus status = sanityCheckLatency(in_latency);
    if (!status.isOk()) {
        return status;
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("operation_enroll_latency", intVec2OptIntVec(in_latency));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationDetectInteractionLatency(
        const std::vector<int32_t>& in_latency) {
    ndk::ScopedAStatus status = sanityCheckLatency(in_latency);
    if (!status.isOk()) {
        return status;
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interact_latency",
                                         intVec2OptIntVec(in_latency));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationDetectInteractionError(int32_t in_error) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("operation_detect_interaction_error", in_error);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationDetectInteractionDuration(int32_t in_duration) {
    if (in_duration < 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER, "Error: duration can not be negative"));
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("operation_detect_interaction_duration", in_duration);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setOperationDetectInteractionAcquired(
        const std::vector<AcquiredInfoAndVendorCode>& in_acquired) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_acquired",
                                         acquiredInfoVec2OptIntVec(in_acquired));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setLockout(bool in_lockout) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("lockout", in_lockout);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setLockoutEnable(bool in_enable) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("lockout_enable", in_enable);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setLockoutTimedThreshold(int32_t in_threshold) {
    if (in_threshold < 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER, "Error: threshold can not be negative"));
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("lockout_timed_threshold", in_threshold);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setLockoutTimedDuration(int32_t in_duration) {
    if (in_duration < 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER, "Error: duration can not be negative"));
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("lockout_timed_duration", in_duration);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setLockoutPermanentThreshold(int32_t in_threshold) {
    if (in_threshold < 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER, "Error: threshold can not be negative"));
    }
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("lockout_permanent_threshold", in_threshold);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::resetConfigurations() {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().init();
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setType(
        ::aidl::android::hardware::biometrics::fingerprint::FingerprintSensorType in_type) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<std::string>("type", Fingerprint::type2String(in_type));
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setSensorId(int32_t in_id) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("sensor_id", in_id);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setSensorStrength(SensorStrength in_strength) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("sensor_strength", (int32_t)in_strength);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setMaxEnrollmentPerUser(int32_t in_max) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<int32_t>("max_enrollments", in_max);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setSensorLocation(const SensorLocation& in_loc) {
    std::string str = std::to_string(in_loc.sensorLocationX) + ":" +
                      std::to_string(in_loc.sensorLocationY) + ":" +
                      std::to_string(in_loc.sensorRadius);
    ;
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<std::string>("sensor_location", str);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setNavigationGuesture(bool in_v) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("navigation_guesture", in_v);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setDetectInteraction(bool in_v) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("detect_interaction", in_v);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setDisplayTouch(bool in_v) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("display_touch", in_v);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus VirtualHal::setControlIllumination(bool in_v) {
    Fingerprint::cfg().sourcedFromAidl();
    Fingerprint::cfg().set<bool>("control_illumination", in_v);
    return ndk::ScopedAStatus::ok();
}

OptIntVec VirtualHal::intVec2OptIntVec(const std::vector<int32_t>& in_vec) {
    OptIntVec optIntVec;
    std::transform(in_vec.begin(), in_vec.end(), std::back_inserter(optIntVec),
                   [](int value) { return std::optional<int>(value); });
    return optIntVec;
}

OptIntVec VirtualHal::acquiredInfoVec2OptIntVec(
        const std::vector<AcquiredInfoAndVendorCode>& in_vec) {
    OptIntVec optIntVec;
    std::transform(in_vec.begin(), in_vec.end(), std::back_inserter(optIntVec),
                   [](AcquiredInfoAndVendorCode ac) {
                       int value;
                       if (ac.getTag() == AcquiredInfoAndVendorCode::acquiredInfo)
                           value = (int)ac.get<Tag::acquiredInfo>();
                       else if (ac.getTag() == AcquiredInfoAndVendorCode::vendorCode)
                           value = ac.get<Tag::vendorCode>();
                       else
                           LOG(FATAL) << "ERROR: wrong AcquiredInfoAndVendorCode tag";
                       return std::optional<int>(value);
                   });
    return optIntVec;
}

::ndk::ScopedAStatus VirtualHal::sanityCheckLatency(const std::vector<int32_t>& in_latency) {
    if (in_latency.size() == 0 || in_latency.size() > 2) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IVirtualHal::STATUS_INVALID_PARAMETER,
                "Error: input input array must contain 1 or 2 elements"));
    }

    for (auto x : in_latency) {
        if (x < 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IVirtualHal::STATUS_INVALID_PARAMETER,
                    "Error: input data must not be negative"));
        }
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
