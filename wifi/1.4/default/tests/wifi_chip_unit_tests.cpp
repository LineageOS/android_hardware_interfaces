/*
 * Copyright (C) 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "wifi_chip.h"

#include "mock_interface_tool.h"
#include "mock_wifi_feature_flags.h"
#include "mock_wifi_iface_util.h"
#include "mock_wifi_legacy_hal.h"
#include "mock_wifi_mode_controller.h"

using testing::NiceMock;
using testing::Return;
using testing::Test;

namespace {
using android::hardware::wifi::V1_0::ChipId;

constexpr ChipId kFakeChipId = 5;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {

class WifiChipTest : public Test {
   protected:
    void setupV1IfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinationsSta = {
            {{{{IfaceType::STA}, 1}, {{IfaceType::P2P}, 1}}}
        };
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinationsAp = {
            {{{{IfaceType::AP}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV1Sta, combinationsSta},
            {feature_flags::chip_mode_ids::kV1Ap, combinationsAp}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void setupV1_AwareIfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinationsSta = {
            {{{{IfaceType::STA}, 1}, {{IfaceType::P2P, IfaceType::NAN}, 1}}}
        };
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinationsAp = {
            {{{{IfaceType::AP}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV1Sta, combinationsSta},
            {feature_flags::chip_mode_ids::kV1Ap, combinationsAp}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void setupV1_AwareDisabledApIfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinationsSta = {
            {{{{IfaceType::STA}, 1}, {{IfaceType::P2P, IfaceType::NAN}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV1Sta, combinationsSta}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void setupV2_AwareIfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinations = {
            {{{{IfaceType::STA}, 1}, {{IfaceType::AP}, 1}}},
            {{{{IfaceType::STA}, 1}, {{IfaceType::P2P, IfaceType::NAN}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV3, combinations}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void setupV2_AwareDisabledApIfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinations = {
            {{{{IfaceType::STA}, 1}, {{IfaceType::P2P, IfaceType::NAN}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV3, combinations}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void setup_MultiIfaceCombination() {
        // clang-format off
        const hidl_vec<V1_0::IWifiChip::ChipIfaceCombination> combinations = {
            {{{{IfaceType::STA}, 3}, {{IfaceType::AP}, 1}}}
        };
        const std::vector<V1_0::IWifiChip::ChipMode> modes = {
            {feature_flags::chip_mode_ids::kV3, combinations}
        };
        // clang-format on
        EXPECT_CALL(*feature_flags_, getChipModes())
            .WillRepeatedly(testing::Return(modes));
    }

    void assertNumberOfModes(uint32_t num_modes) {
        chip_->getAvailableModes(
            [num_modes](const WifiStatus& status,
                        const std::vector<WifiChip::ChipMode>& modes) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                // V2_Aware has 1 mode of operation.
                ASSERT_EQ(num_modes, modes.size());
            });
    }

    void findModeAndConfigureForIfaceType(const IfaceType& type) {
        // This should be aligned with kInvalidModeId in wifi_chip.cpp.
        ChipModeId mode_id = UINT32_MAX;
        chip_->getAvailableModes(
            [&mode_id, &type](const WifiStatus& status,
                              const std::vector<WifiChip::ChipMode>& modes) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                for (const auto& mode : modes) {
                    for (const auto& combination : mode.availableCombinations) {
                        for (const auto& limit : combination.limits) {
                            if (limit.types.end() !=
                                std::find(limit.types.begin(),
                                          limit.types.end(), type)) {
                                mode_id = mode.id;
                            }
                        }
                    }
                }
            });
        ASSERT_NE(UINT32_MAX, mode_id);

        chip_->configureChip(mode_id, [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
    }

    // Returns an empty string on error.
    std::string createIface(const IfaceType& type) {
        std::string iface_name;
        if (type == IfaceType::AP) {
            chip_->createApIface([&iface_name](
                                     const WifiStatus& status,
                                     const sp<V1_0::IWifiApIface>& iface) {
                if (WifiStatusCode::SUCCESS == status.code) {
                    ASSERT_NE(iface.get(), nullptr);
                    iface->getName([&iface_name](const WifiStatus& status,
                                                 const hidl_string& name) {
                        ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                        iface_name = name.c_str();
                    });
                }
            });
        } else if (type == IfaceType::NAN) {
            chip_->createNanIface(
                [&iface_name](
                    const WifiStatus& status,
                    const sp<android::hardware::wifi::V1_0::IWifiNanIface>&
                        iface) {
                    if (WifiStatusCode::SUCCESS == status.code) {
                        ASSERT_NE(iface.get(), nullptr);
                        iface->getName([&iface_name](const WifiStatus& status,
                                                     const hidl_string& name) {
                            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                            iface_name = name.c_str();
                        });
                    }
                });
        } else if (type == IfaceType::P2P) {
            chip_->createP2pIface(
                [&iface_name](const WifiStatus& status,
                              const sp<IWifiP2pIface>& iface) {
                    if (WifiStatusCode::SUCCESS == status.code) {
                        ASSERT_NE(iface.get(), nullptr);
                        iface->getName([&iface_name](const WifiStatus& status,
                                                     const hidl_string& name) {
                            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                            iface_name = name.c_str();
                        });
                    }
                });
        } else if (type == IfaceType::STA) {
            chip_->createStaIface(
                [&iface_name](const WifiStatus& status,
                              const sp<V1_0::IWifiStaIface>& iface) {
                    if (WifiStatusCode::SUCCESS == status.code) {
                        ASSERT_NE(iface.get(), nullptr);
                        iface->getName([&iface_name](const WifiStatus& status,
                                                     const hidl_string& name) {
                            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
                            iface_name = name.c_str();
                        });
                    }
                });
        }
        return iface_name;
    }

    void removeIface(const IfaceType& type, const std::string& iface_name) {
        if (type == IfaceType::AP) {
            chip_->removeApIface(iface_name, [](const WifiStatus& status) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            });
        } else if (type == IfaceType::NAN) {
            chip_->removeNanIface(iface_name, [](const WifiStatus& status) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            });
        } else if (type == IfaceType::P2P) {
            chip_->removeP2pIface(iface_name, [](const WifiStatus& status) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            });
        } else if (type == IfaceType::STA) {
            chip_->removeStaIface(iface_name, [](const WifiStatus& status) {
                ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            });
        }
    }

    bool createRttController() {
        bool success = false;
        chip_->createRttController_1_4(
            NULL, [&success](const WifiStatus& status,
                             const sp<IWifiRttController>& rtt) {
                if (WifiStatusCode::SUCCESS == status.code) {
                    ASSERT_NE(rtt.get(), nullptr);
                    success = true;
                }
            });
        return success;
    }

    sp<WifiChip> chip_;
    ChipId chip_id_ = kFakeChipId;
    std::shared_ptr<NiceMock<wifi_system::MockInterfaceTool>> iface_tool_{
        new NiceMock<wifi_system::MockInterfaceTool>};
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
        new NiceMock<legacy_hal::MockWifiLegacyHal>(iface_tool_)};
    std::shared_ptr<NiceMock<mode_controller::MockWifiModeController>>
        mode_controller_{new NiceMock<mode_controller::MockWifiModeController>};
    std::shared_ptr<NiceMock<iface_util::MockWifiIfaceUtil>> iface_util_{
        new NiceMock<iface_util::MockWifiIfaceUtil>(iface_tool_)};
    std::shared_ptr<NiceMock<feature_flags::MockWifiFeatureFlags>>
        feature_flags_{new NiceMock<feature_flags::MockWifiFeatureFlags>};

   public:
    void SetUp() override {
        chip_ = new WifiChip(chip_id_, legacy_hal_, mode_controller_,
                             iface_util_, feature_flags_);

        EXPECT_CALL(*mode_controller_, changeFirmwareMode(testing::_))
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*legacy_hal_, start())
            .WillRepeatedly(testing::Return(legacy_hal::WIFI_SUCCESS));
    }

    void TearDown() override {
        // Restore default system iface names (This should ideally be using a
        // mock).
        property_set("wifi.interface", "wlan0");
        property_set("wifi.concurrent.interface", "wlan1");
        property_set("wifi.aware.interface", nullptr);
    }
};

////////// V1 Iface Combinations ////////////
// Mode 1 - STA + P2P
// Mode 2 - AP
class WifiChipV1IfaceCombinationTest : public WifiChipTest {
   public:
    void SetUp() override {
        setupV1IfaceCombination();
        WifiChipTest::SetUp();
        // V1 has 2 modes of operation.
        assertNumberOfModes(2u);
    }
};

TEST_F(WifiChipV1IfaceCombinationTest, StaMode_CreateSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
}

TEST_F(WifiChipV1IfaceCombinationTest, StaMode_CreateP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, StaMode_CreateNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, StaMode_CreateAp_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createIface(IfaceType::AP).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, StaMode_CreateStaP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, ApMode_CreateAp_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan0");
}

TEST_F(WifiChipV1IfaceCombinationTest, ApMode_CreateSta_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, ApMode_CreateP2p_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV1IfaceCombinationTest, ApMode_CreateNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

////////// V1 + Aware Iface Combinations ////////////
// Mode 1 - STA + P2P/NAN
// Mode 2 - AP
class WifiChipV1_AwareIfaceCombinationTest : public WifiChipTest {
   public:
    void SetUp() override {
        setupV1_AwareIfaceCombination();
        WifiChipTest::SetUp();
        // V1_Aware has 2 modes of operation.
        assertNumberOfModes(2u);
    }
};

TEST_F(WifiChipV1_AwareIfaceCombinationTest, StaMode_CreateSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, StaMode_CreateP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, StaMode_CreateNan_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, StaMode_CreateAp_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createIface(IfaceType::AP).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest,
       StaMode_CreateStaP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest,
       StaMode_CreateStaNan_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest,
       StaMode_CreateStaP2PNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest,
       StaMode_CreateStaNan_AfterP2pRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto p2p_iface_name = createIface(IfaceType::P2P);
    ASSERT_FALSE(p2p_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());

    // After removing P2P iface, NAN iface creation should succeed.
    removeIface(IfaceType::P2P, p2p_iface_name);
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest,
       StaMode_CreateStaP2p_AfterNanRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto nan_iface_name = createIface(IfaceType::NAN);
    ASSERT_FALSE(nan_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::P2P).empty());

    // After removing NAN iface, P2P iface creation should succeed.
    removeIface(IfaceType::NAN, nan_iface_name);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, ApMode_CreateAp_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan0");
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, ApMode_CreateSta_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, ApMode_CreateP2p_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, ApMode_CreateNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, RttControllerFlowStaModeNoSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, RttControllerFlowStaModeWithSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, RttControllerFlowApToSta) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    const auto ap_iface_name = createIface(IfaceType::AP);
    ASSERT_FALSE(ap_iface_name.empty());
    ASSERT_FALSE(createRttController());

    removeIface(IfaceType::AP, ap_iface_name);

    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, SelectTxScenarioWithOnlySta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    EXPECT_CALL(*legacy_hal_, selectTxPowerScenario("wlan0", testing::_))
        .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    chip_->selectTxPowerScenario_1_2(
        V1_2::IWifiChip::TxPowerScenario::ON_HEAD_CELL_OFF,
        [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
}

TEST_F(WifiChipV1_AwareIfaceCombinationTest, SelectTxScenarioWithOnlyAp) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan0");
    EXPECT_CALL(*legacy_hal_, selectTxPowerScenario("wlan0", testing::_))
        .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    chip_->selectTxPowerScenario_1_2(
        V1_2::IWifiChip::TxPowerScenario::ON_HEAD_CELL_OFF,
        [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
}

////////// V2 + Aware Iface Combinations ////////////
// Mode 1 - STA + STA/AP
//        - STA + P2P/NAN
class WifiChipV2_AwareIfaceCombinationTest : public WifiChipTest {
   public:
    void SetUp() override {
        setupV2_AwareIfaceCombination();
        WifiChipTest::SetUp();
        // V2_Aware has 1 mode of operation.
        assertNumberOfModes(1u);
    }
};

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateNan_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateAp_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan1");
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaSta_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaAp_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    ASSERT_EQ(createIface(IfaceType::AP), "wlan1");
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateApSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan1");
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       CreateSta_AfterStaApRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    const auto sta_iface_name = createIface(IfaceType::STA);
    ASSERT_FALSE(sta_iface_name.empty());
    const auto ap_iface_name = createIface(IfaceType::AP);
    ASSERT_FALSE(ap_iface_name.empty());

    ASSERT_TRUE(createIface(IfaceType::STA).empty());

    // After removing AP & STA iface, STA iface creation should succeed.
    removeIface(IfaceType::STA, sta_iface_name);
    removeIface(IfaceType::AP, ap_iface_name);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaP2p_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaNan_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaP2PNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       CreateStaNan_AfterP2pRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto p2p_iface_name = createIface(IfaceType::P2P);
    ASSERT_FALSE(p2p_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());

    // After removing P2P iface, NAN iface creation should succeed.
    removeIface(IfaceType::P2P, p2p_iface_name);
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       CreateStaP2p_AfterNanRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto nan_iface_name = createIface(IfaceType::NAN);
    ASSERT_FALSE(nan_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::P2P).empty());

    // After removing NAN iface, P2P iface creation should succeed.
    removeIface(IfaceType::NAN, nan_iface_name);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateApNan_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateApP2p_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
    ASSERT_TRUE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       StaMode_CreateStaNan_AfterP2pRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto p2p_iface_name = createIface(IfaceType::P2P);
    ASSERT_FALSE(p2p_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::NAN).empty());

    // After removing P2P iface, NAN iface creation should succeed.
    removeIface(IfaceType::P2P, p2p_iface_name);
    ASSERT_FALSE(createIface(IfaceType::NAN).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       StaMode_CreateStaP2p_AfterNanRemove_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    const auto nan_iface_name = createIface(IfaceType::NAN);
    ASSERT_FALSE(nan_iface_name.empty());
    ASSERT_TRUE(createIface(IfaceType::P2P).empty());

    // After removing NAN iface, P2P iface creation should succeed.
    removeIface(IfaceType::NAN, nan_iface_name);
    ASSERT_FALSE(createIface(IfaceType::P2P).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       CreateStaAp_EnsureDifferentIfaceNames) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    const auto sta_iface_name = createIface(IfaceType::STA);
    const auto ap_iface_name = createIface(IfaceType::AP);
    ASSERT_FALSE(sta_iface_name.empty());
    ASSERT_FALSE(ap_iface_name.empty());
    ASSERT_NE(sta_iface_name, ap_iface_name);
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, RttControllerFlowStaModeNoSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, RttControllerFlowStaModeWithSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, RttControllerFlow) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, SelectTxScenarioWithOnlySta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    EXPECT_CALL(*legacy_hal_, selectTxPowerScenario("wlan0", testing::_))
        .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    chip_->selectTxPowerScenario_1_2(
        V1_2::IWifiChip::TxPowerScenario::ON_HEAD_CELL_OFF,
        [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, SelectTxScenarioWithOnlyAp) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_EQ(createIface(IfaceType::AP), "wlan1");
    EXPECT_CALL(*legacy_hal_, selectTxPowerScenario("wlan1", testing::_))
        .WillOnce(testing::Return(legacy_hal::WIFI_SUCCESS));
    chip_->selectTxPowerScenario_1_2(
        V1_2::IWifiChip::TxPowerScenario::ON_HEAD_CELL_OFF,
        [](const WifiStatus& status) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
        });
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       InvalidateAndRemoveNanOnStaRemove) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");

    // Create NAN iface
    ASSERT_EQ(createIface(IfaceType::NAN), "wlan0");

    // We should have 1 nan iface.
    chip_->getNanIfaceNames(
        [](const WifiStatus& status, const hidl_vec<hidl_string>& iface_names) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(iface_names.size(), 1u);
            ASSERT_EQ(iface_names[0], "wlan0");
        });
    // Retrieve the exact iface object.
    sp<android::hardware::wifi::V1_0::IWifiNanIface> nan_iface;
    chip_->getNanIface(
        "wlan0",
        [&nan_iface](
            const WifiStatus& status,
            const sp<android::hardware::wifi::V1_0::IWifiNanIface>& iface) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_NE(iface.get(), nullptr);
            nan_iface = iface;
        });

    // Remove the STA iface.
    removeIface(IfaceType::STA, "wlan0");
    // We should have 0 nan iface now.
    chip_->getNanIfaceNames(
        [](const WifiStatus& status, const hidl_vec<hidl_string>& iface_names) {
            ASSERT_EQ(WifiStatusCode::SUCCESS, status.code);
            ASSERT_EQ(iface_names.size(), 0u);
        });
    // Any operation on the nan iface object should return error now.
    nan_iface->getName(
        [](const WifiStatus& status, const std::string& /* iface_name */) {
            ASSERT_EQ(WifiStatusCode::ERROR_WIFI_IFACE_INVALID, status.code);
        });
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest,
       InvalidateAndRemoveRttControllerOnStaRemove) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");

    // Create RTT controller
    sp<IWifiRttController> rtt_controller;
    chip_->createRttController_1_4(
        NULL, [&rtt_controller](const WifiStatus& status,
                                const sp<IWifiRttController>& rtt) {
            if (WifiStatusCode::SUCCESS == status.code) {
                ASSERT_NE(rtt.get(), nullptr);
                rtt_controller = rtt;
            }
        });

    // Remove the STA iface.
    removeIface(IfaceType::STA, "wlan0");

    // Any operation on the rtt controller object should return error now.
    rtt_controller->getBoundIface(
        [](const WifiStatus& status, const sp<IWifiIface>& /* iface */) {
            ASSERT_EQ(WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                      status.code);
        });
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateNanWithSharedNanIface) {
    property_set("wifi.aware.interface", nullptr);
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    ASSERT_EQ(createIface(IfaceType::NAN), "wlan0");
    removeIface(IfaceType::NAN, "wlan0");
    EXPECT_CALL(*iface_util_, setUpState(testing::_, testing::_)).Times(0);
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateNanWithDedicatedNanIface) {
    property_set("wifi.aware.interface", "aware0");
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    EXPECT_CALL(*iface_util_, setUpState("aware0", true))
        .WillOnce(testing::Return(true));
    ASSERT_EQ(createIface(IfaceType::NAN), "aware0");

    EXPECT_CALL(*iface_util_, setUpState("aware0", false))
        .WillOnce(testing::Return(true));
    removeIface(IfaceType::NAN, "aware0");
}

////////// V1 Iface Combinations when AP creation is disabled //////////
class WifiChipV1_AwareDisabledApIfaceCombinationTest : public WifiChipTest {
   public:
    void SetUp() override {
        setupV1_AwareDisabledApIfaceCombination();
        WifiChipTest::SetUp();
    }
};

TEST_F(WifiChipV1_AwareDisabledApIfaceCombinationTest,
       StaMode_CreateSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createIface(IfaceType::AP).empty());
}

////////// V2 Iface Combinations when AP creation is disabled //////////
class WifiChipV2_AwareDisabledApIfaceCombinationTest : public WifiChipTest {
   public:
    void SetUp() override {
        setupV2_AwareDisabledApIfaceCombination();
        WifiChipTest::SetUp();
    }
};

TEST_F(WifiChipV2_AwareDisabledApIfaceCombinationTest,
       CreateSta_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createIface(IfaceType::AP).empty());
}

////////// Hypothetical Iface Combination with multiple ifaces //////////
class WifiChip_MultiIfaceTest : public WifiChipTest {
   public:
    void SetUp() override {
        setup_MultiIfaceCombination();
        WifiChipTest::SetUp();
    }
};

TEST_F(WifiChip_MultiIfaceTest, Create3Sta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChip_MultiIfaceTest, CreateStaWithDefaultNames) {
    property_set("wifi.interface.0", "");
    property_set("wifi.interface.1", "");
    property_set("wifi.interface.2", "");
    property_set("wifi.interface", "");
    property_set("wifi.concurrent.interface", "");
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    ASSERT_EQ(createIface(IfaceType::STA), "wlan1");
    ASSERT_EQ(createIface(IfaceType::STA), "wlan2");
}

TEST_F(WifiChip_MultiIfaceTest, CreateStaWithCustomNames) {
    property_set("wifi.interface.0", "test0");
    property_set("wifi.interface.1", "test1");
    property_set("wifi.interface.2", "test2");
    property_set("wifi.interface", "bad0");
    property_set("wifi.concurrent.interface", "bad1");
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "bad0");
    ASSERT_EQ(createIface(IfaceType::STA), "bad1");
    ASSERT_EQ(createIface(IfaceType::STA), "test2");
}

TEST_F(WifiChip_MultiIfaceTest, CreateStaWithCustomAltNames) {
    property_set("wifi.interface.0", "");
    property_set("wifi.interface.1", "");
    property_set("wifi.interface.2", "");
    property_set("wifi.interface", "testA0");
    property_set("wifi.concurrent.interface", "testA1");
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_EQ(createIface(IfaceType::STA), "testA0");
    ASSERT_EQ(createIface(IfaceType::STA), "testA1");
    ASSERT_EQ(createIface(IfaceType::STA), "wlan2");
}

TEST_F(WifiChip_MultiIfaceTest, CreateApStartsWithIdx1) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    // First AP will be slotted to wlan1.
    ASSERT_EQ(createIface(IfaceType::AP), "wlan1");
    // First STA will be slotted to wlan0.
    ASSERT_EQ(createIface(IfaceType::STA), "wlan0");
    // All further STA will be slotted to the remaining free indices.
    ASSERT_EQ(createIface(IfaceType::STA), "wlan2");
    ASSERT_EQ(createIface(IfaceType::STA), "wlan3");
}
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android
