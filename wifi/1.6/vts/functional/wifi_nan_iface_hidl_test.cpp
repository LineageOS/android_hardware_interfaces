/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Nanache License, Version 2.0 (the "License");
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

#include <android-base/logging.h>

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.6/IWifi.h>
#include <android/hardware/wifi/1.6/IWifiNanIface.h>
#include <android/hardware/wifi/1.6/IWifiNanIfaceEventCallback.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using namespace ::android::hardware::wifi::V1_0;
using namespace ::android::hardware::wifi::V1_2;
using namespace ::android::hardware::wifi::V1_4;
using namespace ::android::hardware::wifi::V1_5;
using namespace ::android::hardware::wifi::V1_6;

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;

#define TIMEOUT_PERIOD 10

android::sp<android::hardware::wifi::V1_6::IWifiNanIface> getWifiNanIface_1_6(
        const std::string& instance_name) {
    return android::hardware::wifi::V1_6::IWifiNanIface::castFrom(getWifiNanIface(instance_name));
}

/**
 * Fixture to use for all NAN Iface HIDL interface tests.
 */
class WifiNanIfaceHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware"))
            GTEST_SKIP() << "Skipping this test since NAN is not supported.";
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        iwifiNanIface = getWifiNanIface_1_6(GetInstanceName());
        ASSERT_NE(nullptr, iwifiNanIface.get());
        ASSERT_EQ(WifiStatusCode::SUCCESS, HIDL_INVOKE(iwifiNanIface, registerEventCallback_1_6,
                                                       new WifiNanIfaceEventCallback(*this))
                                                   .code);
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }


    enum CallbackType {
        ANY_CALLBACK = -1,
        INVALID = 0,
        NOTIFY_CAPABILITIES_RESPONSE = 0,
        NOTIFY_ENABLE_RESPONSE,
        NOTIFY_CONFIG_RESPONSE,
        NOTIFY_DISABLE_RESPONSE,
        NOTIFY_START_PUBLISH_RESPONSE,
        NOTIFY_STOP_PUBLISH_RESPONSE,
        NOTIFY_START_SUBSCRIBE_RESPONSE,
        NOTIFY_STOP_SUBSCRIBE_RESPONSE,
        NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE,
        NOTIFY_CREATE_DATA_INTERFACE_RESPONSE,
        NOTIFY_DELETE_DATA_INTERFACE_RESPONSE,
        NOTIFY_INITIATE_DATA_PATH_RESPONSE,
        NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE,
        NOTIFY_TERMINATE_DATA_PATH_RESPONSE,
        NOTIFY_CAPABILITIES_RESPONSE_1_5,
        NOTIFY_CAPABILITIES_RESPONSE_1_6,

        EVENT_CLUSTER_EVENT,
        EVENT_DISABLED,
        EVENT_PUBLISH_TERMINATED,
        EVENT_SUBSCRIBE_TERMINATED,
        EVENT_MATCH,
        EVENT_MATCH_EXPIRED,
        EVENT_FOLLOWUP_RECEIVED,
        EVENT_TRANSMIT_FOLLOWUP,
        EVENT_DATA_PATH_REQUEST,
        EVENT_DATA_PATH_CONFIRM,
        EVENT_DATA_PATH_TERMINATED,
        EVENT_DATA_PATH_CONFIRM_1_2,
        EVENT_DATA_PATH_SCHEDULE_UPDATE,
        EVENT_MATCH_1_6,
        EVENT_DATA_PATH_SCHEDULE_UPDATE_1_6,
        EVENT_DATA_PATH_CONFIRM_1_6,
    };

    /* Used as a mechanism to inform the test about data/event callback */
    inline void notify(CallbackType callbackType) {
        std::unique_lock<std::mutex> lock(mtx_);
        callbackEventBitMap |= (0x1 << callbackType);
        cv_.notify_one();
    }
    /* Test code calls this function to wait for data/event callback */
    /* Must set callbackEventBitMap = INVALID before call this function */
    inline std::cv_status wait(CallbackType waitForCallbackType) {
        std::unique_lock<std::mutex> lock(mtx_);

        EXPECT_NE(INVALID, waitForCallbackType);  // can't ASSERT in a
                                                  // non-void-returning method

        std::cv_status status = std::cv_status::no_timeout;
        auto now = std::chrono::system_clock::now();
        while (!(callbackEventBitMap & (0x1 << waitForCallbackType))) {
            status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
            if (status == std::cv_status::timeout)
                return status;
        }
        return status;
    }

    class WifiNanIfaceEventCallback
        : public ::android::hardware::wifi::V1_6::IWifiNanIfaceEventCallback {
        WifiNanIfaceHidlTest& parent_;

      public:
        WifiNanIfaceEventCallback(WifiNanIfaceHidlTest& parent) : parent_(parent){};

        virtual ~WifiNanIfaceEventCallback() = default;

        Return<void> notifyCapabilitiesResponse(
                uint16_t id, const WifiNanStatus& status,
                const ::android::hardware::wifi::V1_0::NanCapabilities& capabilities) override {
            parent_.id = id;
            parent_.status = status;
            parent_.capabilities = capabilities;

            parent_.notify(NOTIFY_CAPABILITIES_RESPONSE);
            return Void();
        }

        Return<void> notifyCapabilitiesResponse_1_5(
                uint16_t id, const WifiNanStatus& status,
                const ::android::hardware::wifi::V1_5::NanCapabilities& capabilities) override {
            parent_.id = id;
            parent_.status = status;
            parent_.capabilities_1_5 = capabilities;

            parent_.notify(NOTIFY_CAPABILITIES_RESPONSE_1_5);
            return Void();
        }

        Return<void> notifyEnableResponse(uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_ENABLE_RESPONSE);
            return Void();
        }

        Return<void> notifyConfigResponse(uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_CONFIG_RESPONSE);
            return Void();
        }

        Return<void> notifyDisableResponse(uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_DISABLE_RESPONSE);
            return Void();
        }

        Return<void> notifyStartPublishResponse(uint16_t id, const WifiNanStatus& status,
                                                uint8_t sessionId) override {
            parent_.id = id;
            parent_.status = status;
            parent_.sessionId = sessionId;

            parent_.notify(NOTIFY_START_PUBLISH_RESPONSE);
            return Void();
        }

        Return<void> notifyStopPublishResponse(uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_STOP_PUBLISH_RESPONSE);
            return Void();
        }

        Return<void> notifyStartSubscribeResponse(uint16_t id, const WifiNanStatus& status,
                                                  uint8_t sessionId) override {
            parent_.id = id;
            parent_.status = status;
            parent_.sessionId = sessionId;

            parent_.notify(NOTIFY_START_SUBSCRIBE_RESPONSE);
            return Void();
        }

        Return<void> notifyStopSubscribeResponse(uint16_t id,
                                                 const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_STOP_SUBSCRIBE_RESPONSE);
            return Void();
        }

        Return<void> notifyTransmitFollowupResponse(uint16_t id,
                                                    const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE);
            return Void();
        }

        Return<void> notifyCreateDataInterfaceResponse(uint16_t id,
                                                       const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_CREATE_DATA_INTERFACE_RESPONSE);
            return Void();
        }

        Return<void> notifyDeleteDataInterfaceResponse(uint16_t id,
                                                       const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_DELETE_DATA_INTERFACE_RESPONSE);
            return Void();
        }

        Return<void> notifyInitiateDataPathResponse(uint16_t id, const WifiNanStatus& status,
                                                    uint32_t ndpInstanceId) override {
            parent_.id = id;
            parent_.status = status;
            parent_.ndpInstanceId = ndpInstanceId;

            parent_.notify(NOTIFY_INITIATE_DATA_PATH_RESPONSE);
            return Void();
        }

        Return<void> notifyRespondToDataPathIndicationResponse(
                uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE);
            return Void();
        }

        Return<void> notifyTerminateDataPathResponse(uint16_t id,
                                                     const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(NOTIFY_TERMINATE_DATA_PATH_RESPONSE);
            return Void();
        }

        Return<void> eventClusterEvent(const NanClusterEventInd& event) override {
            parent_.nanClusterEventInd = event;

            parent_.notify(EVENT_CLUSTER_EVENT);
            return Void();
        }

        Return<void> eventDisabled(const WifiNanStatus& status) override {
            parent_.status = status;

            parent_.notify(EVENT_DISABLED);
            return Void();
        }

        Return<void> eventPublishTerminated(uint8_t sessionId,
                                            const WifiNanStatus& status) override {
            parent_.sessionId = sessionId;
            parent_.status = status;

            parent_.notify(EVENT_PUBLISH_TERMINATED);
            return Void();
        }

        Return<void> eventSubscribeTerminated(uint8_t sessionId,
                                              const WifiNanStatus& status) override {
            parent_.sessionId = sessionId;
            parent_.status = status;

            parent_.notify(EVENT_SUBSCRIBE_TERMINATED);
            return Void();
        }

        Return<void> eventMatch(
                const ::android::hardware::wifi::V1_0::NanMatchInd& event) override {
            parent_.nanMatchInd = event;

            parent_.notify(EVENT_MATCH);
            return Void();
        }

        Return<void> eventMatchExpired(uint8_t discoverySessionId, uint32_t peerId) override {
            parent_.sessionId = discoverySessionId;
            parent_.peerId = peerId;

            parent_.notify(EVENT_MATCH_EXPIRED);
            return Void();
        }

        Return<void> eventFollowupReceived(const NanFollowupReceivedInd& event) override {
            parent_.nanFollowupReceivedInd = event;

            parent_.notify(EVENT_FOLLOWUP_RECEIVED);
            return Void();
        }

        Return<void> eventTransmitFollowup(uint16_t id, const WifiNanStatus& status) override {
            parent_.id = id;
            parent_.status = status;

            parent_.notify(EVENT_TRANSMIT_FOLLOWUP);
            return Void();
        }

        Return<void> eventDataPathRequest(const NanDataPathRequestInd& event) override {
            parent_.nanDataPathRequestInd = event;

            parent_.notify(EVENT_DATA_PATH_REQUEST);
            return Void();
        }

        Return<void> eventDataPathConfirm(
                const ::android::hardware::wifi::V1_0::NanDataPathConfirmInd& event) override {
            parent_.nanDataPathConfirmInd = event;

            parent_.notify(EVENT_DATA_PATH_CONFIRM);
            return Void();
        }

        Return<void> eventDataPathTerminated(uint32_t ndpInstanceId) override {
            parent_.ndpInstanceId = ndpInstanceId;

            parent_.notify(EVENT_DATA_PATH_TERMINATED);
            return Void();
        }

        Return<void> eventDataPathConfirm_1_2(
                const ::android::hardware::wifi::V1_2::NanDataPathConfirmInd& event) override {
            parent_.nanDataPathConfirmInd_1_2 = event;

            parent_.notify(EVENT_DATA_PATH_CONFIRM_1_2);
            return Void();
        }

        Return<void> eventDataPathScheduleUpdate(
                const ::android::hardware::wifi::V1_2::NanDataPathScheduleUpdateInd& event)
                override {
            parent_.nanDataPathScheduleUpdateInd_1_2 = event;

            parent_.notify(EVENT_DATA_PATH_SCHEDULE_UPDATE);
            return Void();
        }

        Return<void> eventMatch_1_6(
                const ::android::hardware::wifi::V1_6::NanMatchInd& event) override {
            parent_.nanMatchInd_1_6 = event;

            parent_.notify(EVENT_MATCH_1_6);
            return Void();
        }

        Return<void> notifyCapabilitiesResponse_1_6(
                uint16_t id, const WifiNanStatus& status,
                const ::android::hardware::wifi::V1_6::NanCapabilities& capabilities) override {
            parent_.id = id;
            parent_.status = status;
            parent_.capabilities_1_6 = capabilities;

            parent_.notify(NOTIFY_CAPABILITIES_RESPONSE_1_6);
            return Void();
        }

        Return<void> eventDataPathScheduleUpdate_1_6(
                const ::android::hardware::wifi::V1_6::NanDataPathScheduleUpdateInd& event)
                override {
            parent_.nanDataPathScheduleUpdateInd_1_6 = event;

            parent_.notify(EVENT_DATA_PATH_SCHEDULE_UPDATE_1_6);
            return Void();
        }

        Return<void> eventDataPathConfirm_1_6(
                const ::android::hardware::wifi::V1_6::NanDataPathConfirmInd& event) override {
            parent_.nanDataPathConfirmInd_1_6 = event;

            parent_.notify(EVENT_DATA_PATH_CONFIRM_1_6);
            return Void();
        }
    };

  private:
    // synchronization objects
    std::mutex mtx_;
    std::condition_variable cv_;

  protected:
    android::sp<::android::hardware::wifi::V1_6::IWifiNanIface> iwifiNanIface;

    // Data from IWifiNanIfaceEventCallback callbacks: this is the collection of
    // all arguments to all callbacks. They are set by the callback
    // (notifications or events) and can be retrieved by tests.
    uint32_t callbackEventBitMap;
    uint16_t id;
    WifiNanStatus status;
    uint8_t sessionId;
    uint32_t ndpInstanceId;
    NanClusterEventInd nanClusterEventInd;
    ::android::hardware::wifi::V1_0::NanMatchInd nanMatchInd;
    ::android::hardware::wifi::V1_6::NanMatchInd nanMatchInd_1_6;
    uint32_t peerId;
    NanFollowupReceivedInd nanFollowupReceivedInd;
    NanDataPathRequestInd nanDataPathRequestInd;
    ::android::hardware::wifi::V1_0::NanCapabilities capabilities;
    ::android::hardware::wifi::V1_5::NanCapabilities capabilities_1_5;
    ::android::hardware::wifi::V1_6::NanCapabilities capabilities_1_6;
    ::android::hardware::wifi::V1_0::NanDataPathConfirmInd nanDataPathConfirmInd;
    ::android::hardware::wifi::V1_2::NanDataPathConfirmInd nanDataPathConfirmInd_1_2;
    ::android::hardware::wifi::V1_6::NanDataPathConfirmInd nanDataPathConfirmInd_1_6;
    ::android::hardware::wifi::V1_2::NanDataPathScheduleUpdateInd nanDataPathScheduleUpdateInd_1_2;
    ::android::hardware::wifi::V1_6::NanDataPathScheduleUpdateInd nanDataPathScheduleUpdateInd_1_6;

    std::string GetInstanceName() { return GetParam(); }
};

/*
 * Create:
 * Ensures that an instance of the IWifiNanIface proxy object is
 * successfully created.
 */
TEST_P(WifiNanIfaceHidlTest, Create) {
    // The creation of a proxy object is tested as part of SetUp method.
}

/*
 * enableRequest_1_6InvalidArgs: validate that fails with invalid arguments
 */
TEST_P(WifiNanIfaceHidlTest, enableRequest_1_6InvalidArgs) {
    uint16_t inputCmdId = 10;
    callbackEventBitMap = INVALID;
    ::android::hardware::wifi::V1_4::NanEnableRequest nanEnableRequest = {};
    ::android::hardware::wifi::V1_6::NanConfigRequestSupplemental nanConfigRequestSupp = {};
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, enableRequest_1_6, inputCmdId,
                                        nanEnableRequest, nanConfigRequestSupp);
    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::SUCCESS, halStatus.code);

        // wait for a callback
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(0x1 << NOTIFY_ENABLE_RESPONSE, callbackEventBitMap & (0x1 << NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(id, inputCmdId);
        ASSERT_EQ(status.status, NanStatusType::INVALID_ARGS);
    }
}

/*
 * enableRequest_1_6ShimInvalidArgs: validate that fails with invalid arguments
 * to the shim
 */
TEST_P(WifiNanIfaceHidlTest, enableRequest_1_6ShimInvalidArgs) {
    uint16_t inputCmdId = 10;
    ::android::hardware::wifi::V1_4::NanEnableRequest nanEnableRequest = {};
    nanEnableRequest.configParams.numberOfPublishServiceIdsInBeacon = 128;  // must be <= 127
    ::android::hardware::wifi::V1_6::NanConfigRequestSupplemental nanConfigRequestSupp = {};
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, enableRequest_1_6, inputCmdId,
                                        nanEnableRequest, nanConfigRequestSupp);
    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, halStatus.code);
    }
}

/*
 * configRequest_1_6InvalidArgs: validate that fails with invalid arguments
 */
TEST_P(WifiNanIfaceHidlTest, configRequest_1_6InvalidArgs) {
    uint16_t inputCmdId = 10;
    callbackEventBitMap = INVALID;
    ::android::hardware::wifi::V1_4::NanConfigRequest nanConfigRequest = {};
    ::android::hardware::wifi::V1_6::NanConfigRequestSupplemental nanConfigRequestSupp = {};
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, configRequest_1_6, inputCmdId,
                                        nanConfigRequest, nanConfigRequestSupp);

    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::SUCCESS, halStatus.code);

        // wait for a callback
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_CONFIG_RESPONSE));
        ASSERT_EQ(0x1 << NOTIFY_CONFIG_RESPONSE, callbackEventBitMap & (0x1 << NOTIFY_CONFIG_RESPONSE));
        ASSERT_EQ(id, inputCmdId);
        ASSERT_EQ(status.status, NanStatusType::INVALID_ARGS);
    }
}

/*
 * configRequest_1_6ShimInvalidArgs: validate that fails with invalid arguments
 * to the shim
 */
TEST_P(WifiNanIfaceHidlTest, configRequest_1_6ShimInvalidArgs) {
    uint16_t inputCmdId = 10;
    ::android::hardware::wifi::V1_4::NanConfigRequest nanConfigRequest = {};
    nanConfigRequest.numberOfPublishServiceIdsInBeacon = 128;  // must be <= 127
    ::android::hardware::wifi::V1_6::NanConfigRequestSupplemental nanConfigRequestSupp = {};
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, configRequest_1_6, inputCmdId,
                                        nanConfigRequest, nanConfigRequestSupp);
    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, halStatus.code);
    }
}

/*
 * notifyCapabilitiesResponse_1_6: validate that returns capabilities.
 */
TEST_P(WifiNanIfaceHidlTest, notifyCapabilitiesResponse_1_6) {
    uint16_t inputCmdId = 10;
    callbackEventBitMap = INVALID;
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, getCapabilitiesRequest_1_5, inputCmdId).code;
    ASSERT_EQ(WifiStatusCode::SUCCESS, halStatus);
    // wait for a callback
    ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_CAPABILITIES_RESPONSE_1_6));
    ASSERT_EQ(0x1 << NOTIFY_CAPABILITIES_RESPONSE_1_6, callbackEventBitMap & (0x1 << NOTIFY_CAPABILITIES_RESPONSE_1_6));
    ASSERT_EQ(id, inputCmdId);
    ASSERT_EQ(status.status, NanStatusType::SUCCESS);

    // check for reasonable capability values
    EXPECT_GT(capabilities_1_6.maxConcurrentClusters, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxPublishes, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxSubscribes, (unsigned int)0);
    EXPECT_EQ(capabilities_1_6.maxServiceNameLen, (unsigned int)255);
    EXPECT_EQ(capabilities_1_6.maxMatchFilterLen, (unsigned int)255);
    EXPECT_GT(capabilities_1_6.maxTotalMatchFilterLen, (unsigned int)255);
    EXPECT_EQ(capabilities_1_6.maxServiceSpecificInfoLen, (unsigned int)255);
    EXPECT_GE(capabilities_1_6.maxExtendedServiceSpecificInfoLen, (unsigned int)255);
    EXPECT_GT(capabilities_1_6.maxNdiInterfaces, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxNdpSessions, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxAppInfoLen, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxQueuedTransmitFollowupMsgs, (unsigned int)0);
    EXPECT_GT(capabilities_1_6.maxSubscribeInterfaceAddresses, (unsigned int)0);
    EXPECT_NE(capabilities_1_6.supportedCipherSuites, (unsigned int)0);
    EXPECT_TRUE(capabilities_1_6.instantCommunicationModeSupportFlag ||
                !capabilities_1_6.instantCommunicationModeSupportFlag);
}

/*
 * startPublishRequest_1_6: validate that success with valid arguments
 */
TEST_P(WifiNanIfaceHidlTest, startPublishRequest_1_6) {
    uint16_t inputCmdId = 10;
    ::android::hardware::wifi::V1_0::NanBandSpecificConfig config24 = {};
    config24.rssiClose = 60;
    config24.rssiMiddle = 70;
    config24.rssiCloseProximity = 60;
    config24.dwellTimeMs = 200;
    config24.scanPeriodSec = 20;
    config24.validDiscoveryWindowIntervalVal = false;
    config24.discoveryWindowIntervalVal = 0;

    ::android::hardware::wifi::V1_0::NanBandSpecificConfig config5 = {};
    config5.rssiClose = 60;
    config5.rssiMiddle = 75;
    config5.rssiCloseProximity = 60;
    config5.dwellTimeMs = 200;
    config5.scanPeriodSec = 20;
    config5.validDiscoveryWindowIntervalVal = false;
    config5.discoveryWindowIntervalVal = 0;
    ::android::hardware::wifi::V1_4::NanEnableRequest req = {};
    req.operateInBand[(size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_24GHZ] = true;
    req.operateInBand[(size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_5GHZ] = false;
    req.hopCountMax = 2;
    req.configParams.masterPref = 0;
    req.configParams.disableDiscoveryAddressChangeIndication = true;
    req.configParams.disableStartedClusterIndication = true;
    req.configParams.disableJoinedClusterIndication = true;
    req.configParams.includePublishServiceIdsInBeacon = true;
    req.configParams.numberOfPublishServiceIdsInBeacon = 0;
    req.configParams.includeSubscribeServiceIdsInBeacon = true;
    req.configParams.numberOfSubscribeServiceIdsInBeacon = 0;
    req.configParams.rssiWindowSize = 8;
    req.configParams.macAddressRandomizationIntervalSec = 1800;
    req.configParams.bandSpecificConfig[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_24GHZ] = config24;
    req.configParams.bandSpecificConfig[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_5GHZ] = config5;

    req.debugConfigs.validClusterIdVals = true;
    req.debugConfigs.clusterIdTopRangeVal = 65535;
    req.debugConfigs.clusterIdBottomRangeVal = 0;
    req.debugConfigs.validIntfAddrVal = false;
    req.debugConfigs.validOuiVal = false;
    req.debugConfigs.ouiVal = 0;
    req.debugConfigs.validRandomFactorForceVal = false;
    req.debugConfigs.randomFactorForceVal = 0;
    req.debugConfigs.validHopCountForceVal = false;
    req.debugConfigs.hopCountForceVal = 0;
    req.debugConfigs.validDiscoveryChannelVal = false;
    req.debugConfigs.discoveryChannelMhzVal[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_24GHZ] = 0;
    req.debugConfigs.discoveryChannelMhzVal[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_5GHZ] = 0;
    req.debugConfigs.validUseBeaconsInBandVal = false;
    req.debugConfigs.useBeaconsInBandVal[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_24GHZ] = true;
    req.debugConfigs.useBeaconsInBandVal[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_5GHZ] = true;
    req.debugConfigs.validUseSdfInBandVal = false;
    req.debugConfigs.useSdfInBandVal[(
            size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_24GHZ] = true;
    req.debugConfigs
            .useSdfInBandVal[(size_t)::android::hardware::wifi::V1_4::NanBandIndex::NAN_BAND_5GHZ] =
            true;

    ::android::hardware::wifi::V1_6::NanConfigRequestSupplemental nanConfigRequestSupp = {};
    nanConfigRequestSupp.V1_5.V1_2.discoveryBeaconIntervalMs = 20;
    nanConfigRequestSupp.V1_5.V1_2.numberOfSpatialStreamsInDiscovery = 0;
    nanConfigRequestSupp.V1_5.V1_2.enableDiscoveryWindowEarlyTermination = false;

    callbackEventBitMap = INVALID;

    const auto& halStatus =
            HIDL_INVOKE(iwifiNanIface, enableRequest_1_6, inputCmdId, req, nanConfigRequestSupp);
    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::SUCCESS, halStatus.code);

        // wait for a callback
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(0x1 << NOTIFY_ENABLE_RESPONSE, callbackEventBitMap & (0x1 << NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(id, inputCmdId);
        ASSERT_EQ(status.status, NanStatusType::SUCCESS);
    }
    ::android::hardware::wifi::V1_6::NanPublishRequest nanPublishRequest = {};
    nanPublishRequest.baseConfigs.sessionId = 0;
    nanPublishRequest.baseConfigs.ttlSec = 0;
    nanPublishRequest.baseConfigs.discoveryWindowPeriod = 1;
    nanPublishRequest.baseConfigs.discoveryCount = 0;
    nanPublishRequest.baseConfigs.serviceName = {97};

    nanPublishRequest.baseConfigs.discoveryMatchIndicator = NanMatchAlg::MATCH_NEVER;
    nanPublishRequest.baseConfigs.useRssiThreshold = false;
    nanPublishRequest.baseConfigs.disableDiscoveryTerminationIndication = false;
    nanPublishRequest.baseConfigs.disableMatchExpirationIndication = true;
    nanPublishRequest.baseConfigs.disableFollowupReceivedIndication = false;
    nanPublishRequest.baseConfigs.securityConfig.securityType = NanDataPathSecurityType::OPEN;
    nanPublishRequest.autoAcceptDataPathRequests = false;
    nanPublishRequest.publishType = NanPublishType::UNSOLICITED;
    nanPublishRequest.txType = NanTxType::BROADCAST;

    const auto& halStatus1 =
            HIDL_INVOKE(iwifiNanIface, startPublishRequest_1_6, inputCmdId + 1, nanPublishRequest);

    if (halStatus1.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::SUCCESS, halStatus1.code);

        // wait for a callback
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_START_PUBLISH_RESPONSE));
        ASSERT_EQ(0x1 << NOTIFY_START_PUBLISH_RESPONSE, callbackEventBitMap & (0x1 << NOTIFY_START_PUBLISH_RESPONSE));
        ASSERT_EQ(id, inputCmdId + 1);
        ASSERT_EQ(status.status, NanStatusType::SUCCESS);
    }
}

/*
 * respondToDataPathIndicationRequest_1_6InvalidArgs: validate that fails with invalid arguments
 */
TEST_P(WifiNanIfaceHidlTest, respondToDataPathIndicationRequest_1_6ShimInvalidArgs) {
    uint16_t inputCmdId = 10;
    callbackEventBitMap = INVALID;
    ::android::hardware::wifi::V1_6::NanRespondToDataPathIndicationRequest
            nanRespondToDataPathIndicationRequest = {};
    nanRespondToDataPathIndicationRequest.ifaceName = "AwareinterfaceNameTooLong";
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, respondToDataPathIndicationRequest_1_6,
                                        inputCmdId, nanRespondToDataPathIndicationRequest);

    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, halStatus.code);
    }
}

/*
 * initiateDataPathRequest_1_6InvalidArgs: validate that fails with invalid arguments
 */
TEST_P(WifiNanIfaceHidlTest, initiateDataPathRequest_1_6ShimInvalidArgs) {
    uint16_t inputCmdId = 10;
    callbackEventBitMap = INVALID;
    ::android::hardware::wifi::V1_6::NanInitiateDataPathRequest nanInitiateDataPathRequest = {};
    nanInitiateDataPathRequest.ifaceName = "AwareinterfaceNameTooLong";
    const auto& halStatus = HIDL_INVOKE(iwifiNanIface, initiateDataPathRequest_1_6, inputCmdId,
                                        nanInitiateDataPathRequest);

    if (halStatus.code != WifiStatusCode::ERROR_NOT_SUPPORTED) {
        ASSERT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, halStatus.code);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiNanIfaceHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, WifiNanIfaceHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 ::android::hardware::wifi::V1_6::IWifi::descriptor)),
                         android::hardware::PrintInstanceNameToString);
