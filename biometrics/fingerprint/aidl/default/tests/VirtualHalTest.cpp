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

#include <android/binder_process.h>
#include <fingerprint.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "Fingerprint.h"
#include "VirtualHal.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;

namespace aidl::android::hardware::biometrics::fingerprint {

class VirtualHalTest : public ::testing::Test {
  public:
    static const int32_t STATUS_FAILED_TO_SET_PARAMETER = 2;

  protected:
    void SetUp() override {
        mHal = ndk::SharedRefBase::make<Fingerprint>();
        mVhal = ndk::SharedRefBase::make<VirtualHal>(mHal.get());
        ASSERT_TRUE(mVhal != nullptr);
        mHal->resetConfigToDefault();
    }

    void TearDown() override { mHal->resetConfigToDefault(); }

    std::shared_ptr<VirtualHal> mVhal;

    ndk::ScopedAStatus validateNonNegativeInputOfInt32(const char* name,
                                                       ndk::ScopedAStatus (VirtualHal::*f)(int32_t),
                                                       const std::vector<int32_t>& in_good);

  private:
    std::shared_ptr<Fingerprint> mHal;
};

ndk::ScopedAStatus VirtualHalTest::validateNonNegativeInputOfInt32(
        const char* name, ndk::ScopedAStatus (VirtualHal::*f)(int32_t),
        const std::vector<int32_t>& in_params_good) {
    ndk::ScopedAStatus status;
    for (auto& param : in_params_good) {
        status = (*mVhal.*f)(param);
        if (!status.isOk()) return status;
        if (Fingerprint::cfg().get<int32_t>(name) != param) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                    "Error: fail to set non-negative parameter"));
        }
    }

    int32_t old_param = Fingerprint::cfg().get<int32_t>(name);
    status = (*mVhal.*f)(-1);
    if (status.isOk()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER, "Error: should return NOK"));
    }
    if (status.getServiceSpecificError() != IVirtualHal::STATUS_INVALID_PARAMETER) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                "Error: unexpected return error code"));
    }
    if (Fingerprint::cfg().get<int32_t>(name) != old_param) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                VirtualHalTest::STATUS_FAILED_TO_SET_PARAMETER,
                "Error: unexpected parameter change on failed attempt"));
    }
    return ndk::ScopedAStatus::ok();
}

TEST_F(VirtualHalTest, init) {
    mVhal->setLockout(false);
    ASSERT_TRUE(Fingerprint::cfg().get<bool>("lockout") == false);
    ASSERT_TRUE(Fingerprint::cfg().get<std::string>("type") == "rear");
    ASSERT_TRUE(Fingerprint::cfg().get<std::int32_t>("sensor_strength") == 2);
    std::int64_t id = Fingerprint::cfg().get<std::int64_t>("authenticator_id");
    ASSERT_TRUE(Fingerprint::cfg().get<std::int64_t>("authenticator_id") == 0);
    ASSERT_TRUE(Fingerprint::cfg().getopt<OptIntVec>("enrollments") == OptIntVec());
}

TEST_F(VirtualHalTest, enrollment_hit_int32) {
    mVhal->setEnrollmentHit(11);
    ASSERT_TRUE(Fingerprint::cfg().get<int32_t>("enrollment_hit") == 11);
}

TEST_F(VirtualHalTest, next_enrollment) {
    struct {
        std::string nextEnrollmentStr;
        fingerprint::NextEnrollment nextEnrollment;
    } testData[] = {
            {"1:20:true", {1, {{20}}, true}},
            {"1:50,60,70:true", {1, {{50}, {60}, {70}}, true}},
            {"2:50-[8],60,70-[2,1002,1]:false",
             {2,
              {{50, {{AcquiredInfo::START}}},
               {60},
               {70, {{AcquiredInfo::PARTIAL}, {1002}, {AcquiredInfo::GOOD}}}},
              false}},
    };

    for (auto& d : testData) {
        mVhal->setNextEnrollment(d.nextEnrollment);
        ASSERT_TRUE(Fingerprint::cfg().get<std::string>("next_enrollment") == d.nextEnrollmentStr);
    }
}

TEST_F(VirtualHalTest, authenticator_id_int64) {
    mVhal->setAuthenticatorId(12345678900);
    ASSERT_TRUE(Fingerprint::cfg().get<int64_t>("authenticator_id") == 12345678900);
}

TEST_F(VirtualHalTest, opeationAuthenticateFails_bool) {
    mVhal->setOperationAuthenticateFails(true);
    ASSERT_TRUE(Fingerprint::cfg().get<bool>("operation_authenticate_fails"));
}

TEST_F(VirtualHalTest, operationAuthenticateAcquired_int32_vector) {
    using Tag = AcquiredInfoAndVendorCode::Tag;
    std::vector<AcquiredInfoAndVendorCode> ac{
            {AcquiredInfo::START}, {AcquiredInfo::PARTIAL}, {1023}};
    mVhal->setOperationAuthenticateAcquired(ac);
    OptIntVec ac_get = Fingerprint::cfg().getopt<OptIntVec>("operation_authenticate_acquired");
    ASSERT_TRUE(ac_get.size() == ac.size());
    for (int i = 0; i < ac.size(); i++) {
        int acCode = (ac[i].getTag() == Tag::acquiredInfo) ? (int)ac[i].get<Tag::acquiredInfo>()
                                                           : ac[i].get<Tag::vendorCode>();
        ASSERT_TRUE(acCode == ac_get[i]);
    }
}

TEST_F(VirtualHalTest, type) {
    struct {
        FingerprintSensorType type;
        const char* typeStr;
    } typeMap[] = {{FingerprintSensorType::REAR, "rear"},
                   {FingerprintSensorType::POWER_BUTTON, "side"},
                   {FingerprintSensorType::UNDER_DISPLAY_OPTICAL, "udfps"},
                   {FingerprintSensorType::UNDER_DISPLAY_ULTRASONIC, "udfps"},
                   {FingerprintSensorType::UNKNOWN, "unknown"}};
    for (auto const& x : typeMap) {
        mVhal->setType(x.type);
        ASSERT_TRUE(Fingerprint::cfg().get<std::string>("type") == x.typeStr);
    }
}

TEST_F(VirtualHalTest, sensorStrength) {
    SensorStrength strengths[] = {SensorStrength::CONVENIENCE, SensorStrength::WEAK,
                                  SensorStrength::STRONG};

    for (auto const& strength : strengths) {
        mVhal->setSensorStrength(strength);
        ASSERT_TRUE(Fingerprint::cfg().get<int32_t>("sensor_strength") == (int32_t)(strength));
    }
}

TEST_F(VirtualHalTest, sensorLocation) {
    SensorLocation loc = {.sensorLocationX = 1, .sensorLocationY = 2, .sensorRadius = 3};
    mVhal->setSensorLocation(loc);
    ASSERT_TRUE(Fingerprint::cfg().get<std::string>("sensor_location") == "1:2:3");
}

TEST_F(VirtualHalTest, setLatency) {
    ndk::ScopedAStatus status;
    std::vector<int32_t> in_lats[] = {{1}, {2, 3}, {5, 4}};
    for (auto const& in_lat : in_lats) {
        status = mVhal->setOperationAuthenticateLatency(in_lat);
        ASSERT_TRUE(status.isOk());
        OptIntVec out_lat = Fingerprint::cfg().getopt<OptIntVec>("operation_authenticate_latency");
        ASSERT_TRUE(in_lat.size() == out_lat.size());
        for (int i = 0; i < in_lat.size(); i++) {
            ASSERT_TRUE(in_lat[i] == out_lat[i]);
        }
    }

    std::vector<int32_t> bad_in_lats[] = {{}, {1, 2, 3}, {1, -3}};
    for (auto const& in_lat : bad_in_lats) {
        status = mVhal->setOperationAuthenticateLatency(in_lat);
        ASSERT_TRUE(!status.isOk());
        ASSERT_TRUE(status.getServiceSpecificError() == IVirtualHal::STATUS_INVALID_PARAMETER);
    }
}

TEST_F(VirtualHalTest, setOperationAuthenticateDuration) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "operation_authenticate_duration", &IVirtualHal::setOperationAuthenticateDuration,
            {0, 33});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setOperationDetectInteractionDuration) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "operation_detect_interaction_duration",
            &IVirtualHal::setOperationDetectInteractionDuration, {0, 34});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutTimedDuration) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_timed_duration", &IVirtualHal::setLockoutTimedDuration, {0, 35});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutTimedThreshold) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_timed_threshold", &IVirtualHal::setLockoutTimedThreshold, {0, 36});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setLockoutPermanentThreshold) {
    ndk::ScopedAStatus status = validateNonNegativeInputOfInt32(
            "lockout_permanent_threshold", &IVirtualHal::setLockoutPermanentThreshold, {0, 37});
    ASSERT_TRUE(status.isOk());
}

TEST_F(VirtualHalTest, setOthers) {
    // Verify that there is no CHECK() failures
    mVhal->setEnrollments({7, 6, 5});
    mVhal->setChallenge(111222333444555666);
    mVhal->setOperationAuthenticateError(4);
    mVhal->setOperationEnrollError(5);
    mVhal->setOperationEnrollLatency({4, 5});
    mVhal->setOperationDetectInteractionError(6);
    mVhal->setOperationDetectInteractionAcquired({{AcquiredInfo::START}, {AcquiredInfo::GOOD}});
    mVhal->setLockout(false);
    mVhal->setLockoutEnable(false);
    mVhal->setSensorId(5);
    mVhal->setMaxEnrollmentPerUser(6);
    mVhal->setNavigationGuesture(false);
    mVhal->setDetectInteraction(false);
    mVhal->setDisplayTouch(false);
    mVhal->setControlIllumination(false);
}

}  // namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
