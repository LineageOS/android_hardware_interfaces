/*
 * Copyright (C) 2021 The Android Open Source Project
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
 * See the License for the std::shared_ptrecific language governing permissions and
 * limitations under the License.
 */
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/uwb/BnUwbClientCallback.h>
#include <aidl/android/hardware/uwb/IUwb.h>
#include <aidl/android/hardware/uwb/IUwbChip.h>
#include <aidl/android/hardware/uwb/IUwbClientCallback.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <future>

using aidl::android::hardware::uwb::BnUwbClientCallback;
using aidl::android::hardware::uwb::IUwb;
using aidl::android::hardware::uwb::IUwbChip;
using aidl::android::hardware::uwb::IUwbClientCallback;
using aidl::android::hardware::uwb::UwbEvent;
using aidl::android::hardware::uwb::UwbStatus;
using android::ProcessState;
using android::String16;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

namespace {
constexpr static int kCallbackTimeoutMs = 250;
}  // namespace

class UwbClientCallback : public BnUwbClientCallback {
  public:
    UwbClientCallback(const std::function<void(const std::vector<uint8_t>&)>& on_uci_message_cb,
                      const std::function<void(UwbEvent, UwbStatus)>& on_hal_event_cb)
        : on_uci_message_cb_(on_uci_message_cb), on_hal_event_cb_(on_hal_event_cb) {}

    ScopedAStatus onUciMessage(const std::vector<uint8_t>& data) override {
        on_uci_message_cb_(data);
        return ScopedAStatus::ok();
    }

    ScopedAStatus onHalEvent(UwbEvent uwb_event, UwbStatus uwb_status) override {
        on_hal_event_cb_(uwb_event, uwb_status);
        return ScopedAStatus::ok();
    }

  private:
    std::function<void(const std::vector<uint8_t>&)> on_uci_message_cb_;
    std::function<void(UwbEvent, UwbStatus)> on_hal_event_cb_;
};

class UwbAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        iuwb_ = IUwb::fromBinder(SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(iuwb_, nullptr);
    }
    virtual void TearDown() override {
        // Trigger HAL close at end of each test.
        const auto iuwb_chip = getAnyChip();
        iuwb_chip->close();
    }
    std::shared_ptr<IUwb> iuwb_;

    // TODO (b/197638976): We pick the first chip here. Need to fix this
    // for supporting multiple chips in the future.
    std::string getAnyChipName() {
        std::vector<std::string> chip_names;
        ScopedAStatus status = iuwb_->getChips(&chip_names);
        EXPECT_TRUE(status.isOk());
        EXPECT_FALSE(chip_names.empty());
        return chip_names[0];
    }

    // TODO (b/197638976): We pick the first chip here. Need to fix this
    // for supporting multiple chips in the future.
    std::shared_ptr<IUwbChip> getAnyChip() {
        std::shared_ptr<IUwbChip> iuwb_chip;
        ScopedAStatus status = iuwb_->getChip(getAnyChipName(), &iuwb_chip);
        EXPECT_TRUE(status.isOk());
        EXPECT_NE(iuwb_chip, nullptr);
        return iuwb_chip;
    }

    std::shared_ptr<IUwbChip> getAnyChipAndOpen() {
        std::promise<void> open_cb_promise;
        std::future<void> open_cb_future{open_cb_promise.get_future()};
        std::shared_ptr<UwbClientCallback> callback = ndk::SharedRefBase::make<UwbClientCallback>(
                [](auto /* data */) {},
                [&open_cb_promise](auto event, auto /* status */) {
                    if (event == UwbEvent::OPEN_CPLT) {
                        open_cb_promise.set_value();
                    }
                });
        std::chrono::milliseconds timeout{kCallbackTimeoutMs};
        const auto iuwb_chip = getAnyChip();
        EXPECT_TRUE(iuwb_chip->open(callback).isOk());
        EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
        return iuwb_chip;
    }
};

TEST_P(UwbAidl, GetChips) {
    std::vector<std::string> chip_names;
    ScopedAStatus status = iuwb_->getChips(&chip_names);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(chip_names.empty());
}

TEST_P(UwbAidl, GetChip) {
    std::shared_ptr<IUwbChip> iuwb_chip;
    ScopedAStatus status = iuwb_->getChip(getAnyChipName(), &iuwb_chip);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(iuwb_chip, nullptr);
}

TEST_P(UwbAidl, ChipOpen) {
    std::promise<void> open_cb_promise;
    std::future<void> open_cb_future{open_cb_promise.get_future()};
    std::shared_ptr<UwbClientCallback> callback = ndk::SharedRefBase::make<UwbClientCallback>(
            [](auto /* data */) {},
            [&open_cb_promise](auto event, auto /* status */) {
                if (event == UwbEvent::OPEN_CPLT) {
                    open_cb_promise.set_value();
                }
            });
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    const auto iuwb_chip = getAnyChip();
    EXPECT_TRUE(iuwb_chip->open(callback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
}

TEST_P(UwbAidl, ChipClose) {
    std::promise<void> open_cb_promise;
    std::future<void> open_cb_future{open_cb_promise.get_future()};
    std::promise<void> close_cb_promise;
    std::future<void> close_cb_future{close_cb_promise.get_future()};
    std::shared_ptr<UwbClientCallback> callback = ndk::SharedRefBase::make<UwbClientCallback>(
            [](auto /* data */) {},
            [&open_cb_promise, &close_cb_promise](auto event, auto /* status */) {
                if (event == UwbEvent::OPEN_CPLT) {
                    open_cb_promise.set_value();
                }
                if (event == UwbEvent::CLOSE_CPLT) {
                    close_cb_promise.set_value();
                }
            });
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    const auto iuwb_chip = getAnyChip();
    EXPECT_TRUE(iuwb_chip->open(callback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
    EXPECT_TRUE(iuwb_chip->close().isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

TEST_P(UwbAidl, ChipCoreInit) {
    const auto iuwb_chip = getAnyChipAndOpen();
    EXPECT_TRUE(iuwb_chip->coreInit().isOk());
}

TEST_P(UwbAidl, ChipSessionInit) {
    const auto iuwb_chip = getAnyChipAndOpen();
    EXPECT_TRUE(iuwb_chip->sessionInit(0).isOk());
}

TEST_P(UwbAidl, ChipGetSupportedAndroidUciVersion) {
    const auto iuwb_chip = getAnyChipAndOpen();
    EXPECT_TRUE(iuwb_chip->coreInit().isOk());

    int32_t version;
    EXPECT_TRUE(iuwb_chip->getSupportedAndroidUciVersion(&version).isOk());
    EXPECT_GT(version, 0);
}

TEST_P(UwbAidl, ChipGetName) {
    std::string chip_name = getAnyChipName();
    std::shared_ptr<IUwbChip> iuwb_chip;
    ScopedAStatus status = iuwb_->getChip(chip_name, &iuwb_chip);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(iuwb_chip, nullptr);

    std::string retrieved_chip_name;
    status = iuwb_chip->getName(&retrieved_chip_name);
    EXPECT_TRUE(status.isOk());
    EXPECT_EQ(retrieved_chip_name, chip_name);
}

/**
TEST_P(UwbAidl, ChipSendUciMessage_GetDeviceInfo) {
const auto iuwb_chip = getAnyChipAndOpen(callback);
EXPECT_TRUE(iuwb_chip->coreInit(callback).isOk());

const std::vector<uint8_t>
EXPECT_TRUE(iuwb_chip->sendUciMessage().isOk());
} */

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(UwbAidl);
INSTANTIATE_TEST_SUITE_P(Uwb, UwbAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IUwb::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    // UWB HAL only allows 1 client, make sure framework
    // does not have UWB HAL open before running
    std::system("/system/bin/cmd uwb disable-uwb");
    sleep(3);
    auto status = RUN_ALL_TESTS();
    sleep(3);
    std::system("/system/bin/cmd uwb enable-uwb");
    return status;
}
