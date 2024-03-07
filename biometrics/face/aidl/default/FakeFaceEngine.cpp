/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "FaceVirtualHalEngine"

#include "FakeFaceEngine.h"

#include <android-base/logging.h>

#include <face.sysprop.h>

#include "util/CancellationSignal.h"
#include "util/Util.h"

using namespace ::android::face::virt;

namespace aidl::android::hardware::biometrics::face {

FaceSensorType FakeFaceEngine::GetSensorType() {
    std::string type = FaceHalProperties::type().value_or("");
    if (type == "IR") {
        return FaceSensorType::IR;
    } else {
        FaceHalProperties::type("RGB");
        return FaceSensorType::RGB;
    }
}

common::SensorStrength FakeFaceEngine::GetSensorStrength() {
    std::string strength = FaceHalProperties::strength().value_or("");
    if (strength == "convenience") {
        return common::SensorStrength::CONVENIENCE;
    } else if (strength == "weak") {
        return common::SensorStrength::WEAK;
    } else {
        FaceHalProperties::strength("strong");
        return common::SensorStrength::STRONG;
    }
}

void FakeFaceEngine::generateChallengeImpl(ISessionCallback* cb) {
    BEGIN_OP(0);
    std::uniform_int_distribution<int64_t> dist;
    auto challenge = dist(mRandom);
    FaceHalProperties::challenge(challenge);
    cb->onChallengeGenerated(challenge);
}

void FakeFaceEngine::revokeChallengeImpl(ISessionCallback* cb, int64_t challenge) {
    BEGIN_OP(0);
    FaceHalProperties::challenge({});
    cb->onChallengeRevoked(challenge);
}
void FakeFaceEngine::getEnrollmentConfigImpl(ISessionCallback* /*cb*/,
                                             std::vector<EnrollmentStageConfig>* /*return_val*/) {}
void FakeFaceEngine::enrollImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                                EnrollmentType /*enrollmentType*/,
                                const std::vector<Feature>& /*features*/,
                                const std::future<void>& cancel) {
    BEGIN_OP(FaceHalProperties::operation_start_enroll_latency().value_or(0));

    // Do proper HAT verification in the real implementation.
    if (hat.mac.empty()) {
        LOG(ERROR) << "Fail: hat";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    // Format: <id>:<progress_ms-[acquiredInfo,...],...:<success>
    // ------:-----------------------------------------:--------------
    //          |           |                              |--->enrollment success (true/false)
    //          |           |--> progress_steps
    //          |
    //          |-->enrollment id
    //
    //
    //   progress_steps
    //        <progress_duration>-[acquiredInfo,...]+
    //        ----------------------------  ---------------------
    //                 |                            |-> sequence of acquiredInfo code
    //                 | --> time duration of the step in ms
    //
    //        E.g.   1:2000-[21,1108,5,6,1],1000-[1113,4,1]:true
    //              A success enrollement of id 1 by 2 steps
    //                    1st step lasts 2000ms with acquiredInfo codes (21,1108,5,6,1)
    //                    2nd step lasts 1000ms with acquiredInfo codes (1113,4,1)
    //
    std::string defaultNextEnrollment =
            "1:1000-[21,7,1,1103],1500-[1108,1],2000-[1113,1],2500-[1118,1]:true";
    auto nextEnroll = FaceHalProperties::next_enrollment().value_or(defaultNextEnrollment);
    auto parts = Util::split(nextEnroll, ":");
    if (parts.size() != 3) {
        LOG(ERROR) << "Fail: invalid next_enrollment:" << nextEnroll;
        cb->onError(Error::VENDOR, 0 /* vendorError */);
        return;
    }
    auto enrollmentId = std::stoi(parts[0]);
    auto progress = Util::parseEnrollmentCapture(parts[1]);
    for (size_t i = 0; i < progress.size(); i += 2) {
        auto left = (progress.size() - i) / 2 - 1;
        auto duration = progress[i][0];
        auto acquired = progress[i + 1];
        auto N = acquired.size();

        for (int j = 0; j < N; j++) {
            SLEEP_MS(duration / N);

            if (shouldCancel(cancel)) {
                LOG(ERROR) << "Fail: cancel";
                cb->onError(Error::CANCELED, 0 /* vendorCode */);
                return;
            }
            EnrollmentFrame frame = {};
            auto ac = convertAcquiredInfo(acquired[j]);
            frame.data.acquiredInfo = ac.first;
            frame.data.vendorCode = ac.second;
            frame.stage = (i == 0 && j == 0) ? EnrollmentStage::FIRST_FRAME_RECEIVED
                          : (i == progress.size() - 2 && j == N - 1)
                                  ? EnrollmentStage::ENROLLMENT_FINISHED
                                  : EnrollmentStage::WAITING_FOR_CENTERING;
            cb->onEnrollmentFrame(frame);
        }

        if (left == 0 && !IS_TRUE(parts[2])) {  // end and failed
            LOG(ERROR) << "Fail: requested by caller: " << nextEnroll;
            FaceHalProperties::next_enrollment({});
            cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
        } else {  // progress and update props if last time
            LOG(INFO) << "onEnroll: " << enrollmentId << " left: " << left;
            if (left == 0) {
                auto enrollments = FaceHalProperties::enrollments();
                enrollments.emplace_back(enrollmentId);
                FaceHalProperties::enrollments(enrollments);
                FaceHalProperties::next_enrollment({});
                // change authenticatorId after new enrollment
                auto id = FaceHalProperties::authenticator_id().value_or(0);
                auto newId = id + 1;
                FaceHalProperties::authenticator_id(newId);
                LOG(INFO) << "Enrolled: " << enrollmentId;
            }
            cb->onEnrollmentProgress(enrollmentId, left);
        }
    }
}

void FakeFaceEngine::authenticateImpl(ISessionCallback* cb, int64_t /*operationId*/,
                                      const std::future<void>& cancel) {
    BEGIN_OP(FaceHalProperties::operation_authenticate_latency().value_or(0));

    auto id = FaceHalProperties::enrollment_hit().value_or(0);
    auto enrolls = FaceHalProperties::enrollments();
    auto isEnrolled = std::find(enrolls.begin(), enrolls.end(), id) != enrolls.end();

    auto vec2str = [](std::vector<AcquiredInfo> va) {
        std::stringstream ss;
        bool isFirst = true;
        for (auto ac : va) {
            if (!isFirst) ss << ",";
            ss << std::to_string((int8_t)ac);
            isFirst = false;
        }
        return ss.str();
    };

    // default behavior mimic face sensor in U
    int64_t defaultAuthDuration = 500;
    std::string defaultAcquiredInfo =
            vec2str({AcquiredInfo::START, AcquiredInfo::FIRST_FRAME_RECEIVED});
    if (!isEnrolled) {
        std::vector<AcquiredInfo> v;
        for (int i = 0; i < 56; i++) v.push_back(AcquiredInfo::NOT_DETECTED);
        defaultAcquiredInfo += "," + vec2str(v);
        defaultAuthDuration = 2100;
    } else {
        defaultAcquiredInfo += "," + vec2str({AcquiredInfo::TOO_BRIGHT, AcquiredInfo::TOO_BRIGHT,
                                              AcquiredInfo::TOO_BRIGHT, AcquiredInfo::TOO_BRIGHT,
                                              AcquiredInfo::GOOD, AcquiredInfo::GOOD});
    }

    int64_t now = Util::getSystemNanoTime();
    int64_t duration =
            FaceHalProperties::operation_authenticate_duration().value_or(defaultAuthDuration);
    auto acquired =
            FaceHalProperties::operation_authenticate_acquired().value_or(defaultAcquiredInfo);
    auto acquiredInfos = Util::parseIntSequence(acquired);
    int N = acquiredInfos.size();

    if (N == 0) {
        LOG(ERROR) << "Fail to parse authentiate acquired info: " + acquired;
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    if (mLockoutTracker.checkIfLockout(cb)) {
        return;
    }

    int i = 0;
    do {
        if (FaceHalProperties::lockout().value_or(false)) {
            LOG(ERROR) << "Fail: lockout";
            cb->onLockoutPermanent();
            cb->onError(Error::HW_UNAVAILABLE, 0 /* vendorError */);
            return;
        }

        if (FaceHalProperties::operation_authenticate_fails().value_or(false)) {
            LOG(ERROR) << "Fail: operation_authenticate_fails";
            mLockoutTracker.addFailedAttempt(cb);
            cb->onAuthenticationFailed();
            return;
        }

        auto err = FaceHalProperties::operation_authenticate_error().value_or(0);
        if (err != 0) {
            LOG(ERROR) << "Fail: operation_authenticate_error";
            auto ec = convertError(err);
            cb->onError(ec.first, ec.second);
            return; /* simply terminating current operation for any user inserted error,
                            revisit if tests need*/
        }

        if (shouldCancel(cancel)) {
            LOG(ERROR) << "Fail: cancel";
            cb->onError(Error::CANCELED, 0 /* vendorCode */);
            return;
        }

        if (i < N) {
            auto ac = convertAcquiredInfo(acquiredInfos[i]);
            AuthenticationFrame frame;
            frame.data.acquiredInfo = ac.first;
            frame.data.vendorCode = ac.second;
            cb->onAuthenticationFrame(frame);
            LOG(INFO) << "AcquiredInfo:" << i << ": (" << (int)ac.first << "," << (int)ac.second
                      << ")";
            i++;
        }

        SLEEP_MS(duration / N);
    } while (!Util::hasElapsed(now, duration));

    if (id > 0 && isEnrolled) {
        mLockoutTracker.reset();
        cb->onAuthenticationSucceeded(id, {} /* hat */);
        return;
    } else {
        LOG(ERROR) << "Fail: face not enrolled";
        mLockoutTracker.addFailedAttempt(cb);
        cb->onAuthenticationFailed();
        cb->onError(Error::TIMEOUT, 0 /* vendorError*/);
        return;
    }
}

std::pair<AcquiredInfo, int32_t> FakeFaceEngine::convertAcquiredInfo(int32_t code) {
    std::pair<AcquiredInfo, int32_t> res;
    if (code > FACE_ACQUIRED_VENDOR_BASE) {
        res.first = AcquiredInfo::VENDOR;
        res.second = code - FACE_ACQUIRED_VENDOR_BASE;
    } else {
        res.first = (AcquiredInfo)code;
        res.second = 0;
    }
    return res;
}

std::pair<Error, int32_t> FakeFaceEngine::convertError(int32_t code) {
    std::pair<Error, int32_t> res;
    if (code > FACE_ERROR_VENDOR_BASE) {
        res.first = Error::VENDOR;
        res.second = code - FACE_ERROR_VENDOR_BASE;
    } else {
        res.first = (Error)code;
        res.second = 0;
    }
    return res;
}

void FakeFaceEngine::detectInteractionImpl(ISessionCallback* cb, const std::future<void>& cancel) {
    BEGIN_OP(FaceHalProperties::operation_detect_interaction_latency().value_or(0));

    if (FaceHalProperties::operation_detect_interaction_fails().value_or(false)) {
        LOG(ERROR) << "Fail: operation_detect_interaction_fails";
        cb->onError(Error::VENDOR, 0 /* vendorError */);
        return;
    }

    if (shouldCancel(cancel)) {
        LOG(ERROR) << "Fail: cancel";
        cb->onError(Error::CANCELED, 0 /* vendorCode */);
        return;
    }

    auto id = FaceHalProperties::enrollment_hit().value_or(0);
    auto enrolls = FaceHalProperties::enrollments();
    auto isEnrolled = std::find(enrolls.begin(), enrolls.end(), id) != enrolls.end();
    if (id <= 0 || !isEnrolled) {
        LOG(ERROR) << "Fail: not enrolled";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    cb->onInteractionDetected();
}

void FakeFaceEngine::enumerateEnrollmentsImpl(ISessionCallback* cb) {
    BEGIN_OP(0);
    std::vector<int32_t> enrollments;
    for (const auto& enrollmentId : FaceHalProperties::enrollments()) {
        if (enrollmentId) {
            enrollments.push_back(*enrollmentId);
        }
    }
    cb->onEnrollmentsEnumerated(enrollments);
}

void FakeFaceEngine::removeEnrollmentsImpl(ISessionCallback* cb,
                                           const std::vector<int32_t>& enrollmentIds) {
    BEGIN_OP(0);

    std::vector<std::optional<int32_t>> newEnrollments;
    for (const auto& enrollment : FaceHalProperties::enrollments()) {
        auto id = enrollment.value_or(0);
        if (std::find(enrollmentIds.begin(), enrollmentIds.end(), id) == enrollmentIds.end()) {
            newEnrollments.emplace_back(id);
        }
    }
    FaceHalProperties::enrollments(newEnrollments);
    cb->onEnrollmentsRemoved(enrollmentIds);
}

void FakeFaceEngine::getFeaturesImpl(ISessionCallback* cb) {
    BEGIN_OP(0);

    if (FaceHalProperties::enrollments().empty()) {
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
        return;
    }

    std::vector<Feature> featuresToReturn = {};
    for (const auto& feature : FaceHalProperties::features()) {
        if (feature) {
            featuresToReturn.push_back((Feature)(*feature));
        }
    }
    cb->onFeaturesRetrieved(featuresToReturn);
}

void FakeFaceEngine::setFeatureImpl(ISessionCallback* cb, const keymaster::HardwareAuthToken& hat,
                                    Feature feature, bool enabled) {
    BEGIN_OP(0);

    if (FaceHalProperties::enrollments().empty()) {
        LOG(ERROR) << "Unable to set feature, enrollments are empty";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
        return;
    }

    if (hat.mac.empty()) {
        LOG(ERROR) << "Unable to set feature, invalid hat";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
        return;
    }

    auto features = FaceHalProperties::features();

    auto itr = std::find_if(features.begin(), features.end(), [feature](const auto& theFeature) {
        return *theFeature == (int)feature;
    });

    if (!enabled && (itr != features.end())) {
        features.erase(itr);
    } else if (enabled && (itr == features.end())) {
        features.push_back((int)feature);
    }

    FaceHalProperties::features(features);
    cb->onFeatureSet(feature);
}

void FakeFaceEngine::getAuthenticatorIdImpl(ISessionCallback* cb) {
    BEGIN_OP(0);
    // If this is a weak HAL return 0 per the spec.
    if (GetSensorStrength() != common::SensorStrength::STRONG) {
        cb->onAuthenticatorIdRetrieved(0);
    } else {
        cb->onAuthenticatorIdRetrieved(FaceHalProperties::authenticator_id().value_or(0));
    }
}

void FakeFaceEngine::invalidateAuthenticatorIdImpl(ISessionCallback* cb) {
    BEGIN_OP(0);
    int64_t authenticatorId = FaceHalProperties::authenticator_id().value_or(0);
    int64_t newId = authenticatorId + 1;
    FaceHalProperties::authenticator_id(newId);
    cb->onAuthenticatorIdInvalidated(newId);
}

void FakeFaceEngine::resetLockoutImpl(ISessionCallback* cb,
                                      const keymaster::HardwareAuthToken& /*hat*/) {
    BEGIN_OP(0);
    FaceHalProperties::lockout(false);
    mLockoutTracker.reset();
    cb->onLockoutCleared();
}

}  // namespace aidl::android::hardware::biometrics::face
