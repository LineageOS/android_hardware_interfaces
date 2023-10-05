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
    // format is "<id>,<bucket_id>:<delay>:<succeeds>,<bucket_id>:<delay>:<succeeds>...
    auto nextEnroll = FaceHalProperties::next_enrollment().value_or("");
    // Erase the next enrollment
    FaceHalProperties::next_enrollment({});

    AuthenticationFrame frame;
    frame.data.acquiredInfo = AcquiredInfo::START;
    frame.data.vendorCode = 0;
    cb->onAuthenticationFrame(frame);

    // Do proper HAT verification in the real implementation.
    if (hat.mac.empty()) {
        LOG(ERROR) << "Fail: hat";
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    if (FaceHalProperties::operation_enroll_fails().value_or(false)) {
        LOG(ERROR) << "Fail: operation_enroll_fails";
        cb->onError(Error::VENDOR, 0 /* vendorError */);
        return;
    }

    auto parts = Util::split(nextEnroll, ",");
    if (parts.size() < 2) {
        LOG(ERROR) << "Fail: invalid next_enrollment for : " << nextEnroll;
        cb->onError(Error::VENDOR, 0 /* vendorError */);
        return;
    }

    auto enrollmentId = std::stoi(parts[0]);
    const int numBuckets = parts.size() - 1;
    for (size_t i = 1; i < parts.size(); i++) {
        auto enrollHit = Util::split(parts[i], ":");
        if (enrollHit.size() != 3) {
            LOG(ERROR) << "Error when unpacking enrollment hit: " << parts[i];
            cb->onError(Error::VENDOR, 0 /* vendorError */);
        }
        std::string bucket = enrollHit[0];
        std::string delay = enrollHit[1];
        std::string succeeds = enrollHit[2];

        SLEEP_MS(std::stoi(delay));

        if (shouldCancel(cancel)) {
            LOG(ERROR) << "Fail: cancel";
            cb->onError(Error::CANCELED, 0 /* vendorCode */);
            return;
        }

        if (!IS_TRUE(succeeds)) {  // end and failed
            LOG(ERROR) << "Fail: requested by caller: " << parts[i];
            cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
            return;
        }

        EnrollmentFrame frame;

        frame.data.acquiredInfo = AcquiredInfo::GOOD;
        frame.data.vendorCode = 0;
        cb->onEnrollmentFrame(frame);

        frame.data.acquiredInfo = AcquiredInfo::VENDOR;
        frame.data.vendorCode = std::stoi(bucket);
        cb->onEnrollmentFrame(frame);

        int remainingBuckets = numBuckets - i;
        if (remainingBuckets > 0) {
            cb->onEnrollmentProgress(enrollmentId, remainingBuckets);
        }
    }

    auto enrollments = FaceHalProperties::enrollments();
    enrollments.push_back(enrollmentId);
    FaceHalProperties::enrollments(enrollments);
    LOG(INFO) << "enrolled : " << enrollmentId;
    cb->onEnrollmentProgress(enrollmentId, 0);
}

void FakeFaceEngine::authenticateImpl(ISessionCallback* cb, int64_t /*operationId*/,
                                      const std::future<void>& cancel) {
    BEGIN_OP(FaceHalProperties::operation_authenticate_latency().value_or(0));

    // Signal to the framework that we have begun authenticating.
    AuthenticationFrame frame;
    frame.data.acquiredInfo = AcquiredInfo::START;
    frame.data.vendorCode = 0;
    cb->onAuthenticationFrame(frame);

    // Also signal that we have opened the camera.
    frame = {};
    frame.data.acquiredInfo = AcquiredInfo::FIRST_FRAME_RECEIVED;
    frame.data.vendorCode = 0;
    cb->onAuthenticationFrame(frame);

    auto now = Util::getSystemNanoTime();
    int64_t duration = FaceHalProperties::operation_authenticate_duration().value_or(0);
    if (duration > 0) {
        do {
            SLEEP_MS(5);
        } while (!Util::hasElapsed(now, duration));
    }

    if (FaceHalProperties::operation_authenticate_fails().value_or(false)) {
        LOG(ERROR) << "Fail: operation_authenticate_fails";
        cb->onError(Error::VENDOR, 0 /* vendorError */);
        return;
    }

    if (FaceHalProperties::lockout().value_or(false)) {
        LOG(ERROR) << "Fail: lockout";
        cb->onLockoutPermanent();
        cb->onError(Error::HW_UNAVAILABLE, 0 /* vendorError */);
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
    if (id < 0 || !isEnrolled) {
        LOG(ERROR) << (isEnrolled ? "invalid enrollment hit" : "Fail: not enrolled");
        cb->onAuthenticationFailed();
        cb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorError */);
        return;
    }

    cb->onAuthenticationSucceeded(id, {} /* hat */);
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
    cb->onLockoutCleared();
}

}  // namespace aidl::android::hardware::biometrics::face