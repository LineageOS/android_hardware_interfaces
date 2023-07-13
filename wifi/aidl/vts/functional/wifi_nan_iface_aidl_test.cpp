/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Staache License, Version 2.0 (the "License");
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

#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifi.h>
#include <aidl/android/hardware/wifi/BnWifiNanIfaceEventCallback.h>
#include <aidl/android/hardware/wifi/NanBandIndex.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::BnWifiNanIfaceEventCallback;
using aidl::android::hardware::wifi::IWifiNanIface;
using aidl::android::hardware::wifi::NanBandIndex;
using aidl::android::hardware::wifi::NanBandSpecificConfig;
using aidl::android::hardware::wifi::NanBootstrappingConfirmInd;
using aidl::android::hardware::wifi::NanBootstrappingRequestInd;
using aidl::android::hardware::wifi::NanCapabilities;
using aidl::android::hardware::wifi::NanClusterEventInd;
using aidl::android::hardware::wifi::NanConfigRequest;
using aidl::android::hardware::wifi::NanConfigRequestSupplemental;
using aidl::android::hardware::wifi::NanDataPathConfirmInd;
using aidl::android::hardware::wifi::NanDataPathRequestInd;
using aidl::android::hardware::wifi::NanDataPathScheduleUpdateInd;
using aidl::android::hardware::wifi::NanDataPathSecurityType;
using aidl::android::hardware::wifi::NanEnableRequest;
using aidl::android::hardware::wifi::NanFollowupReceivedInd;
using aidl::android::hardware::wifi::NanInitiateDataPathRequest;
using aidl::android::hardware::wifi::NanMatchAlg;
using aidl::android::hardware::wifi::NanMatchInd;
using aidl::android::hardware::wifi::NanPairingConfirmInd;
using aidl::android::hardware::wifi::NanPairingRequestInd;
using aidl::android::hardware::wifi::NanPublishRequest;
using aidl::android::hardware::wifi::NanPublishType;
using aidl::android::hardware::wifi::NanRespondToDataPathIndicationRequest;
using aidl::android::hardware::wifi::NanStatus;
using aidl::android::hardware::wifi::NanStatusCode;
using aidl::android::hardware::wifi::NanSuspensionModeChangeInd;
using aidl::android::hardware::wifi::NanTxType;

#define TIMEOUT_PERIOD 10

class WifiNanIfaceAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware"))
            GTEST_SKIP() << "Skipping this test since NAN is not supported.";
        stopWifiService(getInstanceName());

        wifi_nan_iface_ = getWifiNanIface(getInstanceName());
        ASSERT_NE(nullptr, wifi_nan_iface_.get());
        std::shared_ptr<WifiNanIfaceEventCallback> callback =
                ndk::SharedRefBase::make<WifiNanIfaceEventCallback>(*this);
        EXPECT_TRUE(wifi_nan_iface_->registerEventCallback(callback).isOk());
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

    enum CallbackType {
        INVALID = 0,

        NOTIFY_CAPABILITIES_RESPONSE = 1,
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
        NOTIFY_INITIATE_PAIRING_RESPONSE,
        NOTIFY_RESPOND_TO_PAIRING_INDICATION_RESPONSE,
        NOTIFY_INITIATE_BOOTSTRAPPING_RESPONSE,
        NOTIFY_RESPOND_TO_BOOTSTRAPPING_INDICATION_RESPONSE,
        NOTIFY_SUSPEND_RESPONSE,
        NOTIFY_RESUME_RESPONSE,
        NOTIFY_TERMINATE_PAIRING_RESPONSE,

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
        EVENT_DATA_PATH_SCHEDULE_UPDATE,
        EVENT_PAIRING_REQUEST,
        EVENT_PAIRING_CONFIRM,
        EVENT_BOOTSTRAPPING_REQUEST,
        EVENT_BOOTSTRAPPING_CONFIRM,
        EVENT_SUSPENSION_MODE_CHANGE,
    };

    // Used as a mechanism to inform the test about data/event callbacks.
    inline void notify(CallbackType callbackType) {
        std::unique_lock<std::mutex> lock(mtx_);
        callback_event_bitmap_ |= (UINT64_C(0x1) << callbackType);
        cv_.notify_one();
    }

    // Test code calls this function to wait for data/event callback.
    // Must set callback_event_bitmap_ to 0 before calling this function.
    inline std::cv_status wait(CallbackType waitForCallbackType) {
        std::unique_lock<std::mutex> lock(mtx_);
        EXPECT_NE(INVALID, waitForCallbackType);

        std::cv_status status = std::cv_status::no_timeout;
        auto now = std::chrono::system_clock::now();
        while (!(receivedCallback(waitForCallbackType))) {
            status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
            if (status == std::cv_status::timeout) return status;
        }
        return status;
    }

    inline bool receivedCallback(CallbackType waitForCallbackType) {
        return callback_event_bitmap_ & (UINT64_C(0x1) << waitForCallbackType);
    }

    class WifiNanIfaceEventCallback : public BnWifiNanIfaceEventCallback {
      public:
        WifiNanIfaceEventCallback(WifiNanIfaceAidlTest& parent) : parent_(parent){};

        ::ndk::ScopedAStatus eventClusterEvent(const NanClusterEventInd& event) override {
            parent_.nan_cluster_event_ind_ = event;
            parent_.notify(EVENT_CLUSTER_EVENT);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventDataPathConfirm(const NanDataPathConfirmInd& event) override {
            parent_.nan_data_path_confirm_ind_ = event;
            parent_.notify(EVENT_DATA_PATH_CONFIRM);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventDataPathRequest(const NanDataPathRequestInd& event) override {
            parent_.nan_data_path_request_ind_ = event;
            parent_.notify(EVENT_DATA_PATH_REQUEST);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventDataPathScheduleUpdate(
                const NanDataPathScheduleUpdateInd& event) override {
            parent_.nan_data_path_schedule_update_ind_ = event;
            parent_.notify(EVENT_DATA_PATH_SCHEDULE_UPDATE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventDataPathTerminated(int32_t ndpInstanceId) override {
            parent_.ndp_instance_id_ = ndpInstanceId;
            parent_.notify(EVENT_DATA_PATH_TERMINATED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventDisabled(const NanStatus& status) override {
            parent_.status_ = status;
            parent_.notify(EVENT_DISABLED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventFollowupReceived(const NanFollowupReceivedInd& event) override {
            parent_.nan_followup_received_ind_ = event;
            parent_.notify(EVENT_FOLLOWUP_RECEIVED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventMatch(const NanMatchInd& event) override {
            parent_.nan_match_ind_ = event;
            parent_.notify(EVENT_MATCH);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventMatchExpired(int8_t discoverySessionId, int32_t peerId) override {
            parent_.session_id_ = discoverySessionId;
            parent_.peer_id_ = peerId;
            parent_.notify(EVENT_MATCH_EXPIRED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventPublishTerminated(int8_t sessionId,
                                                    const NanStatus& status) override {
            parent_.session_id_ = sessionId;
            parent_.status_ = status;
            parent_.notify(EVENT_PUBLISH_TERMINATED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventSubscribeTerminated(int8_t sessionId,
                                                      const NanStatus& status) override {
            parent_.session_id_ = sessionId;
            parent_.status_ = status;
            parent_.notify(EVENT_SUBSCRIBE_TERMINATED);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventTransmitFollowup(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(EVENT_TRANSMIT_FOLLOWUP);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventPairingConfirm(const NanPairingConfirmInd& event) override {
            parent_.nan_pairing_confirm_ind_ = event;
            parent_.notify(EVENT_PAIRING_CONFIRM);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventPairingRequest(const NanPairingRequestInd& event) override {
            parent_.nan_pairing_request_ind_ = event;
            parent_.notify(EVENT_PAIRING_REQUEST);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventBootstrappingConfirm(
                const NanBootstrappingConfirmInd& event) override {
            parent_.nan_bootstrapping_confirm_ind_ = event;
            parent_.notify(EVENT_BOOTSTRAPPING_CONFIRM);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventBootstrappingRequest(
                const NanBootstrappingRequestInd& event) override {
            parent_.nan_bootstrapping_request_ind_ = event;
            parent_.notify(EVENT_BOOTSTRAPPING_REQUEST);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus eventSuspensionModeChanged(
                const NanSuspensionModeChangeInd& event) override {
            parent_.nan_suspension_mode_change_ind_ = event;
            parent_.notify(EVENT_SUSPENSION_MODE_CHANGE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyCapabilitiesResponse(
                char16_t id, const NanStatus& status,
                const NanCapabilities& capabilities) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.capabilities_ = capabilities;
            parent_.notify(NOTIFY_CAPABILITIES_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyConfigResponse(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_CONFIG_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyCreateDataInterfaceResponse(char16_t id,
                                                               const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_CREATE_DATA_INTERFACE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyDeleteDataInterfaceResponse(char16_t id,
                                                               const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_DELETE_DATA_INTERFACE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyDisableResponse(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_DISABLE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyEnableResponse(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_ENABLE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyInitiateDataPathResponse(char16_t id, const NanStatus& status,
                                                            int32_t ndpInstanceId) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.ndp_instance_id_ = ndpInstanceId;
            parent_.notify(NOTIFY_INITIATE_DATA_PATH_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyRespondToDataPathIndicationResponse(
                char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyStartPublishResponse(char16_t id, const NanStatus& status,
                                                        int8_t sessionId) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.session_id_ = sessionId;
            parent_.notify(NOTIFY_START_PUBLISH_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyStartSubscribeResponse(char16_t id, const NanStatus& status,
                                                          int8_t sessionId) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.session_id_ = sessionId;
            parent_.notify(NOTIFY_START_SUBSCRIBE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyStopPublishResponse(char16_t id,
                                                       const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_STOP_PUBLISH_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyStopSubscribeResponse(char16_t id,
                                                         const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_STOP_SUBSCRIBE_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyTerminateDataPathResponse(char16_t id,
                                                             const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_TERMINATE_DATA_PATH_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifySuspendResponse(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_SUSPEND_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyResumeResponse(char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_RESUME_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyTransmitFollowupResponse(char16_t id,
                                                            const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyInitiatePairingResponse(char16_t id, const NanStatus& status,
                                                           int32_t pairingInstanceId) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.pairing_instance_id_ = pairingInstanceId;
            parent_.notify(NOTIFY_INITIATE_PAIRING_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyRespondToPairingIndicationResponse(
                char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_RESPOND_TO_PAIRING_INDICATION_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyInitiateBootstrappingResponse(
                char16_t id, const NanStatus& status, int32_t bootstrapppingInstanceId) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.bootstrappping_instance_id_ = bootstrapppingInstanceId;
            parent_.notify(NOTIFY_INITIATE_BOOTSTRAPPING_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyRespondToBootstrappingIndicationResponse(
                char16_t id, const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_RESPOND_TO_BOOTSTRAPPING_INDICATION_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }
        ::ndk::ScopedAStatus notifyTerminatePairingResponse(char16_t id,
                                                            const NanStatus& status) override {
            parent_.id_ = id;
            parent_.status_ = status;
            parent_.notify(NOTIFY_TERMINATE_PAIRING_RESPONSE);
            return ndk::ScopedAStatus::ok();
        }

      private:
        WifiNanIfaceAidlTest& parent_;
    };

  protected:
    std::shared_ptr<IWifiNanIface> wifi_nan_iface_;
    uint64_t callback_event_bitmap_;
    uint16_t id_;
    uint8_t session_id_;
    uint32_t ndp_instance_id_;
    uint32_t pairing_instance_id_;
    uint32_t bootstrappping_instance_id_;
    uint32_t peer_id_;
    NanCapabilities capabilities_;
    NanClusterEventInd nan_cluster_event_ind_;
    NanDataPathConfirmInd nan_data_path_confirm_ind_;
    NanDataPathRequestInd nan_data_path_request_ind_;
    NanDataPathScheduleUpdateInd nan_data_path_schedule_update_ind_;
    NanFollowupReceivedInd nan_followup_received_ind_;
    NanMatchInd nan_match_ind_;
    NanStatus status_;
    NanPairingRequestInd nan_pairing_request_ind_;
    NanPairingConfirmInd nan_pairing_confirm_ind_;
    NanBootstrappingRequestInd nan_bootstrapping_request_ind_;
    NanBootstrappingConfirmInd nan_bootstrapping_confirm_ind_;
    NanSuspensionModeChangeInd nan_suspension_mode_change_ind_;

    const char* getInstanceName() { return GetParam().c_str(); }

  private:
    // synchronization objects
    std::mutex mtx_;
    std::condition_variable cv_;
};

/*
 * FailOnIfaceInvalid
 * Ensure that API calls to an interface fail with code ERROR_WIFI_IFACE_INVALID
 * after wifi is disabled.
 */
TEST_P(WifiNanIfaceAidlTest, FailOnIfaceInvalid) {
    stopWifiService(getInstanceName());
    sleep(5);  // Ensure that all chips/interfaces are invalidated.
    auto status = wifi_nan_iface_->getCapabilitiesRequest(0);
    ASSERT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_WIFI_IFACE_INVALID));
}

/*
 * EnableRequest - Invalid Args
 */
TEST_P(WifiNanIfaceAidlTest, EnableRequest_InvalidArgs) {
    uint16_t inputCmdId = 10;
    callback_event_bitmap_ = 0;
    NanEnableRequest nanEnableRequest = {};
    NanConfigRequestSupplemental nanConfigRequestSupp = {};
    auto status =
            wifi_nan_iface_->enableRequest(inputCmdId, nanEnableRequest, nanConfigRequestSupp);
    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(status.isOk());

        // Wait for a callback.
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_ENABLE_RESPONSE));
        ASSERT_TRUE(receivedCallback(NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(id_, inputCmdId);
        ASSERT_EQ(status_.status, NanStatusCode::INVALID_ARGS);
    }
}

/*
 * ConfigRequest - Invalid Args
 */
TEST_P(WifiNanIfaceAidlTest, ConfigRequest_InvalidArgs) {
    uint16_t inputCmdId = 10;
    callback_event_bitmap_ = 0;
    NanConfigRequest nanConfigRequest = {};
    NanConfigRequestSupplemental nanConfigRequestSupp = {};
    auto status =
            wifi_nan_iface_->configRequest(inputCmdId, nanConfigRequest, nanConfigRequestSupp);

    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(status.isOk());

        // Wait for a callback.
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_CONFIG_RESPONSE));
        ASSERT_TRUE(receivedCallback(NOTIFY_CONFIG_RESPONSE));
        ASSERT_EQ(id_, inputCmdId);
        ASSERT_EQ(status_.status, NanStatusCode::INVALID_ARGS);
    }
}

/*
 * EnableRequest - Invalid Args in Shim Conversion
 */
TEST_P(WifiNanIfaceAidlTest, EnableRequest_InvalidShimArgs) {
    uint16_t inputCmdId = 10;
    NanEnableRequest nanEnableRequest = {};
    nanEnableRequest.configParams.numberOfPublishServiceIdsInBeacon = -15;  // must be > 0
    NanConfigRequestSupplemental nanConfigRequestSupp = {};
    auto status =
            wifi_nan_iface_->enableRequest(inputCmdId, nanEnableRequest, nanConfigRequestSupp);
    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    }
}

/*
 * ConfigRequest - Invalid Args in Shim Conversion
 */
TEST_P(WifiNanIfaceAidlTest, ConfigRequest_InvalidShimArgs) {
    uint16_t inputCmdId = 10;
    NanConfigRequest nanConfigRequest = {};
    nanConfigRequest.numberOfPublishServiceIdsInBeacon = -15;  // must be > 0
    NanConfigRequestSupplemental nanConfigRequestSupp = {};
    auto status =
            wifi_nan_iface_->configRequest(inputCmdId, nanConfigRequest, nanConfigRequestSupp);
    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    }
}

/*
 * NotifyCapabilitiesResponse
 */
TEST_P(WifiNanIfaceAidlTest, NotifyCapabilitiesResponse) {
    uint16_t inputCmdId = 10;
    callback_event_bitmap_ = 0;
    EXPECT_TRUE(wifi_nan_iface_->getCapabilitiesRequest(inputCmdId).isOk());

    // Wait for a callback.
    ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_CAPABILITIES_RESPONSE));
    ASSERT_TRUE(receivedCallback(NOTIFY_CAPABILITIES_RESPONSE));
    ASSERT_EQ(id_, inputCmdId);
    ASSERT_EQ(status_.status, NanStatusCode::SUCCESS);

    // Check for reasonable capability values.
    EXPECT_GT(capabilities_.maxConcurrentClusters, 0);
    EXPECT_GT(capabilities_.maxPublishes, 0);
    EXPECT_GT(capabilities_.maxSubscribes, 0);
    EXPECT_EQ(capabilities_.maxServiceNameLen, 255);
    EXPECT_EQ(capabilities_.maxMatchFilterLen, 255);
    EXPECT_GT(capabilities_.maxTotalMatchFilterLen, 255);
    EXPECT_EQ(capabilities_.maxServiceSpecificInfoLen, 255);
    EXPECT_GE(capabilities_.maxExtendedServiceSpecificInfoLen, 255);
    EXPECT_GT(capabilities_.maxNdiInterfaces, 0);
    EXPECT_GT(capabilities_.maxNdpSessions, 0);
    EXPECT_GT(capabilities_.maxAppInfoLen, 0);
    EXPECT_GT(capabilities_.maxQueuedTransmitFollowupMsgs, 0);
    EXPECT_GT(capabilities_.maxSubscribeInterfaceAddresses, 0);
    EXPECT_NE(static_cast<int32_t>(capabilities_.supportedCipherSuites), 0);
}

/*
 * StartPublishRequest
 */
TEST_P(WifiNanIfaceAidlTest, StartPublishRequest) {
    uint16_t inputCmdId = 10;
    NanBandSpecificConfig config24 = {};
    config24.rssiClose = 60;
    config24.rssiMiddle = 70;
    config24.rssiCloseProximity = 60;
    config24.dwellTimeMs = 200;
    config24.scanPeriodSec = 20;
    config24.validDiscoveryWindowIntervalVal = false;
    config24.discoveryWindowIntervalVal = 0;

    NanBandSpecificConfig config5 = {};
    config5.rssiClose = 60;
    config5.rssiMiddle = 75;
    config5.rssiCloseProximity = 60;
    config5.dwellTimeMs = 200;
    config5.scanPeriodSec = 20;
    config5.validDiscoveryWindowIntervalVal = false;
    config5.discoveryWindowIntervalVal = 0;

    NanEnableRequest req = {};
    req.operateInBand[static_cast<int32_t>(NanBandIndex::NAN_BAND_24GHZ)] = true;
    req.operateInBand[static_cast<int32_t>(NanBandIndex::NAN_BAND_5GHZ)] = false;
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
    req.configParams.bandSpecificConfig[static_cast<int32_t>(NanBandIndex::NAN_BAND_24GHZ)] =
            config24;
    req.configParams.bandSpecificConfig[static_cast<int32_t>(NanBandIndex::NAN_BAND_5GHZ)] =
            config5;

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
    req.debugConfigs.discoveryChannelMhzVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_24GHZ)] = 0;
    req.debugConfigs.discoveryChannelMhzVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_5GHZ)] = 0;
    req.debugConfigs.validUseBeaconsInBandVal = false;
    req.debugConfigs.useBeaconsInBandVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_24GHZ)] = true;
    req.debugConfigs.useBeaconsInBandVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_5GHZ)] = true;
    req.debugConfigs.validUseSdfInBandVal = false;
    req.debugConfigs.useSdfInBandVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_24GHZ)] = true;
    req.debugConfigs.useSdfInBandVal[static_cast<int32_t>(NanBandIndex::NAN_BAND_5GHZ)] = true;

    NanConfigRequestSupplemental nanConfigRequestSupp = {};
    nanConfigRequestSupp.discoveryBeaconIntervalMs = 20;
    nanConfigRequestSupp.numberOfSpatialStreamsInDiscovery = 0;
    nanConfigRequestSupp.enableDiscoveryWindowEarlyTermination = false;

    callback_event_bitmap_ = 0;
    auto status = wifi_nan_iface_->enableRequest(inputCmdId, req, nanConfigRequestSupp);
    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(status.isOk());

        // Wait for a callback.
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_ENABLE_RESPONSE));
        ASSERT_TRUE(receivedCallback(NOTIFY_ENABLE_RESPONSE));
        ASSERT_EQ(id_, inputCmdId);
        ASSERT_EQ(status_.status, NanStatusCode::SUCCESS);
    }

    NanPublishRequest nanPublishRequest = {};
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

    status = wifi_nan_iface_->startPublishRequest(inputCmdId + 1, nanPublishRequest);
    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_TRUE(status.isOk());

        // Wait for a callback.
        ASSERT_EQ(std::cv_status::no_timeout, wait(NOTIFY_START_PUBLISH_RESPONSE));
        ASSERT_TRUE(receivedCallback(NOTIFY_START_PUBLISH_RESPONSE));
        ASSERT_EQ(id_, inputCmdId + 1);
        ASSERT_EQ(status_.status, NanStatusCode::SUCCESS);
    }
}

/*
 * RespondToDataPathIndicationRequest - Invalid Args
 */
TEST_P(WifiNanIfaceAidlTest, RespondToDataPathIndicationRequest_InvalidArgs) {
    uint16_t inputCmdId = 10;
    callback_event_bitmap_ = 0;
    NanRespondToDataPathIndicationRequest nanRespondToDataPathIndicationRequest = {};
    nanRespondToDataPathIndicationRequest.ifaceName = "AwareInterfaceNameTooLong";
    auto status = wifi_nan_iface_->respondToDataPathIndicationRequest(
            inputCmdId, nanRespondToDataPathIndicationRequest);

    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_EQ(status.getServiceSpecificError(),
                  static_cast<int32_t>(WifiStatusCode::ERROR_INVALID_ARGS));
    }
}

/*
 * InitiateDataPathRequest - Invalid Args
 */
TEST_P(WifiNanIfaceAidlTest, InitiateDataPathRequest_InvalidArgs) {
    uint16_t inputCmdId = 10;
    callback_event_bitmap_ = 0;
    NanInitiateDataPathRequest nanInitiateDataPathRequest = {};
    nanInitiateDataPathRequest.ifaceName = "AwareInterfaceNameTooLong";
    auto status = wifi_nan_iface_->initiateDataPathRequest(inputCmdId, nanInitiateDataPathRequest);

    if (!checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        ASSERT_EQ(status.getServiceSpecificError(),
                  static_cast<int32_t>(WifiStatusCode::ERROR_INVALID_ARGS));
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiNanIfaceAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiNanIfaceAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
