/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "**** HAL log ****"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android/hardware/automotive/occupant_awareness/BnOccupantAwarenessClientCallback.h>
#include <android/hardware/automotive/occupant_awareness/IOccupantAwareness.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <vector>

using namespace android::hardware::automotive::occupant_awareness;
using android::hardware::automotive::occupant_awareness::IOccupantAwareness;

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;

constexpr auto kTimeout = std::chrono::seconds(3);

#define EXPECT_OK(ret) ASSERT_TRUE((ret).isOk())

class OccupantAwarenessCallback : public BnOccupantAwarenessClientCallback {
  public:
    OccupantAwarenessCallback(const std::function<void(int, OccupantAwarenessStatus)>& callback)
        : mCallback(callback) {}
    Status onSystemStatusChanged(int detectionFlags, OccupantAwarenessStatus status) override {
        mCallback(detectionFlags, status);
        return Status::ok();
    }

    Status onDetectionEvent(const OccupantDetections& detections) override {
        (void)detections;
        return Status::ok();
    }

  private:
    std::function<void(int, OccupantAwarenessStatus)> mCallback;
};

class OccupantAwarenessAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mOccupantAwarenessService =
                android::waitForDeclaredService<IOccupantAwareness>(String16(GetParam().c_str()));
        ASSERT_NE(mOccupantAwarenessService, nullptr);
    }

    sp<IOccupantAwareness> mOccupantAwarenessService;
};

// Test that startDetection() returns within the timeout.
TEST_P(OccupantAwarenessAidl, StartDetectionTest) {
    auto start = std::chrono::system_clock::now();
    OccupantAwarenessStatus occupantAwarenessStatus;
    Status status = mOccupantAwarenessService->startDetection(&occupantAwarenessStatus);
    auto elapsed = std::chrono::system_clock::now() - start;
    EXPECT_OK(status);
    ASSERT_LE(elapsed, kTimeout);

    EXPECT_OK(mOccupantAwarenessService->stopDetection(&occupantAwarenessStatus));
}

// Test that getCapabilityForRole() returns supported capabilities for the role. The test only
// verifies that the IPC call returns successfully and does not verify the supported capabilities.
TEST_P(OccupantAwarenessAidl, GetCapabilityTest) {
    std::vector<Role> rolesToTest = {Role::FRONT_PASSENGER,        Role::DRIVER,
                                     Role::ROW_2_PASSENGER_LEFT,   Role::ROW_2_PASSENGER_CENTER,
                                     Role::ROW_2_PASSENGER_RIGHT,  Role::ROW_3_PASSENGER_LEFT,
                                     Role::ROW_3_PASSENGER_CENTER, Role::ROW_3_PASSENGER_RIGHT,
                                     Role::FRONT_OCCUPANTS,        Role::ROW_2_OCCUPANTS,
                                     Role::ROW_3_OCCUPANTS,        Role::ALL_OCCUPANTS};

    for (auto role : rolesToTest) {
        int32_t capabilities;
        EXPECT_OK(mOccupantAwarenessService->getCapabilityForRole(role, &capabilities));
    }
}

// Test that getCapabilityForRole() returns failure when arguments are invalid.
TEST_P(OccupantAwarenessAidl, GetCapabilityFailureTest) {
    int32_t capabilities;
    EXPECT_FALSE(
            mOccupantAwarenessService->getCapabilityForRole(Role::INVALID, &capabilities).isOk());

    Role invalidRole = static_cast<Role>(static_cast<int>(Role::ALL_OCCUPANTS) + 1);
    EXPECT_FALSE(
            mOccupantAwarenessService->getCapabilityForRole(invalidRole, &capabilities).isOk());
}

// Test that getState() returns within the timeout. The test do not attempt to verify the state, but
// only checks that the IPC call returns successfully.
TEST_P(OccupantAwarenessAidl, GetStateTest) {
    std::vector<Role> rolesToTest = {Role::FRONT_PASSENGER,        Role::DRIVER,
                                     Role::ROW_2_PASSENGER_LEFT,   Role::ROW_2_PASSENGER_CENTER,
                                     Role::ROW_2_PASSENGER_RIGHT,  Role::ROW_3_PASSENGER_LEFT,
                                     Role::ROW_3_PASSENGER_CENTER, Role::ROW_3_PASSENGER_RIGHT,
                                     Role::FRONT_OCCUPANTS,        Role::ROW_2_OCCUPANTS,
                                     Role::ROW_3_OCCUPANTS,        Role::ALL_OCCUPANTS};

    std::vector<int> detectionCapabilities = {IOccupantAwareness::CAP_PRESENCE_DETECTION,
                                              IOccupantAwareness::CAP_GAZE_DETECTION,
                                              IOccupantAwareness::CAP_DRIVER_MONITORING_DETECTION};

    for (auto role : rolesToTest) {
        for (auto detectionCapability : detectionCapabilities) {
            OccupantAwarenessStatus oasStatus;
            EXPECT_OK(mOccupantAwarenessService->getState(role, detectionCapability, &oasStatus));
        }
    }
}

// Test that getState() returns failure with invalid args.
TEST_P(OccupantAwarenessAidl, GetStateFailureTest) {
    // Verify that getState() returns error when role is invalid (0).
    OccupantAwarenessStatus oasStatus;
    EXPECT_FALSE(mOccupantAwarenessService
                         ->getState(Role::INVALID, IOccupantAwareness::CAP_PRESENCE_DETECTION,
                                    &oasStatus)
                         .isOk());

    // Verify that getState() returns error when role is invalid (invalid flag).
    int invalidRole = static_cast<int>(Role::ALL_OCCUPANTS) + 1;
    EXPECT_FALSE(mOccupantAwarenessService
                         ->getState(static_cast<Role>(invalidRole),
                                    IOccupantAwareness::CAP_PRESENCE_DETECTION, &oasStatus)
                         .isOk());

    // Verify that getState() returns error when capability is invalid (none).
    EXPECT_FALSE(mOccupantAwarenessService
                         ->getState(Role::FRONT_PASSENGER, IOccupantAwareness::CAP_NONE, &oasStatus)
                         .isOk());

    // Verify that getState() returns error when capability is invalid (invalid flag).
    int invalidDetectionFlags = 0x10;
    EXPECT_FALSE(mOccupantAwarenessService
                         ->getState(Role::FRONT_PASSENGER, invalidDetectionFlags, &oasStatus)
                         .isOk());
}

// Test that setCallback() returns within the timeout.
TEST_P(OccupantAwarenessAidl, SetCallbackTest) {
    sp<OccupantAwarenessCallback> callback =
            new OccupantAwarenessCallback([](int detectionFlags, OccupantAwarenessStatus status) {
                (void)detectionFlags;
                (void)status;
            });
    auto start = std::chrono::system_clock::now();
    Status status = mOccupantAwarenessService->setCallback(callback);
    auto elapsed = std::chrono::system_clock::now() - start;
    EXPECT_OK(status);
    ASSERT_LE(elapsed, kTimeout);
}

// Test that setCallback() returns failure with invalid args.
TEST_P(OccupantAwarenessAidl, SetCallbackFailureTest) {
    sp<OccupantAwarenessCallback> callback = nullptr;
    Status status = mOccupantAwarenessService->setCallback(callback);
    EXPECT_FALSE(status.isOk());
}

// Test that getLatestDetection() returns within the timeout.
TEST_P(OccupantAwarenessAidl, GetLatestDetectionTest) {
    auto start = std::chrono::system_clock::now();
    OccupantDetections detections;
    // Do not check status here, since error status is returned when no detection is present.
    (void)mOccupantAwarenessService->getLatestDetection(&detections);
    auto elapsed = std::chrono::system_clock::now() - start;
    ASSERT_LE(elapsed, kTimeout);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OccupantAwarenessAidl);
INSTANTIATE_TEST_SUITE_P(
        InstantiationName, OccupantAwarenessAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IOccupantAwareness::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
