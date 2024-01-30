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

#include <numeric>
#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifi.h>
#include <aidl/android/hardware/wifi/BnWifiChipEventCallback.h>
#include <aidl/android/hardware/wifi/WifiIfaceMode.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::BnWifiChipEventCallback;
using aidl::android::hardware::wifi::IfaceType;
using aidl::android::hardware::wifi::IWifiApIface;
using aidl::android::hardware::wifi::IWifiChip;
using aidl::android::hardware::wifi::IWifiNanIface;
using aidl::android::hardware::wifi::IWifiP2pIface;
using aidl::android::hardware::wifi::IWifiRttController;
using aidl::android::hardware::wifi::WifiBand;
using aidl::android::hardware::wifi::WifiDebugHostWakeReasonStats;
using aidl::android::hardware::wifi::WifiDebugRingBufferStatus;
using aidl::android::hardware::wifi::WifiDebugRingBufferVerboseLevel;
using aidl::android::hardware::wifi::WifiIfaceMode;
using aidl::android::hardware::wifi::WifiRadioCombination;
using aidl::android::hardware::wifi::WifiStatusCode;
using aidl::android::hardware::wifi::WifiUsableChannel;

class WifiChipAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        stopWifiService(getInstanceName());
        wifi_chip_ = getWifiChip(getInstanceName());
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

  protected:
    int configureChipForConcurrencyType(IfaceConcurrencyType type) {
        int mode_id;
        EXPECT_TRUE(configureChipToSupportConcurrencyType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    bool isConcurrencyTypeSupported(IfaceConcurrencyType type) {
        return doesChipSupportConcurrencyType(wifi_chip_, type);
    }

    std::shared_ptr<IWifiStaIface> configureChipForStaAndGetIface() {
        std::shared_ptr<IWifiStaIface> iface;
        configureChipForConcurrencyType(IfaceConcurrencyType::STA);
        EXPECT_TRUE(wifi_chip_->createStaIface(&iface).isOk());
        EXPECT_NE(nullptr, iface.get());
        return iface;
    }

    std::shared_ptr<IWifiP2pIface> configureChipForP2pAndGetIface() {
        std::shared_ptr<IWifiP2pIface> iface;
        configureChipForConcurrencyType(IfaceConcurrencyType::P2P);
        EXPECT_TRUE(wifi_chip_->createP2pIface(&iface).isOk());
        EXPECT_NE(nullptr, iface.get());
        return iface;
    }

    std::shared_ptr<IWifiApIface> configureChipForApAndGetIface() {
        std::shared_ptr<IWifiApIface> iface;
        configureChipForConcurrencyType(IfaceConcurrencyType::AP);
        EXPECT_TRUE(wifi_chip_->createApIface(&iface).isOk());
        EXPECT_NE(nullptr, iface.get());
        return iface;
    }

    std::shared_ptr<IWifiNanIface> configureChipForNanAndGetIface() {
        std::shared_ptr<IWifiNanIface> iface;
        configureChipForConcurrencyType(IfaceConcurrencyType::NAN_IFACE);
        EXPECT_TRUE(wifi_chip_->createNanIface(&iface).isOk());
        EXPECT_NE(nullptr, iface.get());
        return iface;
    }

    std::string getStaIfaceName(const std::shared_ptr<IWifiStaIface>& iface) {
        std::string iface_name;
        EXPECT_TRUE(iface->getName(&iface_name).isOk());
        return iface_name;
    }

    std::string getP2pIfaceName(const std::shared_ptr<IWifiP2pIface>& iface) {
        std::string iface_name;
        EXPECT_TRUE(iface->getName(&iface_name).isOk());
        return iface_name;
    }

    std::string getApIfaceName(const std::shared_ptr<IWifiApIface>& iface) {
        std::string iface_name;
        EXPECT_TRUE(iface->getName(&iface_name).isOk());
        return iface_name;
    }

    std::string getNanIfaceName(const std::shared_ptr<IWifiNanIface>& iface) {
        std::string iface_name;
        EXPECT_TRUE(iface->getName(&iface_name).isOk());
        return iface_name;
    }

    std::vector<std::shared_ptr<IWifiStaIface>> create2StaIfacesIfPossible() {
        std::shared_ptr<IWifiStaIface> iface1 = configureChipForStaAndGetIface();

        // Try create a create second iface.
        std::shared_ptr<IWifiStaIface> iface2;
        bool add_second_success = wifi_chip_->createStaIface(&iface2).isOk();
        if (!add_second_success) {
            return {iface1};
        }
        EXPECT_NE(nullptr, iface2.get());
        return {iface1, iface2};
    }

    const char* getInstanceName() { return GetParam().c_str(); }

    std::shared_ptr<IWifiChip> wifi_chip_;
};

class WifiChipEventCallback : public BnWifiChipEventCallback {
  public:
    WifiChipEventCallback() = default;

    ::ndk::ScopedAStatus onChipReconfigureFailure(WifiStatusCode /* status */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onChipReconfigured(int /* modeId */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onDebugErrorAlert(int /* errorCode */,
                                           const std::vector<uint8_t>& /* debugData */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onDebugRingBufferDataAvailable(
            const WifiDebugRingBufferStatus& /* status */,
            const std::vector<uint8_t>& /* data */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onIfaceAdded(IfaceType /* type */,
                                      const std::string& /* name */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onIfaceRemoved(IfaceType /* type */,
                                        const std::string& /* name */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onRadioModeChange(
            const std::vector<RadioModeInfo>& /* radioModeInfos */) override {
        return ndk::ScopedAStatus::ok();
    }
};

/*
 * RegisterEventCallback
 *
 * Note: it is not feasible to test the invocation of the callback function,
 * since events are triggered internally in the HAL implementation and cannot be
 * triggered from the test case.
 */
TEST_P(WifiChipAidlTest, RegisterEventCallback) {
    std::shared_ptr<WifiChipEventCallback> callback =
            ndk::SharedRefBase::make<WifiChipEventCallback>();
    ASSERT_NE(nullptr, callback.get());
    EXPECT_TRUE(wifi_chip_->registerEventCallback(callback).isOk());
}

/*
 * GetFeatureSet
 */
TEST_P(WifiChipAidlTest, GetFeatureSet) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features;
    EXPECT_TRUE(wifi_chip_->getFeatureSet(&features).isOk());
}

/*
 * GetId
 */
TEST_P(WifiChipAidlTest, GetId) {
    int id;
    EXPECT_TRUE(wifi_chip_->getId(&id).isOk());
}

/*
 * GetAvailableModes
 */
TEST_P(WifiChipAidlTest, GetAvailableModes) {
    std::vector<IWifiChip::ChipMode> modes;
    EXPECT_TRUE(wifi_chip_->getAvailableModes(&modes).isOk());
    EXPECT_NE(modes.size(), 0);
}

/*
 * GetMode
 */
TEST_P(WifiChipAidlTest, GetMode) {
    int expected_mode = configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int retrieved_mode;
    EXPECT_TRUE(wifi_chip_->getMode(&retrieved_mode).isOk());
    EXPECT_EQ(retrieved_mode, expected_mode);
}

/*
 * GetUsableChannels
 */
TEST_P(WifiChipAidlTest, GetUsableChannels) {
    WifiBand band = WifiBand::BAND_24GHZ_5GHZ_6GHZ;
    uint32_t ifaceModeMask = static_cast<uint32_t>(WifiIfaceMode::IFACE_MODE_P2P_CLIENT) |
                             static_cast<uint32_t>(WifiIfaceMode::IFACE_MODE_P2P_GO);
    uint32_t filterMask =
            static_cast<uint32_t>(IWifiChip::UsableChannelFilter::CELLULAR_COEXISTENCE) |
            static_cast<uint32_t>(IWifiChip::UsableChannelFilter::CONCURRENCY);

    std::vector<WifiUsableChannel> channels;
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    auto status = wifi_chip_->getUsableChannels(band, ifaceModeMask, filterMask, &channels);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "getUsableChannels() is not supported by vendor.";
    }
    EXPECT_TRUE(status.isOk());
}

/*
 * GetSupportedRadioCombinations
 */
TEST_P(WifiChipAidlTest, GetSupportedRadioCombinations) {
    std::vector<WifiRadioCombination> combinations;
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    auto status = wifi_chip_->getSupportedRadioCombinations(&combinations);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "Skipping this test since getSupportedRadioCombinations() "
                        "is not supported by vendor.";
    }
    EXPECT_TRUE(status.isOk());
}

/*
 * SetCountryCode
 */
TEST_P(WifiChipAidlTest, SetCountryCode) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::array<uint8_t, 2> country_code = {0x55, 0x53};
    EXPECT_TRUE(wifi_chip_->setCountryCode(country_code).isOk());
}

/*
 * SetLatencyMode_normal
 * Tests the setLatencyMode() API with Latency mode NORMAL.
 */
TEST_P(WifiChipAidlTest, SetLatencyMode_normal) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    auto status = wifi_chip_->setLatencyMode(IWifiChip::LatencyMode::NORMAL);
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_LATENCY_MODE)) {
        EXPECT_TRUE(status.isOk());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SetLatencyMode_low
 * Tests the setLatencyMode() API with Latency mode LOW.
 */
TEST_P(WifiChipAidlTest, SetLatencyMode_low) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    auto status = wifi_chip_->setLatencyMode(IWifiChip::LatencyMode::LOW);
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_LATENCY_MODE)) {
        EXPECT_TRUE(status.isOk());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SetMultiStaPrimaryConnection
 *
 * Only runs if the device supports 2 STA ifaces.
 */
TEST_P(WifiChipAidlTest, SetMultiStaPrimaryConnection) {
    auto ifaces = create2StaIfacesIfPossible();
    if (ifaces.size() < 2) {
        GTEST_SKIP() << "Device does not support more than 1 STA concurrently";
    }

    auto status = wifi_chip_->setMultiStaPrimaryConnection(getStaIfaceName(ifaces[0]));
    if (!status.isOk()) {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SetMultiStaUseCase
 *
 * Only runs if the device supports 2 STA ifaces.
 */
TEST_P(WifiChipAidlTest, setMultiStaUseCase) {
    auto ifaces = create2StaIfacesIfPossible();
    if (ifaces.size() < 2) {
        GTEST_SKIP() << "Device does not support more than 1 STA concurrently";
    }

    auto status = wifi_chip_->setMultiStaUseCase(
            IWifiChip::MultiStaUseCase::DUAL_STA_TRANSIENT_PREFER_PRIMARY);
    if (!status.isOk()) {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SetCoexUnsafeChannels
 */
TEST_P(WifiChipAidlTest, SetCoexUnsafeChannels) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);

    // Test with an empty vector of CoexUnsafeChannels.
    std::vector<IWifiChip::CoexUnsafeChannel> vec;
    int restrictions = 0;
    auto status = wifi_chip_->setCoexUnsafeChannels(vec, restrictions);
    if (!status.isOk()) {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }

    // Test with a non-empty vector of CoexUnsafeChannels.
    IWifiChip::CoexUnsafeChannel unsafeChannel24Ghz;
    unsafeChannel24Ghz.band = WifiBand::BAND_24GHZ;
    unsafeChannel24Ghz.channel = 6;
    vec.push_back(unsafeChannel24Ghz);
    IWifiChip::CoexUnsafeChannel unsafeChannel5Ghz;
    unsafeChannel5Ghz.band = WifiBand::BAND_5GHZ;
    unsafeChannel5Ghz.channel = 36;
    vec.push_back(unsafeChannel5Ghz);
    restrictions = static_cast<int32_t>(IWifiChip::CoexRestriction::WIFI_AWARE) |
                   static_cast<int32_t>(IWifiChip::CoexRestriction::SOFTAP) |
                   static_cast<int32_t>(IWifiChip::CoexRestriction::WIFI_DIRECT);

    status = wifi_chip_->setCoexUnsafeChannels(vec, restrictions);
    if (!status.isOk()) {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SelectTxPowerScenario - Body
 */
TEST_P(WifiChipAidlTest, SelectTxPowerScenario_body) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    int32_t expected_features =
            static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_TX_POWER_LIMIT) |
            static_cast<int32_t>(IWifiChip::FeatureSetMask::USE_BODY_HEAD_SAR);
    auto status = wifi_chip_->selectTxPowerScenario(IWifiChip::TxPowerScenario::ON_BODY_CELL_OFF);
    if (features & expected_features) {
        EXPECT_TRUE(status.isOk());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * SelectTxPowerScenario - Voice Call
 */
TEST_P(WifiChipAidlTest, SelectTxPowerScenario_voiceCall) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    auto status = wifi_chip_->selectTxPowerScenario(IWifiChip::TxPowerScenario::VOICE_CALL);
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_TX_POWER_LIMIT)) {
        EXPECT_TRUE(status.isOk());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * ResetTxPowerScenario
 */
TEST_P(WifiChipAidlTest, ResetTxPowerScenario) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    auto status = wifi_chip_->resetTxPowerScenario();
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_TX_POWER_LIMIT)) {
        EXPECT_TRUE(status.isOk());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/*
 * ConfigureChip
 */
TEST_P(WifiChipAidlTest, ConfigureChip) {
    std::vector<IWifiChip::ChipMode> modes;
    EXPECT_TRUE(wifi_chip_->getAvailableModes(&modes).isOk());
    EXPECT_NE(modes.size(), 0);
    for (const auto& mode : modes) {
        // configureChip() requires a fresh IWifiChip object.
        wifi_chip_ = getWifiChip(getInstanceName());
        ASSERT_NE(nullptr, wifi_chip_.get());
        EXPECT_TRUE(wifi_chip_->configureChip(mode.id).isOk());
        stopWifiService(getInstanceName());
        // Sleep for 5 milliseconds between each wifi state toggle.
        usleep(5000);
    }
}

/*
 * RequestChipDebugInfo
 */
TEST_P(WifiChipAidlTest, RequestChipDebugInfo) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    IWifiChip::ChipDebugInfo debug_info = {};
    EXPECT_TRUE(wifi_chip_->requestChipDebugInfo(&debug_info).isOk());
    EXPECT_NE(debug_info.driverDescription.size(), 0);
    EXPECT_NE(debug_info.firmwareDescription.size(), 0);
}

/*
 * RequestFirmwareDebugDump
 */
TEST_P(WifiChipAidlTest, RequestFirmwareDebugDump) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::vector<uint8_t> debug_dump;
    auto status = wifi_chip_->requestFirmwareDebugDump(&debug_dump);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * RequestDriverDebugDump
 */
TEST_P(WifiChipAidlTest, RequestDriverDebugDump) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::vector<uint8_t> debug_dump;
    auto status = wifi_chip_->requestDriverDebugDump(&debug_dump);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * GetDebugRingBuffersStatus
 */
TEST_P(WifiChipAidlTest, GetDebugRingBuffersStatus) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::vector<WifiDebugRingBufferStatus> ring_buffer_status;
    auto status = wifi_chip_->getDebugRingBuffersStatus(&ring_buffer_status);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    if (status.isOk()) {
        ASSERT_NE(ring_buffer_status.size(), 0);
        for (const auto& ring_buffer : ring_buffer_status) {
            EXPECT_NE(ring_buffer.ringName.size(), 0);
        }
    }
}

/*
 * GetDebugHostWakeReasonStats
 */
TEST_P(WifiChipAidlTest, GetDebugHostWakeReasonStats) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    WifiDebugHostWakeReasonStats wake_reason_stats = {};
    auto status = wifi_chip_->getDebugHostWakeReasonStats(&wake_reason_stats);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * StartLoggingToDebugRingBuffer
 */
TEST_P(WifiChipAidlTest, StartLoggingToDebugRingBuffer) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::string ring_name;
    std::vector<WifiDebugRingBufferStatus> ring_buffer_status;

    auto status = wifi_chip_->getDebugRingBuffersStatus(&ring_buffer_status);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    if (status.isOk()) {
        ASSERT_NE(ring_buffer_status.size(), 0);
        ring_name = ring_buffer_status[0].ringName;
    }

    status = wifi_chip_->startLoggingToDebugRingBuffer(
            ring_name, WifiDebugRingBufferVerboseLevel::VERBOSE, 5, 1024);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * ForceDumpToDebugRingBuffer
 */
TEST_P(WifiChipAidlTest, ForceDumpToDebugRingBuffer) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    std::string ring_name;
    std::vector<WifiDebugRingBufferStatus> ring_buffer_status;

    auto status = wifi_chip_->getDebugRingBuffersStatus(&ring_buffer_status);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    if (status.isOk()) {
        ASSERT_NE(ring_buffer_status.size(), 0);
        ring_name = ring_buffer_status[0].ringName;
    }

    status = wifi_chip_->forceDumpToDebugRingBuffer(ring_name);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * CreateStaIface
 * Configures the chip in STA mode and creates an iface.
 */
TEST_P(WifiChipAidlTest, CreateStaIface) {
    configureChipForStaAndGetIface();
}

/*
 * CreateApIface
 */
TEST_P(WifiChipAidlTest, CreateApIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::AP)) {
        GTEST_SKIP() << "AP is not supported";
    }
    configureChipForApAndGetIface();
}

/*
 * CreateNanIface
 */
TEST_P(WifiChipAidlTest, CreateNanIface) {
    if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware")) {
        GTEST_SKIP() << "Skipping this test since NAN is not supported.";
    }
    configureChipForNanAndGetIface();
}

/*
 * CreateP2pIface
 */
TEST_P(WifiChipAidlTest, CreateP2pIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::P2P)) {
        GTEST_SKIP() << "P2P is not supported";
    }
    configureChipForP2pAndGetIface();
}

/*
 * GetStaIfaceNames
 * Configures the chip in STA mode and ensures that the iface name list is
 * empty before creating the iface. Then create the iface and ensure that
 * iface name is returned in the iface name list.
 */
TEST_P(WifiChipAidlTest, GetStaIfaceNames) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);

    std::vector<std::string> iface_names;
    EXPECT_TRUE(wifi_chip_->getP2pIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);

    std::shared_ptr<IWifiStaIface> iface;
    EXPECT_TRUE(wifi_chip_->createStaIface(&iface).isOk());
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getStaIfaceName(iface);
    EXPECT_TRUE(wifi_chip_->getStaIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 1);
    EXPECT_EQ(iface_name, iface_names[0]);

    EXPECT_TRUE(wifi_chip_->removeStaIface(iface_name).isOk());
    EXPECT_TRUE(wifi_chip_->getStaIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);
}

/*
 * GetP2pIfaceNames
 */
TEST_P(WifiChipAidlTest, GetP2pIfaceNames) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::P2P)) {
        GTEST_SKIP() << "P2P is not supported";
    }
    configureChipForConcurrencyType(IfaceConcurrencyType::P2P);

    std::vector<std::string> iface_names;
    EXPECT_TRUE(wifi_chip_->getP2pIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);

    std::shared_ptr<IWifiP2pIface> iface;
    EXPECT_TRUE(wifi_chip_->createP2pIface(&iface).isOk());
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getP2pIfaceName(iface);
    EXPECT_TRUE(wifi_chip_->getP2pIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 1);
    EXPECT_EQ(iface_name, iface_names[0]);

    EXPECT_TRUE(wifi_chip_->removeP2pIface(iface_name).isOk());
    EXPECT_TRUE(wifi_chip_->getP2pIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);
}

/*
 * GetApIfaceNames
 */
TEST_P(WifiChipAidlTest, GetApIfaceNames) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::AP)) {
        GTEST_SKIP() << "AP is not supported";
    }
    configureChipForConcurrencyType(IfaceConcurrencyType::AP);

    std::vector<std::string> iface_names;
    EXPECT_TRUE(wifi_chip_->getApIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);

    std::shared_ptr<IWifiApIface> iface;
    EXPECT_TRUE(wifi_chip_->createApIface(&iface).isOk());
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getApIfaceName(iface);
    EXPECT_TRUE(wifi_chip_->getApIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 1);
    EXPECT_EQ(iface_name, iface_names[0]);

    EXPECT_TRUE(wifi_chip_->removeApIface(iface_name).isOk());
    EXPECT_TRUE(wifi_chip_->getApIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);
}

/*
 * GetNanIfaceNames
 */
TEST_P(WifiChipAidlTest, GetNanIfaceNames) {
    if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware")) {
        GTEST_SKIP() << "Skipping this test since NAN is not supported.";
    }
    configureChipForConcurrencyType(IfaceConcurrencyType::NAN_IFACE);

    std::vector<std::string> iface_names;
    EXPECT_TRUE(wifi_chip_->getNanIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);

    std::shared_ptr<IWifiNanIface> iface;
    EXPECT_TRUE(wifi_chip_->createNanIface(&iface).isOk());
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getNanIfaceName(iface);
    EXPECT_TRUE(wifi_chip_->getNanIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 1);
    EXPECT_EQ(iface_name, iface_names[0]);

    EXPECT_TRUE(wifi_chip_->removeNanIface(iface_name).isOk());
    EXPECT_TRUE(wifi_chip_->getNanIfaceNames(&iface_names).isOk());
    EXPECT_EQ(iface_names.size(), 0);
}

/*
 * GetStaIface
 * Configures the chip in STA mode and creates an iface. Then retrieves
 * the iface object using its name and ensures that any other name
 * doesn't retrieve a valid iface object.
 */
TEST_P(WifiChipAidlTest, GetStaIface) {
    std::shared_ptr<IWifiStaIface> iface = configureChipForStaAndGetIface();
    std::string iface_name = getStaIfaceName(iface);

    std::shared_ptr<IWifiStaIface> retrieved_iface;
    EXPECT_TRUE(wifi_chip_->getStaIface(iface_name, &retrieved_iface).isOk());
    EXPECT_NE(nullptr, retrieved_iface.get());

    std::string invalid_name = iface_name + "0";
    std::shared_ptr<IWifiStaIface> invalid_iface;
    auto status = wifi_chip_->getStaIface(invalid_name, &invalid_iface);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_EQ(nullptr, invalid_iface.get());
}

/*
 * GetP2pIface
 */
TEST_P(WifiChipAidlTest, GetP2pIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::P2P)) {
        GTEST_SKIP() << "P2P is not supported";
    }
    std::shared_ptr<IWifiP2pIface> iface = configureChipForP2pAndGetIface();
    std::string iface_name = getP2pIfaceName(iface);

    std::shared_ptr<IWifiP2pIface> retrieved_iface;
    EXPECT_TRUE(wifi_chip_->getP2pIface(iface_name, &retrieved_iface).isOk());
    EXPECT_NE(nullptr, retrieved_iface.get());

    std::string invalid_name = iface_name + "0";
    std::shared_ptr<IWifiP2pIface> invalid_iface;
    auto status = wifi_chip_->getP2pIface(invalid_name, &invalid_iface);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_EQ(nullptr, invalid_iface.get());
}

/*
 * GetApIface
 */
TEST_P(WifiChipAidlTest, GetApIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::AP)) {
        GTEST_SKIP() << "AP is not supported";
    }
    std::shared_ptr<IWifiApIface> iface = configureChipForApAndGetIface();
    std::string iface_name = getApIfaceName(iface);

    std::shared_ptr<IWifiApIface> retrieved_iface;
    EXPECT_TRUE(wifi_chip_->getApIface(iface_name, &retrieved_iface).isOk());
    EXPECT_NE(nullptr, retrieved_iface.get());

    std::string invalid_name = iface_name + "0";
    std::shared_ptr<IWifiApIface> invalid_iface;
    auto status = wifi_chip_->getApIface(invalid_name, &invalid_iface);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_EQ(nullptr, invalid_iface.get());
}

/*
 * GetNanIface
 */
TEST_P(WifiChipAidlTest, GetNanIface) {
    if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware")) {
        GTEST_SKIP() << "Skipping this test since NAN is not supported.";
    }
    std::shared_ptr<IWifiNanIface> iface = configureChipForNanAndGetIface();
    std::string iface_name = getNanIfaceName(iface);

    std::shared_ptr<IWifiNanIface> retrieved_iface;
    EXPECT_TRUE(wifi_chip_->getNanIface(iface_name, &retrieved_iface).isOk());
    EXPECT_NE(nullptr, retrieved_iface.get());

    std::string invalid_name = iface_name + "0";
    std::shared_ptr<IWifiNanIface> invalid_iface;
    auto status = wifi_chip_->getNanIface(invalid_name, &invalid_iface);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_EQ(nullptr, invalid_iface.get());
}

/*
 * RemoveStaIface
 * Configures the chip in STA mode and creates an iface. Then removes
 * the iface object using the correct name and ensures that any other
 * name doesn't remove the iface.
 */
TEST_P(WifiChipAidlTest, RemoveStaIface) {
    std::shared_ptr<IWifiStaIface> iface = configureChipForStaAndGetIface();
    std::string iface_name = getStaIfaceName(iface);

    std::string invalid_name = iface_name + "0";
    auto status = wifi_chip_->removeStaIface(invalid_name);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_TRUE(wifi_chip_->removeStaIface(iface_name).isOk());

    // No such iface exists now, so this should return failure.
    EXPECT_FALSE(wifi_chip_->removeStaIface(iface_name).isOk());
}

/*
 * RemoveP2pIface
 */
TEST_P(WifiChipAidlTest, RemoveP2pIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::P2P)) {
        GTEST_SKIP() << "P2P is not supported";
    }
    std::shared_ptr<IWifiP2pIface> iface = configureChipForP2pAndGetIface();
    std::string iface_name = getP2pIfaceName(iface);

    std::string invalid_name = iface_name + "0";
    auto status = wifi_chip_->removeP2pIface(invalid_name);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_TRUE(wifi_chip_->removeP2pIface(iface_name).isOk());

    // No such iface exists now, so this should return failure.
    EXPECT_FALSE(wifi_chip_->removeP2pIface(iface_name).isOk());
}

/*
 * RemoveApIface
 */
TEST_P(WifiChipAidlTest, RemoveApIface) {
    if (!isConcurrencyTypeSupported(IfaceConcurrencyType::AP)) {
        GTEST_SKIP() << "AP is not supported";
    }
    std::shared_ptr<IWifiApIface> iface = configureChipForApAndGetIface();
    std::string iface_name = getApIfaceName(iface);

    std::string invalid_name = iface_name + "0";
    auto status = wifi_chip_->removeApIface(invalid_name);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_TRUE(wifi_chip_->removeApIface(iface_name).isOk());

    // No such iface exists now, so this should return failure.
    EXPECT_FALSE(wifi_chip_->removeApIface(iface_name).isOk());
}

/*
 * RemoveNanIface
 */
TEST_P(WifiChipAidlTest, RemoveNanIface) {
    if (!::testing::deviceSupportsFeature("android.hardware.wifi.aware")) {
        GTEST_SKIP() << "Skipping this test since NAN is not supported.";
    }
    std::shared_ptr<IWifiNanIface> iface = configureChipForNanAndGetIface();
    std::string iface_name = getNanIfaceName(iface);

    std::string invalid_name = iface_name + "0";
    auto status = wifi_chip_->removeNanIface(invalid_name);
    EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
    EXPECT_TRUE(wifi_chip_->removeNanIface(iface_name).isOk());

    // No such iface exists now, so this should return failure.
    EXPECT_FALSE(wifi_chip_->removeNanIface(iface_name).isOk());
}

/*
 * CreateRttController
 */
TEST_P(WifiChipAidlTest, CreateRttController) {
    std::shared_ptr<IWifiStaIface> iface = configureChipForStaAndGetIface();
    std::shared_ptr<IWifiRttController> rtt_controller;
    auto status = wifi_chip_->createRttController(iface, &rtt_controller);
    if (status.isOk()) {
        EXPECT_NE(nullptr, rtt_controller.get());
    } else {
        EXPECT_TRUE(checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
    }
}

/**
 * CreateBridgedApIface & RemoveIfaceInstanceFromBridgedApIface
 */
TEST_P(WifiChipAidlTest, CreateBridgedApIfaceAndremoveIfaceInstanceFromBridgedApIfaceTest) {
    bool isBridgedSupport = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_bridged_ap_supported");
    if (!isBridgedSupport) {
        GTEST_SKIP() << "Missing Bridged AP support";
    }

    std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(getInstanceName());
    ASSERT_NE(nullptr, wifi_chip.get());
    std::shared_ptr<IWifiApIface> wifi_ap_iface = getBridgedWifiApIface(wifi_chip);
    ASSERT_NE(nullptr, wifi_ap_iface.get());

    std::string br_name;
    std::vector<std::string> instances;
    EXPECT_TRUE(wifi_ap_iface->getName(&br_name).isOk());
    EXPECT_TRUE(wifi_ap_iface->getBridgedInstances(&instances).isOk());
    EXPECT_EQ(instances.size(), 2);

    std::vector<std::string> instances_after_remove;
    EXPECT_TRUE(wifi_chip->removeIfaceInstanceFromBridgedApIface(br_name, instances[0]).isOk());
    EXPECT_TRUE(wifi_ap_iface->getBridgedInstances(&instances_after_remove).isOk());
    EXPECT_EQ(instances_after_remove.size(), 1);
}

/*
 * SetVoipMode_off
 * Tests the setVoipMode() API with VoIP mode OFF.
 */
TEST_P(WifiChipAidlTest, SetVoipMode_off) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_VOIP_MODE)) {
        auto status = wifi_chip_->setVoipMode(IWifiChip::VoipMode::OFF);
        EXPECT_TRUE(status.isOk());
    } else {
        GTEST_SKIP() << "setVoipMode() is not supported by vendor.";
    }
}

/*
 * SetVoipMode_voice
 * Tests the setVoipMode() API with VoIP mode VOICE.
 */
TEST_P(WifiChipAidlTest, SetVoipMode_voice) {
    configureChipForConcurrencyType(IfaceConcurrencyType::STA);
    int32_t features = getChipFeatureSet(wifi_chip_);
    if (features & static_cast<int32_t>(IWifiChip::FeatureSetMask::SET_VOIP_MODE)) {
        auto status = wifi_chip_->setVoipMode(IWifiChip::VoipMode::VOICE);
        EXPECT_TRUE(status.isOk());
    } else {
        GTEST_SKIP() << "setVoipMode() is not supported by vendor.";
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiChipAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiChipAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
