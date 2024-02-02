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

#define LOG_TAG "VtsHalAutomotiveRemoteAccess"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/automotive/remoteaccess/ApState.h>
#include <aidl/android/hardware/automotive/remoteaccess/BnRemoteTaskCallback.h>
#include <aidl/android/hardware/automotive/remoteaccess/IRemoteAccess.h>
#include <aidl/android/hardware/automotive/remoteaccess/ScheduleInfo.h>
#include <aidl/android/hardware/automotive/remoteaccess/TaskType.h>
#include <android-base/thread_annotations.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using ::aidl::android::hardware::automotive::remoteaccess::ApState;
using ::aidl::android::hardware::automotive::remoteaccess::BnRemoteTaskCallback;
using ::aidl::android::hardware::automotive::remoteaccess::IRemoteAccess;
using ::aidl::android::hardware::automotive::remoteaccess::ScheduleInfo;
using ::aidl::android::hardware::automotive::remoteaccess::TaskType;
using ::android::getAidlHalInstanceNames;
using ::android::PrintInstanceNameToString;
using ::android::base::ScopedLockAssertion;
using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;
using ::ndk::SpAIBinder;
using ::testing::UnorderedElementsAre;

namespace {

const std::string TEST_CLIENT_ID = "TEST CLIENT ID";
const std::string TEST_SCHEDULE_ID = "TEST SCHEDULE ID";
const std::string TEST_SCHEDULE_ID_1 = "TEST SCHEDULE ID 1";
const std::string TEST_SCHEDULE_ID_2 = "TEST SCHEDULE ID 2";
const std::vector<uint8_t> TEST_TASK_DATA =
        std::vector<uint8_t>({static_cast<uint8_t>(0xde), static_cast<uint8_t>(0xad),
                              static_cast<uint8_t>(0xbe), static_cast<uint8_t>(0xef)});
const int32_t JOB_DELAY_IN_SECONDS = 5;

}  // namespace

class VtsHalAutomotiveRemoteAccessTargetTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        const std::string& name = GetParam();
        mHal = IRemoteAccess::fromBinder(SpAIBinder(AServiceManager_waitForService(name.c_str())));
        ASSERT_NE(mHal, nullptr) << "Failed to connect to remote access HAL: " << name;
    }

    virtual void TearDown() override {
        if (mHal != nullptr) {
            mHal->clearRemoteTaskCallback();
            mHal->unscheduleAllTasks(TEST_CLIENT_ID);
        }
    }

  protected:
    std::shared_ptr<IRemoteAccess> mHal;

    bool isTaskScheduleSupported();
    int32_t getInterfaceVersion();
    ScheduleInfo getTestScheduleInfo(int32_t delayInSeconds, int32_t count,
                                     int32_t periodicInSeconds);
    void setTaskCallbackAndReadyForTask(std::shared_ptr<BnRemoteTaskCallback> testCallback);
};

class TestRemoteTaskCallback final : public BnRemoteTaskCallback {
  public:
    ScopedAStatus onRemoteTaskRequested(const std::string& clientId,
                                        const std::vector<uint8_t>& data) override {
        {
            std::unique_lock<std::mutex> lockGuard(mLock);
            mClientIds.push_back(clientId);
            mDataList.push_back(data);
        }
        mCv.notify_one();
        return ScopedAStatus::ok();
    }

    const std::vector<std::string> getCalledClientIds() {
        std::lock_guard<std::mutex> lockGuard(mLock);
        return mClientIds;
    }

    const std::vector<std::vector<uint8_t>> getCalledDataList() {
        std::lock_guard<std::mutex> lockGuard(mLock);
        return mDataList;
    }

    bool waitForCallbacks(size_t count, int32_t timeoutInSeconds) {
        std::unique_lock lk(mLock);
        return mCv.wait_for(lk, std::chrono::seconds(timeoutInSeconds), [this, count] {
            ScopedLockAssertion lockAssertion(mLock);
            return mClientIds.size() >= count;
        });
    }

  private:
    std::mutex mLock;
    std::vector<std::string> mClientIds GUARDED_BY(mLock);
    std::vector<std::vector<uint8_t>> mDataList GUARDED_BY(mLock);
    std::condition_variable mCv;
};

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testGetVehicleId) {
    std::string vehicleId;

    ScopedAStatus status = mHal->getVehicleId(&vehicleId);

    ASSERT_TRUE(status.isOk()) << "Failed to call getVehicleId";
    ASSERT_FALSE(vehicleId.empty()) << "Vehicle ID must not be empty";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testGetWakeupServiceName) {
    std::string wakeupServiceName;

    ScopedAStatus status = mHal->getWakeupServiceName(&wakeupServiceName);

    ASSERT_TRUE(status.isOk()) << "Failed to call getWakeupServiceName";
    ASSERT_FALSE(wakeupServiceName.empty()) << "Wakeup service name must not be empty";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testGetProcessorId) {
    std::string processorId;

    ScopedAStatus status = mHal->getProcessorId(&processorId);

    ASSERT_TRUE(status.isOk()) << "Failed to call getProcessorId";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testSetClearRemoteTaskCallback) {
    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();

    ScopedAStatus status = mHal->setRemoteTaskCallback(testCallback);

    ASSERT_TRUE(status.isOk()) << "Failed to call setRemoteTaskCallback";

    status = mHal->clearRemoteTaskCallback();

    ASSERT_TRUE(status.isOk()) << "Failed to call clearRemoteTaskCallback";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testNotifyApStateChange) {
    ApState apState = ApState{
            .isReadyForRemoteTask = false,
            .isWakeupRequired = false,
    };

    ScopedAStatus status = mHal->notifyApStateChange(apState);

    ASSERT_TRUE(status.isOk()) << "Failed to call notifyApStateChange with state: "
                               << apState.toString();

    apState = ApState{
            .isReadyForRemoteTask = true,
            .isWakeupRequired = false,
    };

    ASSERT_TRUE(status.isOk()) << "Failed to call notifyApStateChange with state: "
                               << apState.toString();
}

int32_t VtsHalAutomotiveRemoteAccessTargetTest::getInterfaceVersion() {
    int32_t interfaceVersion = 0;
    mHal->getInterfaceVersion(&interfaceVersion);
    return interfaceVersion;
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testIsTaskScheduleSupported) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    bool supported;

    ScopedAStatus status = mHal->isTaskScheduleSupported(&supported);

    ASSERT_TRUE(status.isOk()) << "Failed to call isTaskScheduleSupported";
}

bool VtsHalAutomotiveRemoteAccessTargetTest::isTaskScheduleSupported() {
    bool supported = false;
    mHal->isTaskScheduleSupported(&supported);
    return supported;
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testGetSupportedTaskTypesForScheduling) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::vector<TaskType> supportedTaskTypes;

    ScopedAStatus status = mHal->getSupportedTaskTypesForScheduling(&supportedTaskTypes);

    ASSERT_TRUE(status.isOk()) << "Failed to call getSupportedTaskTypesForScheduling";

    if (!isTaskScheduleSupported()) {
        ASSERT_TRUE(supportedTaskTypes.empty())
                << "getSupportedTaskTypesForScheduling must return empty array "
                << "if isTaskScheduleSupported is false";
        return;
    }

    ASSERT_TRUE(std::find(supportedTaskTypes.begin(), supportedTaskTypes.end(), TaskType::CUSTOM) !=
                supportedTaskTypes.end())
            << "getSupportedTaskTypesForScheduling must contain TaskType::CUSTOM";
}

ScheduleInfo VtsHalAutomotiveRemoteAccessTargetTest::getTestScheduleInfo(
        int32_t delayInSeconds, int32_t count, int32_t periodicInSeconds) {
    auto nowInEpochSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();

    return ScheduleInfo{
            .clientId = TEST_CLIENT_ID,
            .scheduleId = TEST_SCHEDULE_ID,
            .taskType = TaskType::CUSTOM,
            .taskData = TEST_TASK_DATA,
            .count = count,
            .startTimeInEpochSeconds = nowInEpochSeconds + delayInSeconds,
            .periodicInSeconds = periodicInSeconds,
    };
}

void VtsHalAutomotiveRemoteAccessTargetTest::setTaskCallbackAndReadyForTask(
        std::shared_ptr<BnRemoteTaskCallback> testCallback) {
    mHal->setRemoteTaskCallback(testCallback);
    // Notify isReadForRemoteTask to be true.
    mHal->notifyApStateChange(ApState{
            .isReadyForRemoteTask = true,
            .isWakeupRequired = false,
    });
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testScheduleTask) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();
    setTaskCallbackAndReadyForTask(testCallback);

    int32_t count = 2;
    ScheduleInfo scheduleInfo = getTestScheduleInfo(
            /*delayInSeconds=*/JOB_DELAY_IN_SECONDS, count, /*periodicInSeconds=*/1);
    ScopedAStatus status = mHal->scheduleTask(scheduleInfo);

    if (!isTaskScheduleSupported()) {
        ASSERT_FALSE(status.isOk()) << "scheduleTask must return EX_ILLEGAL_ARGUMENT "
                                    << "if isTaskScheduleSupported is false";
        ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT)
                << "scheduleTask must return EX_ILLEGAL_ARGUMENT "
                << "if isTaskScheduleSupported is false";
        return;
    }

    ASSERT_TRUE(status.isOk()) << "Failed to call scheduleTask with scheduleInfo: "
                               << scheduleInfo.toString();

    int32_t timeoutInSeconds = JOB_DELAY_IN_SECONDS + 5;
    bool gotCallbacks = testCallback->waitForCallbacks(count, timeoutInSeconds);
    // unschedule the task before checking the result.
    mHal->unscheduleTask(TEST_CLIENT_ID, TEST_SCHEDULE_ID);

    ASSERT_TRUE(gotCallbacks) << "Callbacks is not called enough times before timeout: "
                              << timeoutInSeconds << "s";
    std::vector<std::vector<uint8_t>> dataList = testCallback->getCalledDataList();
    std::vector<std::string> clientIds = testCallback->getCalledClientIds();

    for (size_t i = 0; i < dataList.size(); i++) {
        EXPECT_EQ(dataList[i], TEST_TASK_DATA) << "Must receive expected task data";
        EXPECT_EQ(clientIds[i], TEST_CLIENT_ID) << "Must receive expected client id";
    }
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testUnscheduleTask) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();
    setTaskCallbackAndReadyForTask(testCallback);

    ScheduleInfo scheduleInfo = getTestScheduleInfo(
            /*delayInSeconds=*/JOB_DELAY_IN_SECONDS, /*count=*/1, /*periodicInSeconds=*/0);
    mHal->scheduleTask(scheduleInfo);
    ScopedAStatus status = mHal->unscheduleTask(TEST_CLIENT_ID, TEST_SCHEDULE_ID);

    ASSERT_TRUE(status.isOk()) << "Failed to call unscheduleTask";

    // If not cancelled, should be called in 5s, wait for 6s and make sure no task arrives.
    std::this_thread::sleep_for(std::chrono::seconds(JOB_DELAY_IN_SECONDS + 1));

    ASSERT_TRUE(testCallback->getCalledClientIds().empty())
            << "Remote task callback must not be called if the task is cancelled";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testUnscheduleAllTasks) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();
    setTaskCallbackAndReadyForTask(testCallback);

    ScheduleInfo scheduleInfo = getTestScheduleInfo(
            /*delayInSeconds=*/JOB_DELAY_IN_SECONDS, /*count=*/1, /*periodicInSeconds=*/0);
    mHal->scheduleTask(scheduleInfo);
    ScopedAStatus status = mHal->unscheduleAllTasks(TEST_CLIENT_ID);

    ASSERT_TRUE(status.isOk()) << "Failed to call unscheduleAllTasks";

    // If not cancelled, should be called in 5s, wait for 6s and make sure no task arrives.
    std::this_thread::sleep_for(std::chrono::seconds(JOB_DELAY_IN_SECONDS + 1));

    ASSERT_TRUE(testCallback->getCalledClientIds().empty())
            << "Remote task callback must not be called if the task is cancelled";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testIsTaskScheduled) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();
    setTaskCallbackAndReadyForTask(testCallback);

    ScheduleInfo scheduleInfo = getTestScheduleInfo(
            /*delayInSeconds=*/JOB_DELAY_IN_SECONDS, /*count=*/1, /*periodicInSeconds=*/0);
    mHal->scheduleTask(scheduleInfo);

    bool scheduled;
    ScopedAStatus status = mHal->isTaskScheduled(TEST_CLIENT_ID, TEST_SCHEDULE_ID, &scheduled);

    ASSERT_TRUE(status.isOk()) << "Failed to call unscheduleTask";

    if (!isTaskScheduleSupported()) {
        ASSERT_FALSE(scheduled) << "isTaskScheduled must return false "
                                << "if isTaskScheduleSupported is false";
        return;
    }

    ASSERT_TRUE(scheduled) << "isTaskScheduled must return true if the task is scheduled";

    mHal->unscheduleAllTasks(TEST_CLIENT_ID);
    status = mHal->isTaskScheduled(TEST_CLIENT_ID, TEST_SCHEDULE_ID, &scheduled);

    ASSERT_TRUE(status.isOk()) << "Failed to call unscheduleTask";
    ASSERT_FALSE(scheduled) << "isTaskScheduled must return false if the task is not scheduled";
}

TEST_P(VtsHalAutomotiveRemoteAccessTargetTest, testGetAllPendingScheduledTasks) {
    if (getInterfaceVersion() < 2) {
        GTEST_SKIP() << "Require RemoteAccess HAL v2";
    }

    std::shared_ptr<TestRemoteTaskCallback> testCallback =
            SharedRefBase::make<TestRemoteTaskCallback>();
    setTaskCallbackAndReadyForTask(testCallback);

    auto nowInEpochSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count();

    ScheduleInfo scheduleInfo1 = ScheduleInfo{
            .clientId = TEST_CLIENT_ID,
            .scheduleId = TEST_SCHEDULE_ID_1,
            .taskType = TaskType::CUSTOM,
            .taskData = TEST_TASK_DATA,
            .count = 1,
            .startTimeInEpochSeconds = nowInEpochSeconds + 5,
            .periodicInSeconds = 0,
    };
    ScheduleInfo scheduleInfo2 = ScheduleInfo{
            .clientId = TEST_CLIENT_ID,
            .scheduleId = TEST_SCHEDULE_ID_2,
            .taskType = TaskType::CUSTOM,
            .taskData = TEST_TASK_DATA,
            .count = 10,
            .startTimeInEpochSeconds = nowInEpochSeconds + 10,
            .periodicInSeconds = 1,
    };
    mHal->scheduleTask(scheduleInfo1);
    mHal->scheduleTask(scheduleInfo2);

    std::vector<ScheduleInfo> outScheduleInfo;
    ScopedAStatus status = mHal->getAllPendingScheduledTasks(TEST_CLIENT_ID, &outScheduleInfo);

    ASSERT_TRUE(status.isOk()) << "Failed to call getAllPendingScheduledTasks";

    if (!isTaskScheduleSupported()) {
        ASSERT_TRUE(outScheduleInfo.empty())
                << "Must return empty array for getAllPendingScheduledTasks "
                << "if isTaskScheduleSupported is false";
        return;
    }

    ASSERT_THAT(outScheduleInfo, UnorderedElementsAre(scheduleInfo1, scheduleInfo2))
            << "expected all pending schedule info mismatch";

    mHal->unscheduleTask(TEST_CLIENT_ID, TEST_SCHEDULE_ID_1);

    status = mHal->getAllPendingScheduledTasks(TEST_CLIENT_ID, &outScheduleInfo);

    ASSERT_TRUE(status.isOk()) << "Failed to call getAllPendingScheduledTasks";

    ASSERT_THAT(outScheduleInfo, UnorderedElementsAre(scheduleInfo2))
            << "expected all pending schedule info mismatch";
}

// It is possible that no remote access HAL is registered.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VtsHalAutomotiveRemoteAccessTargetTest);

INSTANTIATE_TEST_SUITE_P(PerInstance, VtsHalAutomotiveRemoteAccessTargetTest,
                         testing::ValuesIn(getAidlHalInstanceNames(IRemoteAccess::descriptor)),
                         PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Starts a process pool for callbacks.
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
