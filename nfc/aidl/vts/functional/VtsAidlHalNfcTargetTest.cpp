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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "nfc_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/nfc/BnNfc.h>
#include <aidl/android/hardware/nfc/BnNfcClientCallback.h>
#include <aidl/android/hardware/nfc/INfc.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <chrono>
#include <future>

using aidl::android::hardware::nfc::INfc;
using aidl::android::hardware::nfc::INfcClientCallback;
using aidl::android::hardware::nfc::NfcCloseType;
using aidl::android::hardware::nfc::NfcConfig;
using aidl::android::hardware::nfc::NfcEvent;
using aidl::android::hardware::nfc::NfcStatus;
using aidl::android::hardware::nfc::PresenceCheckAlgorithm;

using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using android::base::StringPrintf;
using ndk::enum_range;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;

constexpr static int kCallbackTimeoutMs = 10000;

// 261 bytes is the default and minimum transceive length
constexpr unsigned int MIN_ISO_DEP_TRANSCEIVE_LENGTH = 261;

// Range of valid off host route ids
constexpr uint8_t MIN_OFFHOST_ROUTE_ID = 0x01;
constexpr uint8_t MAX_OFFHOST_ROUTE_ID = 0xFE;

class NfcClientCallback : public aidl::android::hardware::nfc::BnNfcClientCallback {
  public:
    NfcClientCallback(const std::function<void(NfcEvent, NfcStatus)>& on_hal_event_cb,
                      const std::function<void(const std::vector<uint8_t>&)>& on_nci_data_cb)
        : on_nci_data_cb_(on_nci_data_cb), on_hal_event_cb_(on_hal_event_cb) {}
    virtual ~NfcClientCallback() = default;

    ::ndk::ScopedAStatus sendEvent(NfcEvent event, NfcStatus event_status) override {
        on_hal_event_cb_(event, event_status);
        return ::ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus sendData(const std::vector<uint8_t>& data) override {
        on_nci_data_cb_(data);
        return ::ndk::ScopedAStatus::ok();
    };

  private:
    std::function<void(const std::vector<uint8_t>&)> on_nci_data_cb_;
    std::function<void(NfcEvent, NfcStatus)> on_hal_event_cb_;
};

class NfcAidl : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        infc_ = INfc::fromBinder(binder);
        ASSERT_NE(infc_, nullptr);
    }
    std::shared_ptr<INfc> infc_;
};

/*
 * OpenAndCloseForDisable:
 * Makes an open call, waits for NfcEvent::OPEN_CPLT
 * Immediately calls close(NfcCloseType::DISABLE) and
 * waits for NfcEvent::CLOSE_CPLT
 *
 */
TEST_P(NfcAidl, OpenAndCloseForDisable) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                LOG(INFO) << StringPrintf("%s,%d ", __func__, event);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close DISABLE";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    LOG(INFO) << "wait for close";
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * OpenAndCloseForHostSwitchedOff:
 * Makes an open call, waits for NfcEvent::OPEN_CPLT
 * Immediately calls close(NfcCloseType::HOST_SWITCHED_OFF) and
 * waits for NfcEvent::CLOSE_CPLT
 *
 */
TEST_P(NfcAidl, OpenAndCloseForHostSwitchedOff) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close HOST_SWITCHED_OFF";
    EXPECT_TRUE(infc_->close(NfcCloseType::HOST_SWITCHED_OFF).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * OpenAfterOpen:
 * Calls open() multiple times
 * Checks status
 */
TEST_P(NfcAidl, OpenAfterOpen) {
    int open_count = 0;
    std::promise<void> open_cb_promise;
    std::promise<void> open2_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto open2_cb_future = open2_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &open2_cb_promise, &open_count](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) {
                    open_count == 0 ? open_cb_promise.set_value() : open2_cb_promise.set_value();
                    open_count++;
                }
            },
            [](auto) {});

    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Open again and wait for OPEN_CPLT
    LOG(INFO) << "open again";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open2_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * CloseAfterClose:
 * Calls close() multiple times
 * Checks status
 */
TEST_P(NfcAidl, CloseAfterClose) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
    // Close again should fail.
    LOG(INFO) << "close again";
    EXPECT_TRUE(!(infc_->close(NfcCloseType::DISABLE).isOk()));
}

/*
 * PowerCycleAfterOpen:
 * Calls powerCycle() after open
 * Waits for NfcEvent.OPEN_CPLT
 * Checks status
 */
TEST_P(NfcAidl, PowerCycleAfterOpen) {
    int open_cplt_count = 0;
    std::promise<void> open_cb_promise;
    std::promise<void> power_cycle_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto power_cycle_cb_future = power_cycle_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise, &power_cycle_cb_promise, &open_cplt_count](
                    auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) {
                    if (open_cplt_count == 0) {
                        open_cplt_count++;
                        open_cb_promise.set_value();
                    } else {
                        power_cycle_cb_promise.set_value();
                    }
                }
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // PowerCycle and wait for OPEN_CPLT
    LOG(INFO) << "PowerCycle";
    EXPECT_TRUE(infc_->powerCycle().isOk());
    EXPECT_EQ(power_cycle_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * PowerCycleAfterClose:
 * Calls powerCycle() after close
 * PowerCycle should fail immediately
 */
TEST_P(NfcAidl, PowerCycleAfterClose) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);

    // PowerCycle should fail
    LOG(INFO) << "PowerCycle";
    EXPECT_TRUE(!(infc_->powerCycle().isOk()));
}

/*
 * CoreInitializedAfterOpen:
 * Calls coreInitialized() after open
 * Waits for NfcEvent.POST_INIT_CPLT
 */
TEST_P(NfcAidl, CoreInitializedAfterOpen) {
    std::promise<void> open_cb_promise;
    std::promise<void> core_init_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto core_init_cb_future = core_init_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise, &core_init_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::POST_INIT_CPLT) core_init_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // CoreInitialized and wait for POST_INIT_CPLT
    LOG(INFO) << "coreInitialized";
    EXPECT_TRUE(infc_->coreInitialized().isOk());
    EXPECT_EQ(core_init_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);
}

/*
 * CoreInitializedAfterClose:
 * Calls coreInitialized() after close
 * coreInitialized() should fail immediately
 */
TEST_P(NfcAidl, CoreInitializedAfterClose) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);

    // coreInitialized should fail
    LOG(INFO) << "CoreInitialized";
    EXPECT_TRUE(!(infc_->coreInitialized().isOk()));
}

/*
 * PreDiscoverAfterClose:
 * Call preDiscover() after close
 * preDiscover() should fail immediately
 */
TEST_P(NfcAidl, PreDiscoverAfterClose) {
    std::promise<void> open_cb_promise;
    std::promise<void> close_cb_promise;
    auto open_cb_future = open_cb_promise.get_future();
    auto close_cb_future = close_cb_promise.get_future();
    std::shared_ptr<INfcClientCallback> mCallback = ::ndk::SharedRefBase::make<NfcClientCallback>(
            [&open_cb_promise, &close_cb_promise](auto event, auto status) {
                EXPECT_EQ(status, NfcStatus::OK);
                if (event == NfcEvent::OPEN_CPLT) open_cb_promise.set_value();
                if (event == NfcEvent::CLOSE_CPLT) close_cb_promise.set_value();
            },
            [](auto) {});
    std::chrono::milliseconds timeout{kCallbackTimeoutMs};
    // Open and wait for OPEN_CPLT
    LOG(INFO) << "open";
    EXPECT_TRUE(infc_->open(mCallback).isOk());
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);

    // Close and wait for CLOSE_CPLT
    LOG(INFO) << "close";
    EXPECT_TRUE(infc_->close(NfcCloseType::DISABLE).isOk());
    EXPECT_EQ(close_cb_future.wait_for(timeout), std::future_status::ready);

    // preDiscover should fail
    LOG(INFO) << "preDiscover";
    EXPECT_TRUE(!(infc_->preDiscover().isOk()));
}

/*
 * checkGetConfigValues:
 * Calls getConfig()
 * checks if fields in NfcConfig are populated correctly
 */
TEST_P(NfcAidl, CheckGetConfigValues) {
    NfcConfig configValue;
    EXPECT_TRUE(infc_->getConfig(&configValue).isOk());
    EXPECT_GE(configValue.maxIsoDepTransceiveLength, MIN_ISO_DEP_TRANSCEIVE_LENGTH);
    LOG(INFO) << StringPrintf("configValue.maxIsoDepTransceiveLength = %x",
                              configValue.maxIsoDepTransceiveLength);
    for (auto uicc : configValue.offHostRouteUicc) {
        LOG(INFO) << StringPrintf("offHostRouteUicc = %x", uicc);
        EXPECT_GE(uicc, MIN_OFFHOST_ROUTE_ID);
        EXPECT_LE(uicc, MAX_OFFHOST_ROUTE_ID);
    }
    for (auto ese : configValue.offHostRouteEse) {
        LOG(INFO) << StringPrintf("offHostRouteEse = %x", ese);
        EXPECT_GE(ese, MIN_OFFHOST_ROUTE_ID);
        EXPECT_LE(ese, MAX_OFFHOST_ROUTE_ID);
    }
    if (configValue.defaultIsoDepRoute != 0) {
        EXPECT_GE((uint8_t)configValue.defaultIsoDepRoute, MIN_OFFHOST_ROUTE_ID);
        EXPECT_LE((uint8_t)configValue.defaultIsoDepRoute, MAX_OFFHOST_ROUTE_ID);
    }
}

/*
 * CheckisVerboseLoggingEnabledAfterSetEnableVerboseLogging:
 * Calls setEnableVerboseLogging()
 * checks the return value of isVerboseLoggingEnabled
 */
TEST_P(NfcAidl, CheckisVerboseLoggingEnabledAfterSetEnableVerboseLogging) {
    bool enabled = false;
    EXPECT_TRUE(infc_->setEnableVerboseLogging(true).isOk());
    EXPECT_TRUE(infc_->isVerboseLoggingEnabled(&enabled).isOk());
    EXPECT_TRUE(enabled);
    EXPECT_TRUE(infc_->setEnableVerboseLogging(false).isOk());
    EXPECT_TRUE(infc_->isVerboseLoggingEnabled(&enabled).isOk());
    EXPECT_TRUE(!enabled);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NfcAidl);
INSTANTIATE_TEST_SUITE_P(Nfc, NfcAidl,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(INfc::descriptor)),
                         ::android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    std::system("/system/bin/svc nfc disable"); /* Turn off NFC */
    sleep(5);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    std::system("/system/bin/svc nfc enable"); /* Turn on NFC */
    sleep(5);
    return status;
}
