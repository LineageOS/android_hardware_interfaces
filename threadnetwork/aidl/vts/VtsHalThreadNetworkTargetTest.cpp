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

#define LOG_TAG "ThreadNetworkHalTargetTest"

#include <future>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <log/log.h>

#include <aidl/android/hardware/threadnetwork/BnThreadChipCallback.h>
#include <aidl/android/hardware/threadnetwork/IThreadChip.h>

using aidl::android::hardware::threadnetwork::BnThreadChipCallback;
using aidl::android::hardware::threadnetwork::IThreadChip;
using android::ProcessState;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

namespace {
constexpr static int kCallbackTimeoutMs = 5000;
}  // namespace

class ThreadChipCallback : public BnThreadChipCallback {
  public:
    ThreadChipCallback(const std::function<void(const std::vector<uint8_t>&)>& on_spinel_message_cb)
        : on_spinel_message_cb_(on_spinel_message_cb) {}

    ScopedAStatus onReceiveSpinelFrame(const std::vector<uint8_t>& in_aFrame) {
        on_spinel_message_cb_(in_aFrame);
        return ScopedAStatus::ok();
    }

  private:
    std::function<void(const std::vector<uint8_t>&)> on_spinel_message_cb_;
};

class ThreadNetworkAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        std::string serviceName = GetParam();

        ALOGI("serviceName: %s", serviceName.c_str());

        thread_chip = IThreadChip::fromBinder(
                SpAIBinder(AServiceManager_waitForService(serviceName.c_str())));
        ASSERT_NE(thread_chip, nullptr);
    }

    virtual void TearDown() override { thread_chip->close(); }

    std::shared_ptr<IThreadChip> thread_chip;
};

TEST_P(ThreadNetworkAidl, Open) {
    std::shared_ptr<ThreadChipCallback> callback =
            ndk::SharedRefBase::make<ThreadChipCallback>([](auto /* data */) {});

    EXPECT_TRUE(thread_chip->open(callback).isOk());
}

TEST_P(ThreadNetworkAidl, Close) {
    std::shared_ptr<ThreadChipCallback> callback =
            ndk::SharedRefBase::make<ThreadChipCallback>([](auto /* data */) {});

    EXPECT_TRUE(thread_chip->open(callback).isOk());
    EXPECT_TRUE(thread_chip->close().isOk());
}

TEST_P(ThreadNetworkAidl, Reset) {
    std::shared_ptr<ThreadChipCallback> callback =
            ndk::SharedRefBase::make<ThreadChipCallback>([](auto /* data */) {});

    EXPECT_TRUE(thread_chip->open(callback).isOk());
    EXPECT_TRUE(thread_chip->hardwareReset().isOk());
}

TEST_P(ThreadNetworkAidl, SendSpinelFrame) {
    const uint8_t kCmdOffset = 2;
    const uint8_t kMajorVersionOffset = 3;
    const uint8_t kMinorVersionOffset = 4;
    const std::vector<uint8_t> kGetSpinelProtocolVersion({0x81, 0x02, 0x01});
    const std::vector<uint8_t> kGetSpinelProtocolVersionResponse({0x81, 0x06, 0x01, 0x04, 0x03});
    uint8_t min_major_version = kGetSpinelProtocolVersionResponse[kMajorVersionOffset];
    uint8_t min_minor_version = kGetSpinelProtocolVersionResponse[kMinorVersionOffset];
    uint8_t major_version;
    uint8_t minor_version;
    std::promise<void> open_cb_promise;
    std::future<void> open_cb_future{open_cb_promise.get_future()};
    std::shared_ptr<ThreadChipCallback> callback;
    std::vector<uint8_t> received_frame;
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};

    callback = ndk::SharedRefBase::make<ThreadChipCallback>(
            [&](const std::vector<uint8_t>& in_aFrame) {
                if (in_aFrame.size() == kGetSpinelProtocolVersionResponse.size() &&
                    in_aFrame[kCmdOffset] == kGetSpinelProtocolVersionResponse[kCmdOffset]) {
                    major_version = in_aFrame[kMajorVersionOffset];
                    minor_version = in_aFrame[kMinorVersionOffset];
                    open_cb_promise.set_value();
                }
            });

    ASSERT_NE(callback, nullptr);

    EXPECT_TRUE(thread_chip->open(callback).isOk());

    EXPECT_TRUE(thread_chip->sendSpinelFrame(kGetSpinelProtocolVersion).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    EXPECT_GE(major_version, min_major_version);
    if (major_version == min_major_version) {
        EXPECT_GE(minor_version, min_minor_version);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ThreadNetworkAidl);
INSTANTIATE_TEST_SUITE_P(
        Thread, ThreadNetworkAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IThreadChip::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
