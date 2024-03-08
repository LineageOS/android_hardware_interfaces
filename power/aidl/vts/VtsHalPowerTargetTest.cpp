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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/power/BnPower.h>
#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>

#include <unistd.h>

namespace aidl::android::hardware::power {
namespace {

using android::hardware::power::Boost;
using android::hardware::power::IPower;
using android::hardware::power::IPowerHintSession;
using android::hardware::power::Mode;
using android::hardware::power::SessionHint;
using android::hardware::power::SessionMode;
using android::hardware::power::WorkDuration;

const std::vector<Boost> kBoosts{ndk::enum_range<Boost>().begin(), ndk::enum_range<Boost>().end()};

const std::vector<Mode> kModes{ndk::enum_range<Mode>().begin(), ndk::enum_range<Mode>().end()};

const std::vector<SessionHint> kSessionHints{ndk::enum_range<SessionHint>().begin(),
                                             ndk::enum_range<SessionHint>().end()};

const std::vector<SessionMode> kSessionModes{ndk::enum_range<SessionMode>().begin(),
                                             ndk::enum_range<SessionMode>().end()};

const std::vector<Boost> kInvalidBoosts = {
        static_cast<Boost>(static_cast<int32_t>(kBoosts.front()) - 1),
        static_cast<Boost>(static_cast<int32_t>(kBoosts.back()) + 1),
};

const std::vector<Mode> kInvalidModes = {
        static_cast<Mode>(static_cast<int32_t>(kModes.front()) - 1),
        static_cast<Mode>(static_cast<int32_t>(kModes.back()) + 1),
};

const std::vector<SessionHint> kInvalidSessionHints = {
        static_cast<SessionHint>(static_cast<int32_t>(kSessionHints.front()) - 1),
        static_cast<SessionHint>(static_cast<int32_t>(kSessionHints.back()) + 1),
};

const std::vector<SessionMode> kInvalidSessionModes = {
        static_cast<SessionMode>(static_cast<int32_t>(kSessionModes.front()) - 1),
        static_cast<SessionMode>(static_cast<int32_t>(kSessionModes.back()) + 1),
};

class DurationWrapper : public WorkDuration {
  public:
    DurationWrapper(int64_t dur, int64_t time) {
        durationNanos = dur;
        timeStampNanos = time;
    }
};

const std::vector<int32_t> kSelfTids = {
        gettid(),
};

const std::vector<int32_t> kEmptyTids = {};

const std::vector<WorkDuration> kNoDurations = {};

const std::vector<WorkDuration> kDurationsWithZero = {
        DurationWrapper(1000L, 1L),
        DurationWrapper(0L, 2L),
};

const std::vector<WorkDuration> kDurationsWithNegative = {
        DurationWrapper(1000L, 1L),
        DurationWrapper(-1000L, 2L),
};

const std::vector<WorkDuration> kDurations = {
        DurationWrapper(1L, 1L),
        DurationWrapper(1000L, 2L),
        DurationWrapper(1000000L, 3L),
        DurationWrapper(1000000000L, 4L),
};

class PowerAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        power = IPower::fromBinder(ndk::SpAIBinder(binder));
        auto status = power->getInterfaceVersion(&mServiceVersion);
        ASSERT_TRUE(status.isOk());
    }

    std::shared_ptr<IPower> power;
    int32_t mServiceVersion;
};

class HintSessionAidl : public PowerAidl {
  public:
    virtual void SetUp() override {
        PowerAidl::SetUp();
        if (mServiceVersion < 2) {
            GTEST_SKIP() << "DEVICE not launching with Power V2 and beyond.";
        }

        auto status = power->createHintSession(getpid(), getuid(), kSelfTids, 16666666L, &mSession);
        ASSERT_TRUE(status.isOk());
        ASSERT_NE(nullptr, mSession);
    }
    std::shared_ptr<IPowerHintSession> mSession;
};

TEST_P(PowerAidl, setMode) {
    for (const auto& mode : kModes) {
        ASSERT_TRUE(power->setMode(mode, true).isOk());
        ASSERT_TRUE(power->setMode(mode, false).isOk());
    }
    for (const auto& mode : kInvalidModes) {
        ASSERT_TRUE(power->setMode(mode, true).isOk());
        ASSERT_TRUE(power->setMode(mode, false).isOk());
    }
}

TEST_P(PowerAidl, isModeSupported) {
    for (const auto& mode : kModes) {
        bool supported;
        ASSERT_TRUE(power->isModeSupported(mode, &supported).isOk());
    }
    for (const auto& mode : kInvalidModes) {
        bool supported;
        ASSERT_TRUE(power->isModeSupported(mode, &supported).isOk());
        // Should return false for values outside enum
        ASSERT_FALSE(supported);
    }
}

TEST_P(PowerAidl, setBoost) {
    for (const auto& boost : kBoosts) {
        ASSERT_TRUE(power->setBoost(boost, 0).isOk());
        ASSERT_TRUE(power->setBoost(boost, 1000).isOk());
        ASSERT_TRUE(power->setBoost(boost, -1).isOk());
    }
    for (const auto& boost : kInvalidBoosts) {
        ASSERT_TRUE(power->setBoost(boost, 0).isOk());
        ASSERT_TRUE(power->setBoost(boost, 1000).isOk());
        ASSERT_TRUE(power->setBoost(boost, -1).isOk());
    }
}

TEST_P(PowerAidl, isBoostSupported) {
    for (const auto& boost : kBoosts) {
        bool supported;
        ASSERT_TRUE(power->isBoostSupported(boost, &supported).isOk());
    }
    for (const auto& boost : kInvalidBoosts) {
        bool supported;
        ASSERT_TRUE(power->isBoostSupported(boost, &supported).isOk());
        // Should return false for values outside enum
        ASSERT_FALSE(supported);
    }
}

TEST_P(PowerAidl, getHintSessionPreferredRate) {
    if (mServiceVersion < 2) {
        GTEST_SKIP() << "DEVICE not launching with Power V2 and beyond.";
    }

    int64_t rate = -1;
    ASSERT_TRUE(power->getHintSessionPreferredRate(&rate).isOk());
    // At least 1ms rate limit from HAL
    ASSERT_GE(rate, 1000000);
}

TEST_P(HintSessionAidl, createAndCloseHintSession) {
    ASSERT_TRUE(mSession->pause().isOk());
    ASSERT_TRUE(mSession->resume().isOk());
    // Test normal destroy operation
    ASSERT_TRUE(mSession->close().isOk());
    mSession.reset();
}

TEST_P(HintSessionAidl, createHintSessionFailed) {
    std::shared_ptr<IPowerHintSession> session;
    auto status = power->createHintSession(getpid(), getuid(), kEmptyTids, 16666666L, &session);

    // Regardless of whether V2 and beyond is supported, the status is always not STATUS_OK.
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());
}

TEST_P(HintSessionAidl, updateAndReportDurations) {
    ASSERT_TRUE(mSession->updateTargetWorkDuration(16666667LL).isOk());
    ASSERT_TRUE(mSession->reportActualWorkDuration(kDurations).isOk());
}

TEST_P(HintSessionAidl, sendSessionHint) {
    if (mServiceVersion < 4) {
        GTEST_SKIP() << "DEVICE not launching with Power V4 and beyond.";
    }

    for (const auto& sessionHint : kSessionHints) {
        ASSERT_TRUE(mSession->sendHint(sessionHint).isOk());
    }
    for (const auto& sessionHint : kInvalidSessionHints) {
        ASSERT_TRUE(mSession->sendHint(sessionHint).isOk());
    }
}

TEST_P(HintSessionAidl, setThreads) {
    if (mServiceVersion < 4) {
        GTEST_SKIP() << "DEVICE not launching with Power V4 and beyond.";
    }

    auto status = mSession->setThreads(kEmptyTids);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());

    ASSERT_TRUE(mSession->setThreads(kSelfTids).isOk());
}

TEST_P(HintSessionAidl, setSessionMode) {
    if (mServiceVersion < 5) {
        GTEST_SKIP() << "DEVICE not launching with Power V5 and beyond.";
    }

    for (const auto& sessionMode : kSessionModes) {
        ASSERT_TRUE(mSession->setMode(sessionMode, true).isOk());
        ASSERT_TRUE(mSession->setMode(sessionMode, false).isOk());
    }
    for (const auto& sessionMode : kInvalidSessionModes) {
        ASSERT_TRUE(mSession->setMode(sessionMode, true).isOk());
        ASSERT_TRUE(mSession->setMode(sessionMode, false).isOk());
    }
}

// FIXED_PERFORMANCE mode is required for all devices which ship on Android 11
// or later
TEST_P(PowerAidl, hasFixedPerformance) {
    bool supported;
    ASSERT_TRUE(power->isModeSupported(Mode::FIXED_PERFORMANCE, &supported).isOk());
    ASSERT_TRUE(supported);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerAidl);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HintSessionAidl);

INSTANTIATE_TEST_SUITE_P(Power, PowerAidl,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(IPower::descriptor)),
                         ::android::PrintInstanceNameToString);
INSTANTIATE_TEST_SUITE_P(Power, HintSessionAidl,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(IPower::descriptor)),
                         ::android::PrintInstanceNameToString);

}  // namespace
}  // namespace aidl::android::hardware::power

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
