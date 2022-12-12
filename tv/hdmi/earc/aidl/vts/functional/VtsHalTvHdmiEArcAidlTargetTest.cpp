/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "EArc_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tv/hdmi/earc/BnEArcCallback.h>
#include <aidl/android/hardware/tv/hdmi/earc/IEArc.h>
#include <aidl/android/hardware/tv/hdmi/earc/IEArcStatus.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <sstream>
#include <vector>

using ::aidl::android::hardware::tv::hdmi::earc::BnEArcCallback;
using ::aidl::android::hardware::tv::hdmi::earc::IEArc;
using ::aidl::android::hardware::tv::hdmi::earc::IEArcCallback;
using ::aidl::android::hardware::tv::hdmi::earc::IEArcStatus;
using ::ndk::SpAIBinder;

// The main test class for TV EARC HAL.
class EArcTest : public ::testing::TestWithParam<std::string> {
    static void serviceDied(void* /* cookie */) { ALOGE("VtsHalTvCecAidlTargetTest died"); }

  public:
    void SetUp() override {
        eArc = IEArc::fromBinder(SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(eArc, nullptr);
        ALOGI("%s: getService() for eArc is %s", __func__, eArc->isRemote() ? "remote" : "local");

        eArcCallback = ::ndk::SharedRefBase::make<EArcCallback>();
        ASSERT_NE(eArcCallback, nullptr);
        eArcDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        ASSERT_EQ(AIBinder_linkToDeath(eArc->asBinder().get(), eArcDeathRecipient.get(), 0),
                  STATUS_OK);
    }

    class EArcCallback : public BnEArcCallback {
      public:
        ::ndk::ScopedAStatus onStateChange(IEArcStatus connected __unused,
                                           int32_t portId __unused) {
            return ::ndk::ScopedAStatus::ok();
        };
        ::ndk::ScopedAStatus onCapabilitiesReported(
                const std::vector<uint8_t>& capabilities __unused, int32_t portId __unused) {
            return ::ndk::ScopedAStatus::ok();
        };
    };

    std::shared_ptr<IEArc> eArc;
    std::shared_ptr<IEArcCallback> eArcCallback;
    ::ndk::ScopedAIBinder_DeathRecipient eArcDeathRecipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EArcTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, EArcTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IEArc::descriptor)),
                         android::PrintInstanceNameToString);

TEST_P(EArcTest, setGetEArcEnabled) {
    bool initial_state;
    bool changed_state;
    ASSERT_TRUE(eArc->isEArcEnabled(&initial_state).isOk());
    ASSERT_TRUE(eArc->setEArcEnabled(!initial_state).isOk());
    ASSERT_TRUE(eArc->isEArcEnabled(&changed_state).isOk());
    ASSERT_TRUE(initial_state != changed_state);
    ASSERT_TRUE(eArc->setEArcEnabled(initial_state).isOk());
}

TEST_P(EArcTest, SetCallback) {
    ASSERT_TRUE(eArc->setCallback(eArcCallback).isOk());
}

TEST_P(EArcTest, GetState) {
    IEArcStatus connectionStatus;
    ASSERT_TRUE(eArc->getState(1, &connectionStatus).isOk());
}

TEST_P(EArcTest, GetLastReportedAudioCapabilities) {
    std::vector<uint8_t> capabilities;
    ASSERT_TRUE(eArc->getLastReportedAudioCapabilities(1, &capabilities).isOk());
}
