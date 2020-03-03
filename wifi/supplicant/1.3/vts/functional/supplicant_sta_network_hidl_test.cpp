/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <VtsCoreUtil.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.1/IWifi.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.3/ISupplicantStaNetwork.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "supplicant_hidl_test_utils.h"
#include "supplicant_hidl_test_utils_1_3.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_3::OcspType;
namespace {
constexpr OcspType kTestOcspType = OcspType::REQUEST_CERT_STATUS;
constexpr OcspType kTestInvalidOcspType = (OcspType)-1;
}  // namespace

class SupplicantStaNetworkHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        wifi_v1_0_instance_name_ = std::get<0>(GetParam());
        supplicant_v1_3_instance_name_ = std::get<1>(GetParam());
        isP2pOn_ =
            testing::deviceSupportsFeature("android.hardware.wifi.direct");
        startSupplicantAndWaitForHidlService(wifi_v1_0_instance_name_,
                                             supplicant_v1_3_instance_name_);
        supplicant_ =
            getSupplicant_1_3(supplicant_v1_3_instance_name_, isP2pOn_);
        EXPECT_TRUE(turnOnExcessiveLogging(supplicant_));
        sta_iface_ = getSupplicantStaIface_1_3(supplicant_);
        ASSERT_NE(nullptr, sta_iface_.get());
        sta_network_ = createSupplicantStaNetwork_1_3(supplicant_);
        ASSERT_NE(sta_network_.get(), nullptr);
    }

    virtual void TearDown() override {
        stopSupplicant(wifi_v1_0_instance_name_);
    }

   protected:
    sp<ISupplicantStaIface> sta_iface_;
    // ISupplicantStaNetwork object used for all tests in this fixture.
    sp<ISupplicantStaNetwork> sta_network_;
    sp<ISupplicant> supplicant_;
    bool isP2pOn_ = false;
    std::string wifi_v1_0_instance_name_;
    std::string supplicant_v1_3_instance_name_;

    bool isWapiSupported() {
        uint32_t keyMgmtMask = 0;

        // We need to first get the key management capabilities from the device.
        // If WAPI is not supported, we just pass the test.
        sta_iface_->getKeyMgmtCapabilities_1_3(
            [&](const SupplicantStatus &status, uint32_t keyMgmtMaskInternal) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
                keyMgmtMask = keyMgmtMaskInternal;
            });

        if (!(keyMgmtMask & ISupplicantStaNetwork::KeyMgmtMask::WAPI_PSK)) {
            // WAPI not supported
            return false;
        }

        return true;
    }
};

/*
 * SetGetOcsp
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetOcsp) {
    OcspType testOcspType = kTestOcspType;

    sta_network_->setOcsp(testOcspType, [](const SupplicantStatus &status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->setOcsp(
        kTestInvalidOcspType, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::FAILURE_ARGS_INVALID, status.code);
        });

    sta_network_->getOcsp(
        [testOcspType](const SupplicantStatus &status, OcspType ocspType) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(testOcspType, ocspType);
        });
}

/*
 * SetPmkCacheEntry
 */
TEST_P(SupplicantStaNetworkHidlTest, SetPmkCache) {
    uint8_t bytes[128] = {0};
    std::vector<uint8_t> serializedEntry(bytes, bytes + sizeof(bytes));

    sta_network_->setPmkCache(
        serializedEntry, [](const SupplicantStatus &status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetGetKeyMgmt_1_3, check new WAPI proto support
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetKeyMgmt_1_3) {
    uint32_t keyMgmt = (uint32_t)ISupplicantStaNetwork::KeyMgmtMask::WAPI_PSK;

    sta_network_->setKeyMgmt_1_3(keyMgmt, [](const SupplicantStatus &status) {
        if (SupplicantStatusCode::SUCCESS != status.code) {
            // for unsupport case
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        }
    });

    sta_network_->getKeyMgmt_1_3(
        [&keyMgmt](const SupplicantStatus &status, uint32_t keyMgmtOut) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            } else {
                EXPECT_EQ(keyMgmtOut, keyMgmt);
            }
        });

    keyMgmt = (uint32_t)ISupplicantStaNetwork::KeyMgmtMask::WAPI_CERT;
    sta_network_->setKeyMgmt_1_3(keyMgmt, [](const SupplicantStatus &status) {
        if (SupplicantStatusCode::SUCCESS != status.code) {
            // for unsupport case
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        }
    });

    sta_network_->getKeyMgmt_1_3(
        [&keyMgmt](const SupplicantStatus &status, uint32_t keyMgmtOut) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            } else {
                EXPECT_EQ(keyMgmtOut, keyMgmt);
            }
        });
}

/*
 * SetGetProto_1_3, check new WAPI proto support
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetProto_1_3) {
    uint32_t wapiProto = (uint32_t)ISupplicantStaNetwork::ProtoMask::WAPI;
    sta_network_->setProto(wapiProto, [](const SupplicantStatus &status) {
        if (SupplicantStatusCode::SUCCESS != status.code) {
            // for unsupport case
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        }
    });
    sta_network_->getProto([&](const SupplicantStatus &status, uint32_t proto) {
        if (SupplicantStatusCode::SUCCESS != status.code) {
            // for unsupport case
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        } else {
            EXPECT_EQ(proto, wapiProto);
        }
    });
}

/*
 * SetGetGroupCipher_1_3, check new WAPI support
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetGroupCipher_1_3) {
    uint32_t groupCipher =
        (uint32_t)ISupplicantStaNetwork::GroupCipherMask::SMS4;

    sta_network_->setGroupCipher_1_3(
        groupCipher, [](const SupplicantStatus &status) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            }
        });

    sta_network_->getGroupCipher_1_3(
        [&groupCipher](const SupplicantStatus &status,
                       uint32_t groupCipherOut) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            } else {
                EXPECT_EQ(groupCipherOut, groupCipher);
            }
        });
}

/*
 * SetGetPairwiseCipher_1_3, check new WAPI support
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetPairwiseCipher_1_3) {
    uint32_t pairwiseCipher =
        (uint32_t)ISupplicantStaNetwork::PairwiseCipherMask::SMS4;

    sta_network_->setPairwiseCipher_1_3(
        pairwiseCipher, [](const SupplicantStatus &status) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            }
        });

    sta_network_->getPairwiseCipher_1_3(
        [&pairwiseCipher](const SupplicantStatus &status,
                          uint32_t pairwiseCipherOut) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            } else {
                EXPECT_EQ(pairwiseCipherOut, pairwiseCipher);
            }
        });
}

/*
 * SetGetWapiCertSuite
 */
TEST_P(SupplicantStaNetworkHidlTest, SetGetWapiCertSuite) {
    hidl_string testWapiCertSuite = "suite";

    if (isWapiSupported()) {
        sta_network_->setWapiCertSuite(
            testWapiCertSuite, [](const SupplicantStatus &status) {
                if (SupplicantStatusCode::SUCCESS != status.code) {
                    // for unsupport case
                    EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN,
                              status.code);
                }
            });

        sta_network_->getWapiCertSuite([testWapiCertSuite](
                                           const SupplicantStatus &status,
                                           const hidl_string &wapiCertSuite) {
            if (SupplicantStatusCode::SUCCESS != status.code) {
                // for unsupport case
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            } else {
                EXPECT_EQ(testWapiCertSuite, wapiCertSuite);
            }
        });
    } else {
        sta_network_->setWapiCertSuite(
            testWapiCertSuite, [](const SupplicantStatus &status) {
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            });

        sta_network_->getWapiCertSuite(
            [testWapiCertSuite](const SupplicantStatus &status,
                                const hidl_string &wapiCertSuite __unused) {
                EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
            });
    }
}
/*
 * SetEapErp
 */
TEST_P(SupplicantStaNetworkHidlTest, SetEapErp) {
    if (!isFilsSupported(sta_iface_)) {
        GTEST_SKIP()
            << "Skipping test since driver/supplicant doesn't support FILS";
    }

    sta_network_->setEapErp(true, [](const SupplicantStatus &status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}
INSTANTIATE_TEST_CASE_P(
    PerInstance, SupplicantStaNetworkHidlTest,
    testing::Combine(
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::V1_0::IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::supplicant::V1_3::ISupplicant::
                descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
