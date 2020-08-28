/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.0/IWifiChip.h>
#include <android/hardware/wifi/1.3/IWifiChip.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::V1_0::ChipId;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiChip;
using ::android::hardware::wifi::V1_0::IWifiIface;
using ::android::hardware::wifi::V1_0::IWifiP2pIface;
using ::android::hardware::wifi::V1_0::IWifiRttController;
using ::android::hardware::wifi::V1_0::IWifiStaIface;
using ::android::hardware::wifi::V1_0::WifiDebugHostWakeReasonStats;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferVerboseLevel;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;

namespace {
constexpr WifiDebugRingBufferVerboseLevel kDebugRingBufferVerboseLvl =
    WifiDebugRingBufferVerboseLevel::VERBOSE;
constexpr uint32_t kDebugRingBufferMaxInterval = 5;
constexpr uint32_t kDebugRingBufferMaxDataSize = 1024;

/**
 * Check if any of the ring buffer capabilities are set.
 */
bool hasAnyRingBufferCapabilities(uint32_t caps) {
    return (caps &
            (IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_CONNECT_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_POWER_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_WAKELOCK_EVENT |
             IWifiChip::ChipCapabilityMask::DEBUG_RING_BUFFER_VENDOR_DATA));
}
}  // namespace

/**
 * Fixture for IWifiChip tests.
 *
 * Tests that require SoftAP or NAN support should go into WifiChipHidlApTest or
 * WifiChipHidlNanTest respectively.
 */
class WifiChipHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure test starts with a clean state
        stopWifi(GetInstanceName());

        wifi_chip_ = getWifiChip(GetInstanceName());
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    // Helper function to configure the Chip in one of the supported modes.
    // Most of the non-mode-configuration-related methods require chip
    // to be first configured.
    ChipModeId configureChipForIfaceType(IfaceType type, bool expectSuccess) {
        ChipModeId mode_id;
        EXPECT_EQ(expectSuccess,
            configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    uint32_t configureChipForStaIfaceAndGetCapabilities() {
        configureChipForIfaceType(IfaceType::STA, true);

        sp<::android::hardware::wifi::V1_3::IWifiChip> chip_converted =
            ::android::hardware::wifi::V1_3::IWifiChip::castFrom(wifi_chip_);

        std::pair<WifiStatus, uint32_t> status_and_caps;

        if (chip_converted != nullptr) {
            // Call the newer HAL version
            status_and_caps = HIDL_INVOKE(chip_converted, getCapabilities_1_3);
        } else {
            status_and_caps = HIDL_INVOKE(wifi_chip_, getCapabilities);
        }

        if (status_and_caps.first.code != WifiStatusCode::SUCCESS) {
            EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status_and_caps.first.code);
            return 0;
        }
        return status_and_caps.second;
    }

    std::string getIfaceName(const sp<IWifiIface>& iface) {
        const auto& status_and_name = HIDL_INVOKE(iface, getName);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_name.first.code);
        return status_and_name.second;
    }

    WifiStatusCode createP2pIface(sp<IWifiP2pIface>* p2p_iface) {
        const auto& status_and_iface = HIDL_INVOKE(wifi_chip_, createP2pIface);
        *p2p_iface = status_and_iface.second;
        return status_and_iface.first.code;
    }

    WifiStatusCode removeP2pIface(const std::string& name) {
        return HIDL_INVOKE(wifi_chip_, removeP2pIface, name).code;
    }

    WifiStatusCode createStaIface(sp<IWifiStaIface>* sta_iface) {
        const auto& status_and_iface = HIDL_INVOKE(wifi_chip_, createStaIface);
        *sta_iface = status_and_iface.second;
        return status_and_iface.first.code;
    }

    WifiStatusCode removeStaIface(const std::string& name) {
        return HIDL_INVOKE(wifi_chip_, removeStaIface, name).code;
    }

    sp<IWifiChip> wifi_chip_;

   protected:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * Create:
 * Ensures that an instance of the IWifiChip proxy object is
 * successfully created.
 */
TEST_P(WifiChipHidlTest, Create) {
    // The creation of a proxy object is tested as part of SetUp method.
}

/*
 * GetId:
 */
TEST_P(WifiChipHidlTest, GetId) {
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_chip_, getId).first.code);
}

/*
 * GetAvailableMode:
 */
TEST_P(WifiChipHidlTest, GetAvailableModes) {
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip_, getAvailableModes);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_modes.first.code);
    EXPECT_LT(0u, status_and_modes.second.size());
}

/*
 * ConfigureChip:
 */
TEST_P(WifiChipHidlTest, ConfigureChip) {
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip_, getAvailableModes);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_modes.first.code);
    EXPECT_LT(0u, status_and_modes.second.size());
    for (const auto& mode : status_and_modes.second) {
        // configureChip() requires to be called with a fresh IWifiChip object.
        wifi_chip_ = getWifiChip(GetInstanceName());
        ASSERT_NE(nullptr, wifi_chip_.get());
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  HIDL_INVOKE(wifi_chip_, configureChip, mode.id).code);
        stopWifi(GetInstanceName());
        // Sleep for 5 milliseconds between each wifi state toggle.
        usleep(5000);
    }
}

/*
 * GetCapabilities:
 */
TEST_P(WifiChipHidlTest, GetCapabilities) {
    configureChipForIfaceType(IfaceType::STA, true);
    const auto& status_and_caps = HIDL_INVOKE(wifi_chip_, getCapabilities);
    if (status_and_caps.first.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status_and_caps.first.code);
        return;
    }
    EXPECT_NE(0u, status_and_caps.second);
}

/*
 * GetMode:
 */
TEST_P(WifiChipHidlTest, GetMode) {
    ChipModeId chip_mode_id = configureChipForIfaceType(IfaceType::STA, true);
    const auto& status_and_mode = HIDL_INVOKE(wifi_chip_, getMode);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_mode.first.code);
    EXPECT_EQ(chip_mode_id, status_and_mode.second);
}

/*
 * RequestChipDebugInfo:
 */
TEST_P(WifiChipHidlTest, RequestChipDebugInfo) {
    configureChipForIfaceType(IfaceType::STA, true);
    const auto& status_and_chip_info =
        HIDL_INVOKE(wifi_chip_, requestChipDebugInfo);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_chip_info.first.code);
    EXPECT_LT(0u, status_and_chip_info.second.driverDescription.size());
    EXPECT_LT(0u, status_and_chip_info.second.firmwareDescription.size());
}

/*
 * RequestFirmwareDebugDump
 */
TEST_P(WifiChipHidlTest, RequestFirmwareDebugDump) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status_and_firmware_dump =
        HIDL_INVOKE(wifi_chip_, requestFirmwareDebugDump);
    if (caps & IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_FIRMWARE_DUMP) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_firmware_dump.first.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_firmware_dump.first.code);
    }
}

/*
 * RequestDriverDebugDump
 */
TEST_P(WifiChipHidlTest, RequestDriverDebugDump) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status_and_driver_dump =
        HIDL_INVOKE(wifi_chip_, requestDriverDebugDump);
    if (caps & IWifiChip::ChipCapabilityMask::DEBUG_MEMORY_DRIVER_DUMP) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_driver_dump.first.code);
    } else {
      // API semantics (today) are such that function cannot be called if not capable!
      //
      //  EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
      //            status_and_driver_dump.first.code);
    }
}

/*
 * GetDebugRingBuffersStatus
 */
TEST_P(WifiChipHidlTest, GetDebugRingBuffersStatus) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status_and_ring_buffer_status =
        HIDL_INVOKE(wifi_chip_, getDebugRingBuffersStatus);
    if (hasAnyRingBufferCapabilities(caps)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  status_and_ring_buffer_status.first.code);
        for (const auto& ring_buffer : status_and_ring_buffer_status.second) {
            EXPECT_LT(0u, ring_buffer.ringName.size());
        }
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_ring_buffer_status.first.code);
    }
}

/*
 * StartLoggingToDebugRingBuffer
 */
TEST_P(WifiChipHidlTest, StartLoggingToDebugRingBuffer) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    std::string ring_name;
    const auto& status_and_ring_buffer_status =
        HIDL_INVOKE(wifi_chip_, getDebugRingBuffersStatus);
    if (hasAnyRingBufferCapabilities(caps)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  status_and_ring_buffer_status.first.code);
        ASSERT_LT(0u, status_and_ring_buffer_status.second.size());
        ring_name = status_and_ring_buffer_status.second[0].ringName.c_str();
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_ring_buffer_status.first.code);
    }
    const auto& status =
        HIDL_INVOKE(wifi_chip_, startLoggingToDebugRingBuffer, ring_name,
                    kDebugRingBufferVerboseLvl, kDebugRingBufferMaxInterval,
                    kDebugRingBufferMaxDataSize);
    if (hasAnyRingBufferCapabilities(caps)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * ForceDumpToDebugRingBuffer
 */
TEST_P(WifiChipHidlTest, ForceDumpToDebugRingBuffer) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    std::string ring_name;
    const auto& status_and_ring_buffer_status =
        HIDL_INVOKE(wifi_chip_, getDebugRingBuffersStatus);
    if (hasAnyRingBufferCapabilities(caps)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  status_and_ring_buffer_status.first.code);
        ASSERT_LT(0u, status_and_ring_buffer_status.second.size());
        ring_name = status_and_ring_buffer_status.second[0].ringName.c_str();
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_ring_buffer_status.first.code);
    }
    const auto& status =
        HIDL_INVOKE(wifi_chip_, forceDumpToDebugRingBuffer, ring_name);
    if (hasAnyRingBufferCapabilities(caps)) {
        EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * GetDebugHostWakeReasonStats
 */
TEST_P(WifiChipHidlTest, GetDebugHostWakeReasonStats) {
    uint32_t caps = configureChipForStaIfaceAndGetCapabilities();
    const auto& status_and_debug_wake_reason =
        HIDL_INVOKE(wifi_chip_, getDebugHostWakeReasonStats);
    if (caps & IWifiChip::ChipCapabilityMask::DEBUG_HOST_WAKE_REASON_STATS) {
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  status_and_debug_wake_reason.first.code);
    } else {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  status_and_debug_wake_reason.first.code);
    }
}

/*
 * CreateP2pIface
 * Configures the chip in P2P mode and ensures that at least 1 iface creation
 * succeeds.
 */
TEST_P(WifiChipHidlTest, CreateP2pIface) {
    configureChipForIfaceType(IfaceType::P2P, true);

    sp<IWifiP2pIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&iface));
    EXPECT_NE(nullptr, iface.get());
}

/*
 * GetP2pIfaceNames
 * Configures the chip in P2P mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_P(WifiChipHidlTest, GetP2pIfaceNames) {
    configureChipForIfaceType(IfaceType::P2P, true);

    const auto& status_and_iface_names1 =
        HIDL_INVOKE(wifi_chip_, getP2pIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names1.first.code);
    EXPECT_EQ(0u, status_and_iface_names1.second.size());

    sp<IWifiP2pIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&iface));
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    const auto& status_and_iface_names2 =
        HIDL_INVOKE(wifi_chip_, getP2pIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names2.first.code);
    EXPECT_EQ(1u, status_and_iface_names2.second.size());
    EXPECT_EQ(iface_name, status_and_iface_names2.second[0]);

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeP2pIface(iface_name));
    const auto& status_and_iface_names3 =
        HIDL_INVOKE(wifi_chip_, getP2pIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names3.first.code);
    EXPECT_EQ(0u, status_and_iface_names3.second.size());
}

/*
 * GetP2pIface
 * Configures the chip in P2P mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_P(WifiChipHidlTest, GetP2pIface) {
    configureChipForIfaceType(IfaceType::P2P, true);

    sp<IWifiP2pIface> p2p_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&p2p_iface));
    ASSERT_NE(nullptr, p2p_iface.get());

    std::string iface_name = getIfaceName(p2p_iface);
    const auto& status_and_iface1 =
        HIDL_INVOKE(wifi_chip_, getP2pIface, iface_name);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface1.first.code);
    EXPECT_NE(nullptr, status_and_iface1.second.get());

    std::string invalid_name = iface_name + "0";
    const auto& status_and_iface2 =
        HIDL_INVOKE(wifi_chip_, getP2pIface, invalid_name);
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status_and_iface2.first.code);
    EXPECT_EQ(nullptr, status_and_iface2.second.get());
}

/*
 * RemoveP2pIface
 * Configures the chip in P2P mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_P(WifiChipHidlTest, RemoveP2pIface) {
    configureChipForIfaceType(IfaceType::P2P, true);

    sp<IWifiP2pIface> p2p_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createP2pIface(&p2p_iface));
    ASSERT_NE(nullptr, p2p_iface.get());

    std::string iface_name = getIfaceName(p2p_iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeP2pIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeP2pIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeP2pIface(iface_name));
}

/*
 * CreateStaIface
 * Configures the chip in STA mode and ensures that at least 1 iface creation
 * succeeds.
 */
TEST_P(WifiChipHidlTest, CreateStaIface) {
    configureChipForIfaceType(IfaceType::STA, true);

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    EXPECT_NE(nullptr, iface.get());
}

/*
 * GetStaIfaceNames
 * Configures the chip in STA mode and ensures that the iface list is empty
 * before creating the iface. Then, create the iface and ensure that
 * iface name is returned via the list.
 */
TEST_P(WifiChipHidlTest, GetStaIfaceNames) {
    configureChipForIfaceType(IfaceType::STA, true);

    const auto& status_and_iface_names1 =
        HIDL_INVOKE(wifi_chip_, getStaIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names1.first.code);
    EXPECT_EQ(0u, status_and_iface_names1.second.size());

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    ASSERT_NE(nullptr, iface.get());

    std::string iface_name = getIfaceName(iface);
    const auto& status_and_iface_names2 =
        HIDL_INVOKE(wifi_chip_, getStaIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names2.first.code);
    EXPECT_EQ(1u, status_and_iface_names2.second.size());
    EXPECT_EQ(iface_name, status_and_iface_names2.second[0]);

    EXPECT_EQ(WifiStatusCode::SUCCESS, removeStaIface(iface_name));
    const auto& status_and_iface_names3 =
        HIDL_INVOKE(wifi_chip_, getStaIfaceNames);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface_names3.first.code);
    EXPECT_EQ(0u, status_and_iface_names3.second.size());
}

/*
 * GetStaIface
 * Configures the chip in STA mode and create an iface. Then, retrieve
 * the iface object using the correct name and ensure any other name
 * doesn't retrieve an iface object.
 */
TEST_P(WifiChipHidlTest, GetStaIface) {
    configureChipForIfaceType(IfaceType::STA, true);

    sp<IWifiStaIface> sta_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&sta_iface));
    ASSERT_NE(nullptr, sta_iface.get());

    std::string iface_name = getIfaceName(sta_iface);
    const auto& status_and_iface1 =
        HIDL_INVOKE(wifi_chip_, getStaIface, iface_name);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_iface1.first.code);
    EXPECT_NE(nullptr, status_and_iface1.second.get());

    std::string invalid_name = iface_name + "0";
    const auto& status_and_iface2 =
        HIDL_INVOKE(wifi_chip_, getStaIface, invalid_name);
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, status_and_iface2.first.code);
    EXPECT_EQ(nullptr, status_and_iface2.second.get());
}

/*
 * RemoveStaIface
 * Configures the chip in STA mode and create an iface. Then, remove
 * the iface object using the correct name and ensure any other name
 * doesn't remove the iface.
 */
TEST_P(WifiChipHidlTest, RemoveStaIface) {
    configureChipForIfaceType(IfaceType::STA, true);

    sp<IWifiStaIface> sta_iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&sta_iface));
    ASSERT_NE(nullptr, sta_iface.get());

    std::string iface_name = getIfaceName(sta_iface);
    std::string invalid_name = iface_name + "0";
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeStaIface(invalid_name));
    EXPECT_EQ(WifiStatusCode::SUCCESS, removeStaIface(iface_name));

    // No such iface exists now. So, this should return failure.
    EXPECT_EQ(WifiStatusCode::ERROR_INVALID_ARGS, removeStaIface(iface_name));
}

/*
 * CreateRttController
 */
TEST_P(WifiChipHidlTest, CreateRttController) {
    configureChipForIfaceType(IfaceType::STA, true);

    sp<IWifiStaIface> iface;
    EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface));
    ASSERT_NE(nullptr, iface.get());

    const auto& status_and_rtt_controller =
        HIDL_INVOKE(wifi_chip_, createRttController, iface);
    if (status_and_rtt_controller.first.code !=
        WifiStatusCode::ERROR_NOT_SUPPORTED) {
        EXPECT_EQ(WifiStatusCode::SUCCESS,
                  status_and_rtt_controller.first.code);
        EXPECT_NE(nullptr, status_and_rtt_controller.second.get());
    }
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
