/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <memory>
#include <string>
#define LOG_TAG "VtsRemotelyProvisionableComponentTests"

#include <aidl/android/hardware/security/keymint/BnRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <cppbor.h>
#include <cppbor_parse.h>
#include <gmock/gmock.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/keymaster_configuration.h>
#include <keymint_support/authorization_set.h>
#include <openssl/ec.h>
#include <openssl/ec_key.h>
#include <openssl/x509.h>
#include <remote_prov/remote_prov_utils.h>
#include <optional>
#include <set>
#include <vector>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::std::string;
using ::std::vector;

namespace {

constexpr int32_t VERSION_WITH_UNIQUE_ID_SUPPORT = 2;

constexpr int32_t VERSION_WITHOUT_EEK = 3;
constexpr int32_t VERSION_WITHOUT_TEST_MODE = 3;
constexpr int32_t VERSION_WITH_CERTIFICATE_REQUEST_V2 = 3;
constexpr int32_t VERSION_WITH_SUPPORTED_NUM_KEYS_IN_CSR = 3;

constexpr uint8_t MIN_CHALLENGE_SIZE = 0;
constexpr uint8_t MAX_CHALLENGE_SIZE = 64;
const string RKP_VM_INSTANCE_NAME =
        "android.hardware.security.keymint.IRemotelyProvisionedComponent/avf";

#define INSTANTIATE_REM_PROV_AIDL_TEST(name)                                         \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(name);                             \
    INSTANTIATE_TEST_SUITE_P(                                                        \
            PerInstance, name,                                                       \
            testing::ValuesIn(VtsRemotelyProvisionedComponentTests::build_params()), \
            ::android::PrintInstanceNameToString)

using ::android::sp;
using bytevec = std::vector<uint8_t>;
using testing::MatchesRegex;
using namespace remote_prov;
using namespace keymaster;

bytevec string_to_bytevec(const char* s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
    return bytevec(p, p + strlen(s));
}

ErrMsgOr<MacedPublicKey> corrupt_maced_key(const MacedPublicKey& macedPubKey) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    if (!coseMac0 || coseMac0->asArray()->size() != kCoseMac0EntryCount) {
        return "COSE Mac0 parse failed";
    }
    auto protParams = coseMac0->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
    auto unprotParams = coseMac0->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    auto tag = coseMac0->asArray()->get(kCoseMac0Tag)->asBstr();
    if (!protParams || !unprotParams || !payload || !tag) {
        return "Invalid COSE_Sign1: missing content";
    }
    auto corruptMac0 = cppbor::Array();
    corruptMac0.add(protParams->clone());
    corruptMac0.add(unprotParams->clone());
    corruptMac0.add(payload->clone());
    vector<uint8_t> tagData = tag->value();
    tagData[0] ^= 0x08;
    tagData[tagData.size() - 1] ^= 0x80;
    corruptMac0.add(cppbor::Bstr(tagData));

    return MacedPublicKey{corruptMac0.encode()};
}

ErrMsgOr<cppbor::Array> corrupt_sig(const cppbor::Array* coseSign1) {
    if (coseSign1->size() != kCoseSign1EntryCount) {
        return "Invalid COSE_Sign1, wrong entry count";
    }
    const cppbor::Bstr* protectedParams = coseSign1->get(kCoseSign1ProtectedParams)->asBstr();
    const cppbor::Map* unprotectedParams = coseSign1->get(kCoseSign1UnprotectedParams)->asMap();
    const cppbor::Bstr* payload = coseSign1->get(kCoseSign1Payload)->asBstr();
    const cppbor::Bstr* signature = coseSign1->get(kCoseSign1Signature)->asBstr();
    if (!protectedParams || !unprotectedParams || !payload || !signature) {
        return "Invalid COSE_Sign1: missing content";
    }

    auto corruptSig = cppbor::Array();
    corruptSig.add(protectedParams->clone());
    corruptSig.add(unprotectedParams->clone());
    corruptSig.add(payload->clone());
    vector<uint8_t> sigData = signature->value();
    sigData[0] ^= 0x08;
    corruptSig.add(cppbor::Bstr(sigData));

    return std::move(corruptSig);
}

ErrMsgOr<bytevec> corrupt_sig_chain(const bytevec& encodedEekChain, int which) {
    auto [chain, _, parseErr] = cppbor::parse(encodedEekChain);
    if (!chain || !chain->asArray()) {
        return "EekChain parse failed";
    }

    cppbor::Array* eekChain = chain->asArray();
    if (which >= eekChain->size()) {
        return "selected sig out of range";
    }
    auto corruptChain = cppbor::Array();

    for (int ii = 0; ii < eekChain->size(); ++ii) {
        if (ii == which) {
            auto sig = corrupt_sig(eekChain->get(which)->asArray());
            if (!sig) {
                return "Failed to build corrupted signature" + sig.moveMessage();
            }
            corruptChain.add(sig.moveValue());
        } else {
            corruptChain.add(eekChain->get(ii)->clone());
        }
    }
    return corruptChain.encode();
}

string device_suffix(const string& name) {
    size_t pos = name.find('/');
    if (pos == string::npos) {
        return name;
    }
    return name.substr(pos + 1);
}

bool matching_keymint_device(const string& rp_name, std::shared_ptr<IKeyMintDevice>* keyMint) {
    string rp_suffix = device_suffix(rp_name);

    vector<string> km_names = ::android::getAidlHalInstanceNames(IKeyMintDevice::descriptor);
    for (const string& km_name : km_names) {
        // If the suffix of the KeyMint instance equals the suffix of the
        // RemotelyProvisionedComponent instance, assume they match.
        if (device_suffix(km_name) == rp_suffix && AServiceManager_isDeclared(km_name.c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(km_name.c_str()));
            *keyMint = IKeyMintDevice::fromBinder(binder);
            return true;
        }
    }
    return false;
}

}  // namespace

class VtsRemotelyProvisionedComponentTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            provisionable_ = IRemotelyProvisionedComponent::fromBinder(binder);
        }
        ASSERT_NE(provisionable_, nullptr);
        auto status = provisionable_->getHardwareInfo(&rpcHardwareInfo);
        if (GetParam() == RKP_VM_INSTANCE_NAME &&
            status.getExceptionCode() == EX_UNSUPPORTED_OPERATION) {
            GTEST_SKIP() << "The RKP VM is not supported on this system.";
        }
        ASSERT_TRUE(status.isOk());
    }

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IRemotelyProvisionedComponent::descriptor);
        return params;
    }

  protected:
    std::shared_ptr<IRemotelyProvisionedComponent> provisionable_;
    RpcHardwareInfo rpcHardwareInfo;
};

/**
 * Verify that every implementation reports a different unique id.
 */
TEST(NonParameterizedTests, eachRpcHasAUniqueId) {
    std::set<std::string> uniqueIds;
    for (auto hal : ::android::getAidlHalInstanceNames(IRemotelyProvisionedComponent::descriptor)) {
        ASSERT_TRUE(AServiceManager_isDeclared(hal.c_str()));
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(hal.c_str()));
        std::shared_ptr<IRemotelyProvisionedComponent> rpc =
                IRemotelyProvisionedComponent::fromBinder(binder);
        ASSERT_NE(rpc, nullptr);

        RpcHardwareInfo hwInfo;
        auto status = rpc->getHardwareInfo(&hwInfo);
        if (hal == RKP_VM_INSTANCE_NAME && status.getExceptionCode() == EX_UNSUPPORTED_OPERATION) {
            GTEST_SKIP() << "The RKP VM is not supported on this system.";
        }
        ASSERT_TRUE(status.isOk());

        if (hwInfo.versionNumber >= VERSION_WITH_UNIQUE_ID_SUPPORT) {
            ASSERT_TRUE(hwInfo.uniqueId);
            auto [_, wasInserted] = uniqueIds.insert(*hwInfo.uniqueId);
            EXPECT_TRUE(wasInserted);
        } else {
            ASSERT_FALSE(hwInfo.uniqueId);
        }
    }
}

using GetHardwareInfoTests = VtsRemotelyProvisionedComponentTests;

INSTANTIATE_REM_PROV_AIDL_TEST(GetHardwareInfoTests);

/**
 * Verify that a valid curve is reported by the implementation.
 */
TEST_P(GetHardwareInfoTests, supportsValidCurve) {
    RpcHardwareInfo hwInfo;
    ASSERT_TRUE(provisionable_->getHardwareInfo(&hwInfo).isOk());

    if (rpcHardwareInfo.versionNumber >= VERSION_WITHOUT_EEK) {
        ASSERT_EQ(hwInfo.supportedEekCurve, RpcHardwareInfo::CURVE_NONE)
                << "Invalid curve: " << hwInfo.supportedEekCurve;
        return;
    }

    const std::set<int> validCurves = {RpcHardwareInfo::CURVE_P256, RpcHardwareInfo::CURVE_25519};
    ASSERT_EQ(validCurves.count(hwInfo.supportedEekCurve), 1)
            << "Invalid curve: " << hwInfo.supportedEekCurve;
}

/**
 * Verify that the unique id is within the length limits as described in RpcHardwareInfo.aidl.
 */
TEST_P(GetHardwareInfoTests, uniqueId) {
    if (rpcHardwareInfo.versionNumber < VERSION_WITH_UNIQUE_ID_SUPPORT) {
        return;
    }

    RpcHardwareInfo hwInfo;
    ASSERT_TRUE(provisionable_->getHardwareInfo(&hwInfo).isOk());
    ASSERT_TRUE(hwInfo.uniqueId);
    EXPECT_GE(hwInfo.uniqueId->size(), 1);
    EXPECT_LE(hwInfo.uniqueId->size(), 32);
}

/**
 * Verify implementation supports at least MIN_SUPPORTED_NUM_KEYS_IN_CSR keys in a CSR.
 */
TEST_P(GetHardwareInfoTests, supportedNumKeysInCsr) {
    if (rpcHardwareInfo.versionNumber < VERSION_WITH_SUPPORTED_NUM_KEYS_IN_CSR) {
        return;
    }

    RpcHardwareInfo hwInfo;
    ASSERT_TRUE(provisionable_->getHardwareInfo(&hwInfo).isOk());
    ASSERT_GE(hwInfo.supportedNumKeysInCsr, RpcHardwareInfo::MIN_SUPPORTED_NUM_KEYS_IN_CSR);
}

using GenerateKeyTests = VtsRemotelyProvisionedComponentTests;

INSTANTIATE_REM_PROV_AIDL_TEST(GenerateKeyTests);

/**
 * Generate and validate a production-mode key.  MAC tag can't be verified, but
 * the private key blob should be usable in KeyMint operations.
 */
TEST_P(GenerateKeyTests, generateEcdsaP256Key_prodMode) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = false;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    vector<uint8_t> coseKeyData;
    check_maced_pubkey(macedPubKey, testMode, &coseKeyData);
}

/**
 * Generate and validate a production-mode key, then use it as a KeyMint attestation key.
 */
TEST_P(GenerateKeyTests, generateAndUseEcdsaP256Key_prodMode) {
    // See if there is a matching IKeyMintDevice for this IRemotelyProvisionedComponent.
    std::shared_ptr<IKeyMintDevice> keyMint;
    if (!matching_keymint_device(GetParam(), &keyMint)) {
        // No matching IKeyMintDevice.
        GTEST_SKIP() << "Skipping key use test as no matching KeyMint device found";
        return;
    }
    KeyMintHardwareInfo info;
    ASSERT_TRUE(keyMint->getHardwareInfo(&info).isOk());

    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = false;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    vector<uint8_t> coseKeyData;
    check_maced_pubkey(macedPubKey, testMode, &coseKeyData);

    AttestationKey attestKey;
    attestKey.keyBlob = std::move(privateKeyBlob);
    attestKey.issuerSubjectName = make_name_from_str("Android Keystore Key");

    // Generate an ECDSA key that is attested by the generated P256 keypair.
    AuthorizationSet keyDesc = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .EcdsaSigningKey(EcCurve::P_256)
                                       .AttestationChallenge("foo")
                                       .AttestationApplicationId("bar")
                                       .Digest(Digest::NONE)
                                       .SetDefaultValidity();
    KeyCreationResult creationResult;
    auto result = keyMint->generateKey(keyDesc.vector_data(), attestKey, &creationResult);
    ASSERT_TRUE(result.isOk());
    vector<uint8_t> attested_key_blob = std::move(creationResult.keyBlob);
    vector<KeyCharacteristics> attested_key_characteristics =
            std::move(creationResult.keyCharacteristics);
    vector<Certificate> attested_key_cert_chain = std::move(creationResult.certificateChain);
    EXPECT_EQ(attested_key_cert_chain.size(), 1);

    int32_t aidl_version = 0;
    ASSERT_TRUE(keyMint->getInterfaceVersion(&aidl_version).isOk());
    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
    EXPECT_TRUE(verify_attestation_record(aidl_version, "foo", "bar", sw_enforced, hw_enforced,
                                          info.securityLevel,
                                          attested_key_cert_chain[0].encodedCertificate));

    // Attestation by itself is not valid (last entry is not self-signed).
    EXPECT_FALSE(ChainSignaturesAreValid(attested_key_cert_chain));

    // The signature over the attested key should correspond to the P256 public key.
    X509_Ptr key_cert(parse_cert_blob(attested_key_cert_chain[0].encodedCertificate));
    ASSERT_TRUE(key_cert.get());
    EVP_PKEY_Ptr signing_pubkey;
    p256_pub_key(coseKeyData, &signing_pubkey);
    ASSERT_TRUE(signing_pubkey.get());

    ASSERT_TRUE(X509_verify(key_cert.get(), signing_pubkey.get()))
            << "Verification of attested certificate failed "
            << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL);
}

/**
 * Generate and validate a test-mode key.
 */
TEST_P(GenerateKeyTests, generateEcdsaP256Key_testMode) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = true;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);

    if (rpcHardwareInfo.versionNumber >= VERSION_WITHOUT_TEST_MODE) {
        ASSERT_FALSE(status.isOk());
        EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_REMOVED);
        return;
    }

    ASSERT_TRUE(status.isOk());
    check_maced_pubkey(macedPubKey, testMode, nullptr);
}

class CertificateRequestTestBase : public VtsRemotelyProvisionedComponentTests {
  protected:
    CertificateRequestTestBase()
        : eekId_(string_to_bytevec("eekid")), challenge_(randomBytes(64)) {}

    void generateTestEekChain(size_t eekLength) {
        auto chain = generateEekChain(rpcHardwareInfo.supportedEekCurve, eekLength, eekId_);
        ASSERT_TRUE(chain) << chain.message();
        if (chain) testEekChain_ = chain.moveValue();
        testEekLength_ = eekLength;
    }

    void generateKeys(bool testMode, size_t numKeys) {
        keysToSign_ = std::vector<MacedPublicKey>(numKeys);
        cborKeysToSign_ = cppbor::Array();

        for (auto& key : keysToSign_) {
            bytevec privateKeyBlob;
            auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &key, &privateKeyBlob);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            vector<uint8_t> payload_value;
            check_maced_pubkey(key, testMode, &payload_value);
            cborKeysToSign_.add(cppbor::EncodedItem(payload_value));
        }
    }

    bytevec eekId_;
    size_t testEekLength_;
    EekChain testEekChain_;
    bytevec challenge_;
    std::vector<MacedPublicKey> keysToSign_;
    cppbor::Array cborKeysToSign_;
};

class CertificateRequestTest : public CertificateRequestTestBase {
  protected:
    void SetUp() override {
        CertificateRequestTestBase::SetUp();
        ASSERT_FALSE(HasFatalFailure());

        if (rpcHardwareInfo.versionNumber >= VERSION_WITH_CERTIFICATE_REQUEST_V2) {
            GTEST_SKIP() << "This test case only applies to RKP v1 and v2. "
                         << "RKP version discovered: " << rpcHardwareInfo.versionNumber;
        }
    }
};

/**
 * Generate an empty certificate request in test mode, and decrypt and verify the structure and
 * content.
 */
TEST_P(CertificateRequestTest, EmptyRequest_testMode) {
    bool testMode = true;
    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateTestEekChain(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, {} /* keysToSign */, testEekChain_.chain, challenge_, &deviceInfo,
                &protectedData, &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        auto result = verifyProductionProtectedData(
                deviceInfo, cppbor::Array(), keysToSignMac, protectedData, testEekChain_, eekId_,
                rpcHardwareInfo.supportedEekCurve, provisionable_.get(), challenge_);
        ASSERT_TRUE(result) << result.message();
    }
}

/**
 * Ensure that test mode outputs a unique BCC root key every time we request a
 * certificate request. Else, it's possible that the test mode API could be used
 * to fingerprint devices. Only the GEEK should be allowed to decrypt the same
 * device public key multiple times.
 */
TEST_P(CertificateRequestTest, NewKeyPerCallInTestMode) {
    constexpr bool testMode = true;

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    generateTestEekChain(3);
    auto status = provisionable_->generateCertificateRequest(
            testMode, {} /* keysToSign */, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto firstBcc = verifyProductionProtectedData(
            deviceInfo, /*keysToSign=*/cppbor::Array(), keysToSignMac, protectedData, testEekChain_,
            eekId_, rpcHardwareInfo.supportedEekCurve, provisionable_.get(), challenge_);
    ASSERT_TRUE(firstBcc) << firstBcc.message();

    status = provisionable_->generateCertificateRequest(
            testMode, {} /* keysToSign */, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto secondBcc = verifyProductionProtectedData(
            deviceInfo, /*keysToSign=*/cppbor::Array(), keysToSignMac, protectedData, testEekChain_,
            eekId_, rpcHardwareInfo.supportedEekCurve, provisionable_.get(), challenge_);
    ASSERT_TRUE(secondBcc) << secondBcc.message();

    // Verify that none of the keys in the first BCC are repeated in the second one.
    for (const auto& i : *firstBcc) {
        for (auto& j : *secondBcc) {
            ASSERT_THAT(i.pubKey, testing::Not(testing::ElementsAreArray(j.pubKey)))
                    << "Found a repeated pubkey in two generateCertificateRequest test mode calls";
        }
    }
}

/**
 * Generate an empty certificate request in prod mode. This test must be run explicitly, and
 * is not run by default. Not all devices are GMS devices, and therefore they do not all
 * trust the Google EEK root.
 */
TEST_P(CertificateRequestTest, DISABLED_EmptyRequest_prodMode) {
    bool testMode = false;

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, {} /* keysToSign */, getProdEekChain(rpcHardwareInfo.supportedEekCurve),
            challenge_, &deviceInfo, &protectedData, &keysToSignMac);
    EXPECT_TRUE(status.isOk());
}

/**
 * Generate a non-empty certificate request in test mode.  Decrypt, parse and validate the contents.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_testMode) {
    bool testMode = true;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateTestEekChain(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, testEekChain_.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        auto result = verifyProductionProtectedData(
                deviceInfo, cborKeysToSign_, keysToSignMac, protectedData, testEekChain_, eekId_,
                rpcHardwareInfo.supportedEekCurve, provisionable_.get(), challenge_);
        ASSERT_TRUE(result) << result.message();
    }
}

/**
 * Generate a non-empty certificate request in prod mode. This test must be run explicitly, and
 * is not run by default. Not all devices are GMS devices, and therefore they do not all
 * trust the Google EEK root.
 */
TEST_P(CertificateRequestTest, DISABLED_NonEmptyRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, keysToSign_, getProdEekChain(rpcHardwareInfo.supportedEekCurve), challenge_,
            &deviceInfo, &protectedData, &keysToSignMac);
    EXPECT_TRUE(status.isOk());
}

/**
 * Generate a non-empty certificate request in test mode, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestTest, NonEmptyRequestCorruptMac_testMode) {
    bool testMode = true;
    generateKeys(testMode, 1 /* numKeys */);
    auto result = corrupt_maced_key(keysToSign_[0]);
    ASSERT_TRUE(result) << result.moveMessage();
    MacedPublicKey keyWithCorruptMac = result.moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    generateTestEekChain(3);
    auto status = provisionable_->generateCertificateRequest(
            testMode, {keyWithCorruptMac}, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_MAC);
}

/**
 * Generate a non-empty certificate request in prod mode, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestTest, NonEmptyRequestCorruptMac_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 1 /* numKeys */);
    auto result = corrupt_maced_key(keysToSign_[0]);
    ASSERT_TRUE(result) << result.moveMessage();
    MacedPublicKey keyWithCorruptMac = result.moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, {keyWithCorruptMac}, getProdEekChain(rpcHardwareInfo.supportedEekCurve),
            challenge_, &deviceInfo, &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_MAC);
}

/**
 * Generate a non-empty certificate request in prod mode that has a corrupt EEK chain.
 * Confirm that the request is rejected.
 */
TEST_P(CertificateRequestTest, NonEmptyCorruptEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    auto prodEekChain = getProdEekChain(rpcHardwareInfo.supportedEekCurve);
    auto [parsedChain, _, parseErr] = cppbor::parse(prodEekChain);
    ASSERT_NE(parsedChain, nullptr) << parseErr;
    ASSERT_NE(parsedChain->asArray(), nullptr);

    for (int ii = 0; ii < parsedChain->asArray()->size(); ++ii) {
        auto chain = corrupt_sig_chain(prodEekChain, ii);
        ASSERT_TRUE(chain) << chain.message();

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(testMode, keysToSign_, *chain,
                                                                 challenge_, &deviceInfo,
                                                                 &protectedData, &keysToSignMac);
        ASSERT_FALSE(status.isOk());
        ASSERT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in prod mode that has an incomplete EEK chain.
 * Confirm that the request is rejected.
 */
TEST_P(CertificateRequestTest, NonEmptyIncompleteEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    // Build an EEK chain that omits the first self-signed cert.
    auto truncatedChain = cppbor::Array();
    auto [chain, _, parseErr] = cppbor::parse(getProdEekChain(rpcHardwareInfo.supportedEekCurve));
    ASSERT_TRUE(chain);
    auto eekChain = chain->asArray();
    ASSERT_NE(eekChain, nullptr);
    for (size_t ii = 1; ii < eekChain->size(); ii++) {
        truncatedChain.add(eekChain->get(ii)->clone());
    }

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, keysToSign_, truncatedChain.encode(), challenge_, &deviceInfo, &protectedData,
            &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
}

/**
 * Generate a non-empty certificate request in test mode, with prod keys.  Must fail with
 * STATUS_PRODUCTION_KEY_IN_TEST_REQUEST.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_prodKeyInTestCert) {
    generateKeys(false /* testMode */, 2 /* numKeys */);

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    generateTestEekChain(3);
    auto status = provisionable_->generateCertificateRequest(
            true /* testMode */, keysToSign_, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_PRODUCTION_KEY_IN_TEST_REQUEST);
}

/**
 * Generate a non-empty certificate request in prod mode, with test keys.  Must fail with
 * STATUS_TEST_KEY_IN_PRODUCTION_REQUEST.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_testKeyInProdCert) {
    generateKeys(true /* testMode */, 2 /* numKeys */);

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    generateTestEekChain(3);
    auto status = provisionable_->generateCertificateRequest(
            false /* testMode */, keysToSign_, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST);
}

INSTANTIATE_REM_PROV_AIDL_TEST(CertificateRequestTest);

class CertificateRequestV2Test : public CertificateRequestTestBase {
    void SetUp() override {
        CertificateRequestTestBase::SetUp();
        ASSERT_FALSE(HasFatalFailure());

        if (rpcHardwareInfo.versionNumber < VERSION_WITH_CERTIFICATE_REQUEST_V2) {
            GTEST_SKIP() << "This test case only applies to RKP v3 and above. "
                         << "RKP version discovered: " << rpcHardwareInfo.versionNumber;
        }
    }
};

/**
 * Generate an empty certificate request with all possible length of challenge, and decrypt and
 * verify the structure and content.
 */
// @VsrTest = 3.10-015
TEST_P(CertificateRequestV2Test, EmptyRequest) {
    bytevec csr;

    for (auto size = MIN_CHALLENGE_SIZE; size <= MAX_CHALLENGE_SIZE; size++) {
        SCOPED_TRACE(testing::Message() << "challenge[" << size << "]");
        auto challenge = randomBytes(size);
        auto status =
                provisionable_->generateCertificateRequestV2({} /* keysToSign */, challenge, &csr);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        auto result = verifyProductionCsr(cppbor::Array(), csr, provisionable_.get(), challenge);
        ASSERT_TRUE(result) << result.message();
    }
}

/**
 * Generate a non-empty certificate request with all possible length of challenge.  Decrypt, parse
 * and validate the contents.
 */
// @VsrTest = 3.10-015
TEST_P(CertificateRequestV2Test, NonEmptyRequest) {
    generateKeys(false /* testMode */, 1 /* numKeys */);

    bytevec csr;

    for (auto size = MIN_CHALLENGE_SIZE; size <= MAX_CHALLENGE_SIZE; size++) {
        SCOPED_TRACE(testing::Message() << "challenge[" << size << "]");
        auto challenge = randomBytes(size);
        auto status = provisionable_->generateCertificateRequestV2(keysToSign_, challenge, &csr);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        auto result = verifyProductionCsr(cborKeysToSign_, csr, provisionable_.get(), challenge);
        ASSERT_TRUE(result) << result.message();
    }
}

/**
 * Generate an empty certificate request with invalid size of challenge
 */
TEST_P(CertificateRequestV2Test, EmptyRequestWithInvalidChallengeFail) {
    bytevec csr;

    auto status = provisionable_->generateCertificateRequestV2(
            /* keysToSign */ {}, randomBytes(MAX_CHALLENGE_SIZE + 1), &csr);
    EXPECT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_FAILED);
}

/**
 * Generate a non-empty certificate request.  Make sure contents are reproducible but allow for the
 * signature to be different since algorithms including ECDSA P-256 can include a random value.
 */
// @VsrTest = 3.10-015
TEST_P(CertificateRequestV2Test, NonEmptyRequestReproducible) {
    generateKeys(false /* testMode */, 1 /* numKeys */);

    bytevec csr;

    auto status = provisionable_->generateCertificateRequestV2(keysToSign_, challenge_, &csr);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto firstCsr = verifyProductionCsr(cborKeysToSign_, csr, provisionable_.get(), challenge_);
    ASSERT_TRUE(firstCsr) << firstCsr.message();

    status = provisionable_->generateCertificateRequestV2(keysToSign_, challenge_, &csr);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto secondCsr = verifyProductionCsr(cborKeysToSign_, csr, provisionable_.get(), challenge_);
    ASSERT_TRUE(secondCsr) << secondCsr.message();

    ASSERT_EQ(**firstCsr, **secondCsr);
}

/**
 * Generate a non-empty certificate request with multiple keys.
 */
// @VsrTest = 3.10-015
TEST_P(CertificateRequestV2Test, NonEmptyRequestMultipleKeys) {
    generateKeys(false /* testMode */, rpcHardwareInfo.supportedNumKeysInCsr /* numKeys */);

    bytevec csr;

    auto status = provisionable_->generateCertificateRequestV2(keysToSign_, challenge_, &csr);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto result = verifyProductionCsr(cborKeysToSign_, csr, provisionable_.get(), challenge_);
    ASSERT_TRUE(result) << result.message();
}

/**
 * Generate a non-empty certificate request, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestV2Test, NonEmptyRequestCorruptMac) {
    generateKeys(false /* testMode */, 1 /* numKeys */);
    auto result = corrupt_maced_key(keysToSign_[0]);
    ASSERT_TRUE(result) << result.moveMessage();
    MacedPublicKey keyWithCorruptMac = result.moveValue();

    bytevec csr;
    auto status =
            provisionable_->generateCertificateRequestV2({keyWithCorruptMac}, challenge_, &csr);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_INVALID_MAC);
}

/**
 * Call generateCertificateRequest(). Make sure it's removed.
 */
TEST_P(CertificateRequestV2Test, CertificateRequestV1Removed_prodMode) {
    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            false /* testMode */, {} /* keysToSign */, {} /* EEK chain */, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_REMOVED);
}

/**
 * Call generateCertificateRequest() in test mode. Make sure it's removed.
 */
TEST_P(CertificateRequestV2Test, CertificateRequestV1Removed_testMode) {
    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            true /* testMode */, {} /* keysToSign */, {} /* EEK chain */, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk()) << status.getMessage();
    EXPECT_EQ(status.getServiceSpecificError(), BnRemotelyProvisionedComponent::STATUS_REMOVED);
}

void parse_root_of_trust(const vector<uint8_t>& attestation_cert,
                         vector<uint8_t>* verified_boot_key, VerifiedBoot* verified_boot_state,
                         bool* device_locked, vector<uint8_t>* verified_boot_hash) {
    X509_Ptr cert(parse_cert_blob(attestation_cert));
    ASSERT_TRUE(cert.get());

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    ASSERT_TRUE(attest_rec);

    auto error = parse_root_of_trust(attest_rec->data, attest_rec->length, verified_boot_key,
                                     verified_boot_state, device_locked, verified_boot_hash);
    ASSERT_EQ(error, ErrorCode::OK);
}

/**
 * Generate a CSR and verify DeviceInfo against IDs attested by KeyMint.
 */
// @VsrTest = 3.10-015
TEST_P(CertificateRequestV2Test, DeviceInfo) {
    // See if there is a matching IKeyMintDevice for this IRemotelyProvisionedComponent.
    std::shared_ptr<IKeyMintDevice> keyMint;
    if (!matching_keymint_device(GetParam(), &keyMint)) {
        // No matching IKeyMintDevice.
        GTEST_SKIP() << "Skipping key use test as no matching KeyMint device found";
        return;
    }
    KeyMintHardwareInfo info;
    ASSERT_TRUE(keyMint->getHardwareInfo(&info).isOk());

    // Get IDs attested by KeyMint.
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    auto irpcStatus =
            provisionable_->generateEcdsaP256KeyPair(false, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(irpcStatus.isOk());

    AttestationKey attestKey;
    attestKey.keyBlob = std::move(privateKeyBlob);
    attestKey.issuerSubjectName = make_name_from_str("Android Keystore Key");

    // Generate an ECDSA key that is attested by the generated P256 keypair.
    AuthorizationSet keyDesc = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .EcdsaSigningKey(EcCurve::P_256)
                                       .AttestationChallenge("foo")
                                       .AttestationApplicationId("bar")
                                       .Digest(Digest::NONE)
                                       .SetDefaultValidity();
    KeyCreationResult creationResult;
    auto kmStatus = keyMint->generateKey(keyDesc.vector_data(), attestKey, &creationResult);
    ASSERT_TRUE(kmStatus.isOk());

    vector<KeyCharacteristics> key_characteristics = std::move(creationResult.keyCharacteristics);
    vector<Certificate> key_cert_chain = std::move(creationResult.certificateChain);
    // We didn't provision the attestation key.
    ASSERT_EQ(key_cert_chain.size(), 1);

    // Parse attested patch levels.
    auto auths = HwEnforcedAuthorizations(key_characteristics);

    auto attestedSystemPatchLevel = auths.GetTagValue(TAG_OS_PATCHLEVEL);
    auto attestedVendorPatchLevel = auths.GetTagValue(TAG_VENDOR_PATCHLEVEL);
    auto attestedBootPatchLevel = auths.GetTagValue(TAG_BOOT_PATCHLEVEL);

    ASSERT_TRUE(attestedSystemPatchLevel.has_value());
    ASSERT_TRUE(attestedVendorPatchLevel.has_value());
    ASSERT_TRUE(attestedBootPatchLevel.has_value());

    // Parse attested AVB values.
    vector<uint8_t> key;
    VerifiedBoot attestedVbState;
    bool attestedBootloaderState;
    vector<uint8_t> attestedVbmetaDigest;
    parse_root_of_trust(key_cert_chain[0].encodedCertificate, &key, &attestedVbState,
                        &attestedBootloaderState, &attestedVbmetaDigest);

    // Get IDs from DeviceInfo.
    bytevec csr;
    irpcStatus =
            provisionable_->generateCertificateRequestV2({} /* keysToSign */, challenge_, &csr);
    ASSERT_TRUE(irpcStatus.isOk()) << irpcStatus.getMessage();

    auto result = verifyProductionCsr(cppbor::Array(), csr, provisionable_.get(), challenge_);
    ASSERT_TRUE(result) << result.message();

    std::unique_ptr<cppbor::Array> csrPayload = std::move(*result);
    ASSERT_TRUE(csrPayload);

    auto deviceInfo = csrPayload->get(2)->asMap();
    ASSERT_TRUE(deviceInfo);

    auto vbState = deviceInfo->get("vb_state")->asTstr();
    auto bootloaderState = deviceInfo->get("bootloader_state")->asTstr();
    auto vbmetaDigest = deviceInfo->get("vbmeta_digest")->asBstr();
    auto systemPatchLevel = deviceInfo->get("system_patch_level")->asUint();
    auto vendorPatchLevel = deviceInfo->get("vendor_patch_level")->asUint();
    auto bootPatchLevel = deviceInfo->get("boot_patch_level")->asUint();
    auto securityLevel = deviceInfo->get("security_level")->asTstr();

    ASSERT_TRUE(vbState);
    ASSERT_TRUE(bootloaderState);
    ASSERT_TRUE(vbmetaDigest);
    ASSERT_TRUE(systemPatchLevel);
    ASSERT_TRUE(vendorPatchLevel);
    ASSERT_TRUE(bootPatchLevel);
    ASSERT_TRUE(securityLevel);

    auto kmDeviceName = device_suffix(GetParam());

    // Compare DeviceInfo against IDs attested by KeyMint.
    ASSERT_TRUE((securityLevel->value() == "tee" && kmDeviceName == "default") ||
                (securityLevel->value() == "strongbox" && kmDeviceName == "strongbox"));
    ASSERT_TRUE((vbState->value() == "green" && attestedVbState == VerifiedBoot::VERIFIED) ||
                (vbState->value() == "yellow" && attestedVbState == VerifiedBoot::SELF_SIGNED) ||
                (vbState->value() == "orange" && attestedVbState == VerifiedBoot::UNVERIFIED));
    ASSERT_TRUE((bootloaderState->value() == "locked" && attestedBootloaderState) ||
                (bootloaderState->value() == "unlocked" && !attestedBootloaderState));
    ASSERT_EQ(vbmetaDigest->value(), attestedVbmetaDigest);
    ASSERT_EQ(systemPatchLevel->value(), attestedSystemPatchLevel.value());
    ASSERT_EQ(vendorPatchLevel->value(), attestedVendorPatchLevel.value());
    ASSERT_EQ(bootPatchLevel->value(), attestedBootPatchLevel.value());
}

INSTANTIATE_REM_PROV_AIDL_TEST(CertificateRequestV2Test);

using VsrRequirementTest = VtsRemotelyProvisionedComponentTests;

INSTANTIATE_REM_PROV_AIDL_TEST(VsrRequirementTest);

TEST_P(VsrRequirementTest, VsrEnforcementTest) {
    RpcHardwareInfo hwInfo;
    ASSERT_TRUE(provisionable_->getHardwareInfo(&hwInfo).isOk());
    int vsr_api_level = get_vsr_api_level();
    if (vsr_api_level < 34) {
        GTEST_SKIP() << "Applies only to VSR API level 34 or newer, this device is: "
                     << vsr_api_level;
    }
    EXPECT_GE(hwInfo.versionNumber, 3)
            << "VSR 14+ requires IRemotelyProvisionedComponent v3 or newer.";
}

}  // namespace aidl::android::hardware::security::keymint::test
