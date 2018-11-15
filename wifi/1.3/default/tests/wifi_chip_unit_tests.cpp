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
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "wifi_chip.h"

#include "mock_wifi_feature_flags.h"
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
namespace V1_3 {
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
            chip_->createApIface([&iface_name](const WifiStatus& status,
                                               const sp<IWifiApIface>& iface) {
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
        chip_->createRttController(
            NULL, [&success](const WifiStatus& status,
                             const sp<IWifiRttController>& rtt) {
                if (WifiStatusCode::SUCCESS == status.code) {
                    ASSERT_NE(rtt.get(), nullptr);
                    success = true;
                }
            });
        return success;
    }

   public:
    void SetUp() override {
        chip_ = new WifiChip(chip_id_, legacy_hal_, mode_controller_,
                             feature_flags_);

        EXPECT_CALL(*mode_controller_, changeFirmwareMode(testing::_))
            .WillRepeatedly(testing::Return(true));
        EXPECT_CALL(*legacy_hal_, start())
            .WillRepeatedly(testing::Return(legacy_hal::WIFI_SUCCESS));
    }

   private:
    sp<WifiChip> chip_;
    ChipId chip_id_ = kFakeChipId;
    std::shared_ptr<NiceMock<legacy_hal::MockWifiLegacyHal>> legacy_hal_{
        new NiceMock<legacy_hal::MockWifiLegacyHal>};
    std::shared_ptr<NiceMock<mode_controller::MockWifiModeController>>
        mode_controller_{new NiceMock<mode_controller::MockWifiModeController>};
    std::shared_ptr<NiceMock<feature_flags::MockWifiFeatureFlags>>
        feature_flags_{new NiceMock<feature_flags::MockWifiFeatureFlags>};
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
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
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
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
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
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
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
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
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

TEST_F(WifiChipV1IfaceCombinationTest, RttControllerFlowStaModeNoSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV1IfaceCombinationTest, RttControllerFlowStaModeWithSta) {
    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createRttController());
}

TEST_F(WifiChipV1IfaceCombinationTest, RttControllerFlowApToSta) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    const auto ap_iface_name = createIface(IfaceType::AP);
    ASSERT_FALSE(ap_iface_name.empty());
    ASSERT_FALSE(createRttController());

    removeIface(IfaceType::AP, ap_iface_name);

    findModeAndConfigureForIfaceType(IfaceType::STA);
    ASSERT_TRUE(createRttController());
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
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
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
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaSta_ShouldFail) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
    ASSERT_TRUE(createIface(IfaceType::STA).empty());
}

TEST_F(WifiChipV2_AwareIfaceCombinationTest, CreateStaAp_ShouldSucceed) {
    findModeAndConfigureForIfaceType(IfaceType::AP);
    ASSERT_FALSE(createIface(IfaceType::AP).empty());
    ASSERT_FALSE(createIface(IfaceType::STA).empty());
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

}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
