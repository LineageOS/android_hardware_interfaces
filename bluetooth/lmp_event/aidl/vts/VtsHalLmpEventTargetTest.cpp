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
 * See the License for the std::shared_ptrecific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lmp_event_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/bluetooth/lmp_event/BnBluetoothLmpEvent.h>
#include <aidl/android/hardware/bluetooth/lmp_event/BnBluetoothLmpEventCallback.h>
#include <aidl/android/hardware/bluetooth/lmp_event/Direction.h>
#include <aidl/android/hardware/bluetooth/lmp_event/AddressType.h>
#include <aidl/android/hardware/bluetooth/lmp_event/LmpEventId.h>
#include <aidl/android/hardware/bluetooth/lmp_event/Timestamp.h>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <log/log.h>

#include <chrono>
#include <condition_variable>
#include <cinttypes>
#include <thread>

using ::aidl::android::hardware::bluetooth::lmp_event::BnBluetoothLmpEventCallback;
using ::aidl::android::hardware::bluetooth::lmp_event::IBluetoothLmpEvent;
using ::aidl::android::hardware::bluetooth::lmp_event::IBluetoothLmpEventCallback;
using ::aidl::android::hardware::bluetooth::lmp_event::Direction;
using ::aidl::android::hardware::bluetooth::lmp_event::AddressType;
using ::aidl::android::hardware::bluetooth::lmp_event::LmpEventId;
using ::aidl::android::hardware::bluetooth::lmp_event::Timestamp;

using ::android::ProcessState;
using ::ndk::SpAIBinder;

namespace {
    static constexpr std::chrono::milliseconds kEventTimeoutMs(10000);
}

class BluetoothLmpEventTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        ALOGI("%s", __func__);

        ibt_lmp_event_ = IBluetoothLmpEvent::fromBinder(SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(ibt_lmp_event_, nullptr);

        ibt_lmp_event_cb_ = ndk::SharedRefBase::make<BluetoothLmpEventCallback>(*this);
        ASSERT_NE(ibt_lmp_event_cb_, nullptr);
    }

    virtual void TearDown() override {
        ALOGI("%s", __func__);
        ibt_lmp_event_->unregisterLmpEvents(address_type, address);

        ibt_lmp_event_cb_ = nullptr;
    }

    class BluetoothLmpEventCallback : public BnBluetoothLmpEventCallback {
        public:
            BluetoothLmpEventTest& parent_;
            BluetoothLmpEventCallback(BluetoothLmpEventTest& parent)
                : parent_(parent) {}
            ~BluetoothLmpEventCallback() = default;

            ::ndk::ScopedAStatus onEventGenerated(const Timestamp& timestamp, AddressType address_type,
                    const std::array<uint8_t, 6>& address, Direction direction,
                    LmpEventId lmp_event_id, char16_t conn_event_counter) override {
                for (auto t: address) {
                    ALOGD("%s: 0x%02x", __func__, t);
                }
                if (direction == Direction::TX) {
                    ALOGD("%s: Transmitting", __func__);
                } else if (direction == Direction::RX) {
                    ALOGD("%s: Receiving", __func__);
                }
                if (address_type == AddressType::PUBLIC) {
                    ALOGD("%s: Public address", __func__);
                } else if (address_type == AddressType::RANDOM) {
                    ALOGD("%s: Random address", __func__);
                }
                if (lmp_event_id == LmpEventId::CONNECT_IND) {
                    ALOGD("%s: initiating connection", __func__);
                } else if (lmp_event_id == LmpEventId::LL_PHY_UPDATE_IND) {
                    ALOGD("%s: PHY update indication", __func__);
                }

                ALOGD("%s: time: %" PRId64 "counter value: %x", __func__, timestamp.bluetoothTimeUs, conn_event_counter);

                parent_.event_recv = true;
                parent_.notify();

                return ::ndk::ScopedAStatus::ok();
            }
            ::ndk::ScopedAStatus onRegistered(bool status) override {
                ALOGD("%s: status: %d", __func__, status);
                parent_.status_recv = status;
                parent_.notify();
                return ::ndk::ScopedAStatus::ok();
            }
    };

    inline void notify() {
        std::unique_lock<std::mutex> lock(lmp_event_mtx);
        lmp_event_cv.notify_one();
    }

    inline void wait(bool is_register_event) {
        std::unique_lock<std::mutex> lock(lmp_event_mtx);


        if (is_register_event) {
            lmp_event_cv.wait(lock, [&]() { return status_recv == true; });
        } else {
            lmp_event_cv.wait_for(lock, kEventTimeoutMs,
                    [&](){ return event_recv == true; });
        }

    }

    std::shared_ptr<IBluetoothLmpEvent> ibt_lmp_event_;
    std::shared_ptr<IBluetoothLmpEventCallback> ibt_lmp_event_cb_;

    AddressType address_type;
    std::array<uint8_t, 6> address;

    std::atomic<bool> event_recv;
    bool status_recv;

    std::mutex lmp_event_mtx;
    std::condition_variable lmp_event_cv;
};

TEST_P(BluetoothLmpEventTest, RegisterAndReceive) {
    address = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    address_type = AddressType::RANDOM;
    std::vector<LmpEventId> lmp_event_ids{LmpEventId::CONNECT_IND, LmpEventId::LL_PHY_UPDATE_IND};

    ibt_lmp_event_->registerForLmpEvents(ibt_lmp_event_cb_, address_type, address, lmp_event_ids);
    wait(true);
    EXPECT_EQ(true, status_recv);

    /* Wait for event generated here */
    wait(false);
    EXPECT_EQ(true, event_recv);

    ibt_lmp_event_->unregisterLmpEvents(address_type, address);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothLmpEventTest);
INSTANTIATE_TEST_SUITE_P(BluetoothLmpEvent, BluetoothLmpEventTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IBluetoothLmpEvent::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}

