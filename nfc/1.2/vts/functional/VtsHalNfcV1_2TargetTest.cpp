/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "nfc_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/nfc/1.1/INfcClientCallback.h>
#include <android/hardware/nfc/1.2/INfc.h>
#include <android/hardware/nfc/1.2/types.h>
#include <hardware/nfc.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_1::INfcClientCallback;
using ::android::hardware::nfc::V1_1::NfcEvent;
using ::android::hardware::nfc::V1_2::INfc;
using ::android::hardware::nfc::V1_2::NfcConfig;

// Range of valid off host route ids
constexpr unsigned int MIN_OFFHOST_ROUTE_ID = 0x80;
constexpr unsigned int MAX_OFFHOST_ROUTE_ID = 0xFE;

constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[] = "sendData";

class NfcClientCallbackArgs {
   public:
    NfcEvent last_event_;
    NfcStatus last_status_;
    NfcData last_data_;
};

/* Callback class for data & Event. */
class NfcClientCallback : public ::testing::VtsHalHidlTargetCallbackBase<NfcClientCallbackArgs>,
                          public INfcClientCallback {
   public:
    virtual ~NfcClientCallback() = default;

    /* sendEvent callback function - Records the Event & Status
     * and notifies the TEST
     **/
    Return<void> sendEvent_1_1(NfcEvent event, NfcStatus event_status) override {
        NfcClientCallbackArgs args;
        args.last_event_ = event;
        args.last_status_ = event_status;
        NotifyFromCallback(kCallbackNameSendEvent, args);
        return Void();
    };

    /** NFC 1.1 HAL shouldn't send 1.0 callbacks */
    Return<void> sendEvent(__attribute__((unused))::android::hardware::nfc::V1_0::NfcEvent event,
                           __attribute__((unused)) NfcStatus event_status) override {
        return Void();
    }

    /* sendData callback function. Records the data and notifies the TEST*/
    Return<void> sendData(const NfcData& data) override {
        NfcClientCallbackArgs args;
        args.last_data_ = data;
        NotifyFromCallback(kCallbackNameSendData, args);
        return Void();
    };
};

// Test environment for Nfc HIDL HAL.
class NfcHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static NfcHidlEnvironment* Instance() {
        static NfcHidlEnvironment* instance = new NfcHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<INfc>(); }

   private:
    NfcHidlEnvironment() {}
};

// The main test class for NFC HIDL HAL.
class NfcHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        nfc_ = ::testing::VtsHalHidlTargetTestBase::getService<INfc>();
        ASSERT_NE(nfc_, nullptr);

        nfc_cb_ = new NfcClientCallback();
        ASSERT_NE(nfc_cb_, nullptr);

        EXPECT_EQ(NfcStatus::OK, nfc_->open_1_1(nfc_cb_));
        // Wait for OPEN_CPLT event
        auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
        EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

        /*
         * Close the hal and then re-open to make sure we are in a predictable
         * state for all the tests.
         */
        EXPECT_EQ(NfcStatus::OK, nfc_->close());
        // Wait for CLOSE_CPLT event
        res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
        EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

        EXPECT_EQ(NfcStatus::OK, nfc_->open_1_1(nfc_cb_));
        // Wait for OPEN_CPLT event
        res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
        EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
    }

    virtual void TearDown() override {
        EXPECT_EQ(NfcStatus::OK, nfc_->close());
        // Wait for CLOSE_CPLT event
        auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
        EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
    }

    sp<INfc> nfc_;
    sp<NfcClientCallback> nfc_cb_;
};

/*
 * getConfig:
 * Calls getConfig()
 * checks if fields in NfcConfig are populated correctly
 */
TEST_F(NfcHidlTest, GetExtendedConfig) {
    nfc_->getConfig_1_2([](NfcConfig config) {
        for (uint8_t uicc : config.offHostRouteUicc) {
            EXPECT_GE(uicc, MIN_OFFHOST_ROUTE_ID);
            EXPECT_LE(uicc, MAX_OFFHOST_ROUTE_ID);
        }
        for (uint8_t ese : config.offHostRouteEse) {
            EXPECT_GE(ese, MIN_OFFHOST_ROUTE_ID);
            EXPECT_LE(ese, MAX_OFFHOST_ROUTE_ID);
        }
        if (config.defaultIsoDepRoute != 0) {
            EXPECT_GE(config.defaultIsoDepRoute, MIN_OFFHOST_ROUTE_ID);
            EXPECT_LE(config.defaultIsoDepRoute, MAX_OFFHOST_ROUTE_ID);
        }
    });
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(NfcHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    NfcHidlEnvironment::Instance()->init(&argc, argv);

    std::system("svc nfc disable"); /* Turn off NFC */
    sleep(5);

    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;

    std::system("svc nfc enable"); /* Turn on NFC */
    sleep(5);

    return status;
}
