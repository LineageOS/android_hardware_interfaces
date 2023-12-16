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

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicant.h>
#include <aidl/android/hardware/wifi/supplicant/BnSupplicantStaNetworkCallback.h>
#include <aidl/android/hardware/wifi/supplicant/TlsVersion.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "supplicant_test_utils.h"

using aidl::android::hardware::wifi::supplicant::AuthAlgMask;
using aidl::android::hardware::wifi::supplicant::BnSupplicantStaNetworkCallback;
using aidl::android::hardware::wifi::supplicant::DebugLevel;
using aidl::android::hardware::wifi::supplicant::EapMethod;
using aidl::android::hardware::wifi::supplicant::EapPhase2Method;
using aidl::android::hardware::wifi::supplicant::GroupCipherMask;
using aidl::android::hardware::wifi::supplicant::GroupMgmtCipherMask;
using aidl::android::hardware::wifi::supplicant::IfaceType;
using aidl::android::hardware::wifi::supplicant::ISupplicant;
using aidl::android::hardware::wifi::supplicant::ISupplicantStaIface;
using aidl::android::hardware::wifi::supplicant::ISupplicantStaNetwork;
using aidl::android::hardware::wifi::supplicant::KeyMgmtMask;
using aidl::android::hardware::wifi::supplicant::
    NetworkRequestEapSimGsmAuthParams;
using aidl::android::hardware::wifi::supplicant::
    NetworkRequestEapSimUmtsAuthParams;
using aidl::android::hardware::wifi::supplicant::
    NetworkResponseEapSimGsmAuthParams;
using aidl::android::hardware::wifi::supplicant::
    NetworkResponseEapSimUmtsAuthParams;
using aidl::android::hardware::wifi::supplicant::OcspType;
using aidl::android::hardware::wifi::supplicant::PairwiseCipherMask;
using aidl::android::hardware::wifi::supplicant::ProtoMask;
using aidl::android::hardware::wifi::supplicant::SaeH2eMode;
using aidl::android::hardware::wifi::supplicant::TlsVersion;
using aidl::android::hardware::wifi::supplicant::TransitionDisableIndication;
using aidl::android::hardware::wifi::supplicant::WpaDriverCapabilitiesMask;
using android::ProcessState;

namespace {
const std::vector<uint8_t> kTestIdentity = {0x45, 0x67, 0x98, 0x67, 0x56};
const std::vector<uint8_t> kTestEncryptedIdentity = {0x35, 0x37, 0x58, 0x57,
                                                     0x26};
const std::string kTestSsidStr = "TestSsid1234";
const std::vector<uint8_t> kTestSsid =
    std::vector<uint8_t>(kTestSsidStr.begin(), kTestSsidStr.end());
const std::vector<uint8_t> kTestBssid = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
const std::string kTestPskPassphrase =
    "\"123456780abcdef0123456780abcdef0deadbeef\"";
const std::string kTestEapCert = "keystore://CERT";
const std::string kTestEapMatch = "match";
const KeyMgmtMask kTestKeyMgmt =
    static_cast<KeyMgmtMask>(static_cast<uint32_t>(KeyMgmtMask::WPA_PSK) |
                             static_cast<uint32_t>(KeyMgmtMask::WPA_EAP));

}  // namespace

class SupplicantStaNetworkCallback : public BnSupplicantStaNetworkCallback {
   public:
    SupplicantStaNetworkCallback() = default;

    ::ndk::ScopedAStatus onNetworkEapIdentityRequest() override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onNetworkEapSimGsmAuthRequest(
        const NetworkRequestEapSimGsmAuthParams& /* params */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onNetworkEapSimUmtsAuthRequest(
        const NetworkRequestEapSimUmtsAuthParams& /* params */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTransitionDisable(
        TransitionDisableIndication /* ind */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onServerCertificateAvailable(
            int32_t /* depth */, const std::vector<uint8_t>& /* subject */,
            const std::vector<uint8_t>& /* certHash */,
            const std::vector<uint8_t>& /* certBlob */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onPermanentIdReqDenied() override { return ndk::ScopedAStatus::ok(); }
};

class SupplicantStaNetworkAidlTest
    : public testing::TestWithParam<std::string> {
   public:
    void SetUp() override {
        initializeService();
        supplicant_ = getSupplicant(GetParam().c_str());
        ASSERT_NE(supplicant_, nullptr);
        ASSERT_TRUE(supplicant_
                        ->setDebugParams(DebugLevel::EXCESSIVE,
                                         true,  // show timestamps
                                         true)
                        .isOk());
        EXPECT_TRUE(supplicant_->getStaInterface(getStaIfaceName(), &sta_iface_)
                        .isOk());
        ASSERT_NE(sta_iface_, nullptr);
        EXPECT_TRUE(sta_iface_->addNetwork(&sta_network_).isOk());
        ASSERT_NE(sta_network_, nullptr);
    }

    void TearDown() override {
        stopSupplicantService();
        startWifiFramework();
    }

   protected:
    std::shared_ptr<ISupplicant> supplicant_;
    std::shared_ptr<ISupplicantStaIface> sta_iface_;
    std::shared_ptr<ISupplicantStaNetwork> sta_network_;

    void removeNetwork() {
        ASSERT_NE(sta_iface_, nullptr);
        int32_t net_id;
        EXPECT_TRUE(sta_network_->getId(&net_id).isOk());
        EXPECT_TRUE(sta_iface_->removeNetwork(net_id).isOk());
    }
};

/*
 * RegisterCallback
 */
TEST_P(SupplicantStaNetworkAidlTest, RegisterCallback) {
    std::shared_ptr<SupplicantStaNetworkCallback> callback =
        ndk::SharedRefBase::make<SupplicantStaNetworkCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(sta_network_->registerCallback(callback).isOk());
}

/*
 * GetInterfaceName
 */
TEST_P(SupplicantStaNetworkAidlTest, GetInterfaceName) {
    std::string name;
    EXPECT_TRUE(sta_network_->getInterfaceName(&name).isOk());
    EXPECT_NE(name.size(), 0);
}

/*
 * GetType
 */
TEST_P(SupplicantStaNetworkAidlTest, GetType) {
    IfaceType type;
    EXPECT_TRUE(sta_network_->getType(&type).isOk());
    EXPECT_EQ(type, IfaceType::STA);
}

/*
 * Set/Get ScanSsid
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetScanSsid) {
    bool scanSsid = false;
    EXPECT_TRUE(sta_network_->setScanSsid(true).isOk());
    EXPECT_TRUE(sta_network_->getScanSsid(&scanSsid).isOk());
    EXPECT_TRUE(scanSsid);
}

/*
 * Set/Get RequirePmf
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetRequirePmf) {
    bool requirePmf = false;
    EXPECT_TRUE(sta_network_->setRequirePmf(true).isOk());
    EXPECT_TRUE(sta_network_->getRequirePmf(&requirePmf).isOk());
    EXPECT_TRUE(requirePmf);
}

/*
 * Set/Get IdStr
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetIdStr) {
    const std::string savedIdStr = "TestIdstr";
    EXPECT_TRUE(sta_network_->setIdStr(savedIdStr).isOk());

    std::string retrievedIdStr;
    EXPECT_TRUE(sta_network_->getIdStr(&retrievedIdStr).isOk());
    EXPECT_EQ(retrievedIdStr, savedIdStr);
}

/*
 * Set/Get EapMethod
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapMethod) {
    const EapMethod savedMethod = EapMethod::PEAP;
    EXPECT_TRUE(sta_network_->setEapMethod(savedMethod).isOk());

    EapMethod retrievedMethod;
    EXPECT_TRUE(sta_network_->getEapMethod(&retrievedMethod).isOk());
    EXPECT_EQ(retrievedMethod, savedMethod);
}

/*
 * Set/Get EapPhase2Method
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapPhase2Method) {
    const EapMethod savedEapMethod = EapMethod::PEAP;
    EXPECT_TRUE(sta_network_->setEapMethod(savedEapMethod).isOk());

    const EapPhase2Method savedPhase2Method = EapPhase2Method::NONE;
    EXPECT_TRUE(sta_network_->setEapPhase2Method(savedPhase2Method).isOk());

    EapPhase2Method retrievedMethod;
    EXPECT_TRUE(sta_network_->getEapPhase2Method(&retrievedMethod).isOk());
    EXPECT_EQ(retrievedMethod, savedPhase2Method);
}

/*
 * Set/Get EapIdentity
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapIdentity) {
    EXPECT_TRUE(sta_network_->setEapIdentity(kTestIdentity).isOk());

    std::vector<uint8_t> retrievedIdentity;
    EXPECT_TRUE(sta_network_->getEapIdentity(&retrievedIdentity).isOk());
    EXPECT_EQ(retrievedIdentity, kTestIdentity);
}

/*
 * Set/Get EapAnonymousIdentity
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapAnonymousIdentity) {
    EXPECT_TRUE(sta_network_->setEapAnonymousIdentity(kTestIdentity).isOk());

    std::vector<uint8_t> retrievedIdentity;
    EXPECT_TRUE(
        sta_network_->getEapAnonymousIdentity(&retrievedIdentity).isOk());
    EXPECT_EQ(retrievedIdentity, kTestIdentity);
}

/*
 * Set/Get EapPassword
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapPassword) {
    const std::string eapPasswdStr = "TestEapPasswd1234";
    const std::vector<uint8_t> savedEapPasswd =
        std::vector<uint8_t>(eapPasswdStr.begin(), eapPasswdStr.end());
    ASSERT_TRUE(sta_network_->setEapPassword(savedEapPasswd).isOk());

    std::vector<uint8_t> retrievedEapPasswd;
    ASSERT_TRUE(sta_network_->getEapPassword(&retrievedEapPasswd).isOk());
    ASSERT_EQ(retrievedEapPasswd, savedEapPasswd);
}

/*
 * Set/Get EapCACert
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapCACert) {
    EXPECT_TRUE(sta_network_->setEapCACert(kTestEapCert).isOk());

    std::string retrievedCert;
    EXPECT_TRUE(sta_network_->getEapCACert(&retrievedCert).isOk());
    EXPECT_EQ(retrievedCert, kTestEapCert);
}

/*
 * Set/Get EapCAPath
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapCAPath) {
    EXPECT_TRUE(sta_network_->setEapCAPath(kTestEapCert).isOk());

    std::string retrievedCert;
    EXPECT_TRUE(sta_network_->getEapCAPath(&retrievedCert).isOk());
    EXPECT_EQ(retrievedCert, kTestEapCert);
}

/*
 * Set/Get EapClientCert
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapClientCert) {
    EXPECT_TRUE(sta_network_->setEapClientCert(kTestEapCert).isOk());

    std::string retrievedCert;
    EXPECT_TRUE(sta_network_->getEapClientCert(&retrievedCert).isOk());
    EXPECT_EQ(retrievedCert, kTestEapCert);
}

/*
 * Set/Get EapPrivateKeyId
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapPrivateKeyId) {
    std::string savedKeyId = "key_id";
    EXPECT_TRUE(sta_network_->setEapPrivateKeyId(savedKeyId).isOk());

    std::string retrievedKeyId;
    EXPECT_TRUE(sta_network_->getEapPrivateKeyId(&retrievedKeyId).isOk());
    EXPECT_EQ(retrievedKeyId, savedKeyId);
}

/*
 * Set/Get EapAltSubjectMatch
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapAltSubjectMatch) {
    EXPECT_TRUE(sta_network_->setEapAltSubjectMatch(kTestEapMatch).isOk());

    std::string retrievedMatch;
    EXPECT_TRUE(sta_network_->getEapAltSubjectMatch(&retrievedMatch).isOk());
    EXPECT_EQ(retrievedMatch, kTestEapMatch);
}

/*
 * Set/Get EapSubjectMatch
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapSubjectMatch) {
    EXPECT_TRUE(sta_network_->setEapSubjectMatch(kTestEapMatch).isOk());

    std::string retrievedMatch;
    EXPECT_TRUE(sta_network_->getEapSubjectMatch(&retrievedMatch).isOk());
    EXPECT_EQ(retrievedMatch, kTestEapMatch);
}

/*
 * Set/Get EapDomainSuffixMatch
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapDomainSuffixMatch) {
    EXPECT_TRUE(sta_network_->setEapDomainSuffixMatch(kTestEapMatch).isOk());

    std::string retrievedMatch;
    EXPECT_TRUE(sta_network_->getEapDomainSuffixMatch(&retrievedMatch).isOk());
    EXPECT_EQ(retrievedMatch, kTestEapMatch);
}

/*
 * Set/Get EapEngine
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapEngine) {
    bool retrievedEapEngine = false;
    EXPECT_TRUE(sta_network_->setEapEngine(true).isOk());
    EXPECT_TRUE(sta_network_->getEapEngine(&retrievedEapEngine).isOk());
    EXPECT_TRUE(retrievedEapEngine);
}

/*
 * Set/Get EapEngineID
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetEapEngineId) {
    const std::string savedEngineId = "engine_id";
    EXPECT_TRUE(sta_network_->setEapEngineID(savedEngineId).isOk());

    std::string retrievedId;
    EXPECT_TRUE(sta_network_->getEapEngineId(&retrievedId).isOk());
    EXPECT_EQ(retrievedId, savedEngineId);
}

/*
 * Set/Get Ocsp
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetOcsp) {
    const OcspType savedOcspType = OcspType::REQUEST_CERT_STATUS;
    EXPECT_TRUE(sta_network_->setOcsp(savedOcspType).isOk());

    const OcspType invalidOcspType = static_cast<OcspType>(-1);
    EXPECT_FALSE(sta_network_->setOcsp(invalidOcspType).isOk());

    OcspType retrievedOcspType;
    EXPECT_TRUE(sta_network_->getOcsp(&retrievedOcspType).isOk());
    EXPECT_EQ(retrievedOcspType, savedOcspType);
}

/*
 * Set/Get KeyMgmt
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetKeyMgmt) {
    KeyMgmtMask savedKeyMgmt = KeyMgmtMask::WAPI_PSK;
    EXPECT_TRUE(sta_network_->setKeyMgmt(savedKeyMgmt).isOk());

    KeyMgmtMask retrievedKeyMgmt;
    EXPECT_TRUE(sta_network_->getKeyMgmt(&retrievedKeyMgmt).isOk());
    EXPECT_EQ(retrievedKeyMgmt, savedKeyMgmt);

    savedKeyMgmt = KeyMgmtMask::WAPI_CERT;
    EXPECT_TRUE(sta_network_->setKeyMgmt(savedKeyMgmt).isOk());

    EXPECT_TRUE(sta_network_->getKeyMgmt(&retrievedKeyMgmt).isOk());
    EXPECT_EQ(retrievedKeyMgmt, savedKeyMgmt);
}

/*
 * Set/Get Proto
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetProto) {
    const ProtoMask savedProto = ProtoMask::WAPI;
    EXPECT_TRUE(sta_network_->setProto(savedProto).isOk());

    ProtoMask retrievedProto;
    EXPECT_TRUE(sta_network_->getProto(&retrievedProto).isOk());
    EXPECT_EQ(retrievedProto, savedProto);
}

/*
 * Set/Get GroupCipher
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetGroupCipher) {
    const GroupCipherMask savedCipher = GroupCipherMask::SMS4;
    EXPECT_TRUE(sta_network_->setGroupCipher(savedCipher).isOk());

    GroupCipherMask retrievedCipher;
    EXPECT_TRUE(sta_network_->getGroupCipher(&retrievedCipher).isOk());
    EXPECT_EQ(retrievedCipher, savedCipher);
}

/*
 * Set/Get PairwiseCipher
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetPairwiseCipher) {
    const PairwiseCipherMask savedCipher = PairwiseCipherMask::SMS4;
    EXPECT_TRUE(sta_network_->setPairwiseCipher(savedCipher).isOk());

    PairwiseCipherMask retrievedCipher;
    EXPECT_TRUE(sta_network_->getPairwiseCipher(&retrievedCipher).isOk());
    EXPECT_EQ(retrievedCipher, savedCipher);
}

/*
 * Set/Get WapiCertSuite
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetWapiCertSuite) {
    if (!keyMgmtSupported(sta_iface_, KeyMgmtMask::WAPI_PSK)) {
        GTEST_SKIP() << "Skipping test since WAPI is not supported.";
    }

    const std::string savedCertSuite = "suite";
    EXPECT_TRUE(sta_network_->setWapiCertSuite(savedCertSuite).isOk());

    std::string retrievedCertSuite;
    EXPECT_TRUE(sta_network_->getWapiCertSuite(&retrievedCertSuite).isOk());
    EXPECT_EQ(retrievedCertSuite, savedCertSuite);
}

/*
 * Set/Get WapiPsk
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetWapiPsk) {
    if (!keyMgmtSupported(sta_iface_, KeyMgmtMask::WAPI_PSK)) {
        GTEST_SKIP() << "Skipping test since WAPI is not supported.";
    }

    EXPECT_TRUE(sta_network_->setKeyMgmt(KeyMgmtMask::WAPI_PSK).isOk());
    EXPECT_TRUE(sta_network_->setPskPassphrase(kTestPskPassphrase).isOk());

    std::string retrievedPassphrase;
    EXPECT_TRUE(sta_network_->getPskPassphrase(&retrievedPassphrase).isOk());
    EXPECT_EQ(retrievedPassphrase, kTestPskPassphrase);

    const std::string pskHex = "12345678";
    EXPECT_TRUE(sta_network_->setPskPassphrase(pskHex).isOk());

    EXPECT_TRUE(sta_network_->getPskPassphrase(&retrievedPassphrase).isOk());
    EXPECT_EQ(retrievedPassphrase, pskHex);
}

/*
 * Set/Get SaePassword
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetSaePassword) {
    const std::string savedPassword = "topsecret";
    EXPECT_TRUE(sta_network_->setSaePassword(savedPassword).isOk());

    std::string retrievedPassword;
    EXPECT_TRUE(sta_network_->getSaePassword(&retrievedPassword).isOk());
    EXPECT_EQ(retrievedPassword, savedPassword);
}

/*
 * Set/Get SaePasswordId
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetSaePasswordId) {
    const std::string savedPasswdId = "id1";
    EXPECT_TRUE(sta_network_->setSaePasswordId(savedPasswdId).isOk());

    std::string retrievedPasswdId;
    EXPECT_TRUE(sta_network_->getSaePasswordId(&retrievedPasswdId).isOk());
    EXPECT_EQ(retrievedPasswdId, savedPasswdId);
}

/*
 * Set/Get GroupMgmtCipher
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetGroupMgmtCipher) {
    const GroupMgmtCipherMask savedCipher = GroupMgmtCipherMask::BIP_GMAC_256;
    EXPECT_TRUE(sta_network_->setGroupMgmtCipher(savedCipher).isOk());

    GroupMgmtCipherMask retrievedCipher;
    EXPECT_TRUE(sta_network_->getGroupMgmtCipher(&retrievedCipher).isOk());
    EXPECT_EQ(retrievedCipher, savedCipher);
}

/*
 * Set/Get Ssid
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetSsid) {
    EXPECT_TRUE(sta_network_->setSsid(kTestSsid).isOk());

    std::vector<uint8_t> retrievedSsid;
    EXPECT_TRUE(sta_network_->getSsid(&retrievedSsid).isOk());
    EXPECT_EQ(retrievedSsid, kTestSsid);
}

/*
 * Set/Get Bssid
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetBssid) {
    EXPECT_TRUE(sta_network_->setBssid(kTestBssid).isOk());

    std::vector<uint8_t> retrievedBssid;
    EXPECT_TRUE(sta_network_->getBssid(&retrievedBssid).isOk());
    EXPECT_EQ(retrievedBssid, kTestBssid);
}

/*
 * Set/Get KeyAuthAlg
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetAuthAlg) {
    const AuthAlgMask savedAlg =
        static_cast<AuthAlgMask>(static_cast<uint32_t>(AuthAlgMask::OPEN) |
                                 static_cast<uint32_t>(AuthAlgMask::SHARED));
    EXPECT_TRUE(sta_network_->setAuthAlg(savedAlg).isOk());

    AuthAlgMask retrievedAlg;
    EXPECT_TRUE(sta_network_->getAuthAlg(&retrievedAlg).isOk());
    EXPECT_EQ(retrievedAlg, savedAlg);
}

/*
 * Set/Get WepTxKeyIdx
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetWepTxKeyIdx) {
    const int32_t savedKeyIdx = 2;
    EXPECT_TRUE(sta_network_->setWepTxKeyIdx(savedKeyIdx).isOk());

    int32_t retrievedKeyIdx;
    EXPECT_TRUE(sta_network_->getWepTxKeyIdx(&retrievedKeyIdx).isOk());
    EXPECT_EQ(retrievedKeyIdx, savedKeyIdx);
}

/*
 * Set SAE H2E (Hash-to-Element) mode
 */
TEST_P(SupplicantStaNetworkAidlTest, SetSaeH2eMode) {
    EXPECT_TRUE(sta_network_->setSaeH2eMode(SaeH2eMode::DISABLED).isOk());
    EXPECT_TRUE(sta_network_->setSaeH2eMode(SaeH2eMode::H2E_MANDATORY).isOk());
    EXPECT_TRUE(sta_network_->setSaeH2eMode(SaeH2eMode::H2E_OPTIONAL).isOk());
}

/*
 * Set/Get Psk
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetPsk) {
    const std::vector<uint8_t> savedPsk = std::vector<uint8_t>(32, 0x12);
    EXPECT_TRUE(sta_network_->setPsk(savedPsk).isOk());

    std::vector<uint8_t> retrievedPsk;
    EXPECT_TRUE(sta_network_->getPsk(&retrievedPsk).isOk());
    EXPECT_EQ(retrievedPsk, savedPsk);
}

/*
 * Set/Get WepKeys
 */
TEST_P(SupplicantStaNetworkAidlTest, SetGetWepKeys) {
    const uint32_t maxKeys = 4;
    const std::vector<uint8_t> testWepKey = {0x56, 0x67, 0x67, 0xf4, 0x56};

    for (uint32_t i = 0; i < maxKeys; i++) {
        std::vector<uint8_t> retrievedKey;
        EXPECT_TRUE(sta_network_->setWepKey(i, testWepKey).isOk());
        EXPECT_TRUE(sta_network_->getWepKey(i, &retrievedKey).isOk());
        EXPECT_EQ(retrievedKey, testWepKey);
    }
}

/*
 * SetPmkCacheEntry
 */
TEST_P(SupplicantStaNetworkAidlTest, SetPmkCache) {
    const std::vector<uint8_t> serializedEntry(128, 0);
    EXPECT_TRUE(sta_network_->setPmkCache(serializedEntry).isOk());
}

/*
 * SetEapErp
 */
TEST_P(SupplicantStaNetworkAidlTest, SetEapErp) {
    if (!isFilsSupported(sta_iface_)) {
        GTEST_SKIP()
            << "Skipping test since driver/supplicant doesn't support FILS";
    }
    EXPECT_TRUE(sta_network_->setEapErp(true).isOk());
}

/*
 * SetUpdateIdentifier
 */
TEST_P(SupplicantStaNetworkAidlTest, SetUpdateIdentifier) {
    const uint32_t updateIdentifier = 21;
    EXPECT_TRUE(sta_network_->setUpdateIdentifier(updateIdentifier).isOk());
}

/*
 * SetProactiveKeyCaching
 */
TEST_P(SupplicantStaNetworkAidlTest, SetProactiveKeyCaching) {
    EXPECT_TRUE(sta_network_->setProactiveKeyCaching(true).isOk());
    EXPECT_TRUE(sta_network_->setProactiveKeyCaching(false).isOk());
}

/*
 * EnableSuiteBEapOpenSslCiphers
 */
TEST_P(SupplicantStaNetworkAidlTest, EnableSuiteBEapOpenSslCiphers) {
    EXPECT_TRUE(sta_network_->enableSuiteBEapOpenSslCiphers().isOk());
}

/*
 * EnableTlsSuiteBEapPhase1Param
 */
TEST_P(SupplicantStaNetworkAidlTest, EnableTlsSuiteBEapPhase1Param) {
    EXPECT_TRUE(sta_network_->enableTlsSuiteBEapPhase1Param(true).isOk());
    EXPECT_TRUE(sta_network_->enableTlsSuiteBEapPhase1Param(false).isOk());
}

/*
 * SetEapEncryptedImsiIdentity
 */
TEST_P(SupplicantStaNetworkAidlTest, SetEapEncryptedImsiIdentity) {
    EXPECT_TRUE(
        sta_network_->setEapEncryptedImsiIdentity(kTestEncryptedIdentity)
            .isOk());
}

/*
 * SetStrictConservativePeerMode
 */
TEST_P(SupplicantStaNetworkAidlTest, SetStrictConversativePeerMode) {
    int32_t version = 0;
    sta_network_->getInterfaceVersion(&version);
    if (version < 2) {
        GTEST_SKIP() << "Skipping test since it is not supported on this interface version";
    }
    EXPECT_TRUE(sta_network_->setStrictConservativePeerMode(true).isOk());
    EXPECT_TRUE(sta_network_->setStrictConservativePeerMode(false).isOk());
}

/*
 * SendNetworkEapIdentityResponse
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapIdentityResponse) {
    EXPECT_TRUE(sta_network_
                    ->sendNetworkEapIdentityResponse(kTestIdentity,
                                                     kTestEncryptedIdentity)
                    .isOk());
}

/*
 * Enable SAE PK only mode
 */
TEST_P(SupplicantStaNetworkAidlTest, EnableSaePkOnlyMode) {
    // Check for SAE PK support
    WpaDriverCapabilitiesMask caps;
    EXPECT_TRUE(sta_iface_->getWpaDriverCapabilities(&caps).isOk());
    const bool saePkSupported =
        !!(static_cast<uint32_t>(caps) &
           static_cast<uint32_t>(WpaDriverCapabilitiesMask::SAE_PK));
    LOG(INFO) << "SAE-PK Supported: " << saePkSupported;

    // Operation will succeed if SAE PK is supported, or fail otherwise.
    EXPECT_EQ(sta_network_->enableSaePkOnlyMode(true).isOk(), saePkSupported);
    EXPECT_EQ(sta_network_->enableSaePkOnlyMode(false).isOk(), saePkSupported);
}

/*
 * Enable
 */
TEST_P(SupplicantStaNetworkAidlTest, Enable) {
    // wpa_supplicant won't perform any connection initiation
    // unless at least the SSID and key mgmt params are set.
    EXPECT_TRUE(sta_network_->setSsid(kTestSsid).isOk());
    EXPECT_TRUE(sta_network_->setKeyMgmt(kTestKeyMgmt).isOk());

    EXPECT_TRUE(sta_network_->enable(false).isOk());
    EXPECT_TRUE(sta_network_->enable(true).isOk());

    // Now remove the network and ensure that the call fails.
    removeNetwork();
    ASSERT_FALSE(sta_network_->enable(true).isOk());
}

/*
 * Disable
 */
TEST_P(SupplicantStaNetworkAidlTest, Disable) {
    // wpa_supplicant won't perform any connection initiation
    // unless at least the SSID and key mgmt params are set.
    EXPECT_TRUE(sta_network_->setSsid(kTestSsid).isOk());
    EXPECT_TRUE(sta_network_->setKeyMgmt(kTestKeyMgmt).isOk());

    EXPECT_TRUE(sta_network_->disable().isOk());

    // Now remove the network and ensure that the call fails.
    removeNetwork();
    EXPECT_FALSE(sta_network_->disable().isOk());
}

/*
 * Select
 */
TEST_P(SupplicantStaNetworkAidlTest, Select) {
    // wpa_supplicant won't perform any connection initiation
    // unless at least the SSID and key mgmt params are set.
    EXPECT_TRUE(sta_network_->setSsid(kTestSsid).isOk());
    EXPECT_TRUE(sta_network_->setKeyMgmt(kTestKeyMgmt).isOk());

    EXPECT_TRUE(sta_network_->select().isOk());

    // Now remove the network and ensure that the call fails.
    removeNetwork();
    EXPECT_FALSE(sta_network_->select().isOk());
}

/*
 * SendNetworkEapSimGsmAuthResponse
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapSimGsmAuthResponse) {
    NetworkResponseEapSimGsmAuthParams param;
    param.kc =
        std::vector<uint8_t>({0x56, 0x67, 0x67, 0xf4, 0x76, 0x87, 0x98, 0x12});
    param.sres = std::vector<uint8_t>({0x56, 0x67, 0x67, 0xf4});
    const std::vector<NetworkResponseEapSimGsmAuthParams> params = {param};
    EXPECT_TRUE(sta_network_->sendNetworkEapSimGsmAuthResponse(params).isOk());
}

/*
 * SendNetworkEapSimGsmAuthFailure
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapSimGsmAuthFailure) {
    EXPECT_TRUE(sta_network_->sendNetworkEapSimGsmAuthFailure().isOk());
}

/*
 * SendNetworkEapSimUmtsAuthResponse
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapSimUmtsAuthResponse) {
    NetworkResponseEapSimUmtsAuthParams params;
    params.res = std::vector<uint8_t>({0x56, 0x67, 0x67, 0xf4, 0x67});
    params.ik = std::vector<uint8_t>(16, 0x65);
    params.ck = std::vector<uint8_t>(16, 0x45);
    EXPECT_TRUE(sta_network_->sendNetworkEapSimUmtsAuthResponse(params).isOk());
}

/*
 * SendNetworkEapSimUmtsAuthFailure
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapSimUmtsAuthFailure) {
    EXPECT_TRUE(sta_network_->sendNetworkEapSimUmtsAuthFailure().isOk());
}

/*
 * SendNetworkEapSimUmtsAutsResponse
 */
TEST_P(SupplicantStaNetworkAidlTest, SendNetworkEapSimUmtsAutsResponse) {
    const std::vector<uint8_t> testAutParam = std::vector<uint8_t>(14, 0xe1);
    EXPECT_TRUE(
        sta_network_->sendNetworkEapSimUmtsAutsResponse(testAutParam).isOk());
}

/*
 * GetWpsNfcConfigurationToken
 */
TEST_P(SupplicantStaNetworkAidlTest, GetWpsNfcConfigurationToken) {
    EXPECT_TRUE(sta_network_->setSsid(kTestSsid).isOk());
    EXPECT_TRUE(sta_network_->setKeyMgmt(kTestKeyMgmt).isOk());
    EXPECT_TRUE(sta_network_->setPskPassphrase(kTestPskPassphrase).isOk());

    std::vector<uint8_t> retrievedToken;
    EXPECT_TRUE(
        sta_network_->getWpsNfcConfigurationToken(&retrievedToken).isOk());
    EXPECT_NE(retrievedToken.size(), 0);
}

/*
 * SetRoamingConsortiumSelection
 */
TEST_P(SupplicantStaNetworkAidlTest, SetRoamingConsortiumSelection) {
    const std::vector<uint8_t> testSelection = std::vector<uint8_t>({0x11, 0x21, 0x33, 0x44});
    EXPECT_TRUE(sta_network_->setRoamingConsortiumSelection(testSelection).isOk());
}

/*
 * SetMinimumTlsVersionEapPhase1Param
 */
TEST_P(SupplicantStaNetworkAidlTest, SetMinimumTlsVersionEapPhase1Param) {
    WpaDriverCapabilitiesMask caps;
    EXPECT_TRUE(sta_iface_->getWpaDriverCapabilities(&caps).isOk());
    const bool tlsV13Supported = !!(static_cast<uint32_t>(caps) &
                                    static_cast<uint32_t>(WpaDriverCapabilitiesMask::TLS_V1_3));
    LOG(INFO) << "TLS_V1_3 Supported: " << tlsV13Supported;

    // Operation will succeed if TLS_V1_3 is supported, or fail otherwise.
    EXPECT_EQ(sta_network_->setMinimumTlsVersionEapPhase1Param(TlsVersion::TLS_V1_3).isOk(),
              tlsV13Supported);
}

/*
 * disableEht
 */
TEST_P(SupplicantStaNetworkAidlTest, DisableEht) {
    EXPECT_TRUE(sta_network_->disableEht().isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SupplicantStaNetworkAidlTest);
INSTANTIATE_TEST_SUITE_P(Supplicant, SupplicantStaNetworkAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             ISupplicant::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
