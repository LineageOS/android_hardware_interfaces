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

#define LOG_TAG "VtsRemotelyProvisionableComponentTests"

#include <AndroidRemotelyProvisionedComponentDevice.h>
#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <android/binder_manager.h>
#include <cppbor_parse.h>
#include <gmock/gmock.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymaster/keymaster_configuration.h>
#include <keymint_support/authorization_set.h>
#include <openssl/ec.h>
#include <openssl/ec_key.h>
#include <openssl/x509.h>
#include <remote_prov/remote_prov_utils.h>
#include <vector>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::std::string;
using ::std::vector;

namespace {

#define INSTANTIATE_REM_PROV_AIDL_TEST(name)                                         \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(name);                             \
    INSTANTIATE_TEST_SUITE_P(                                                        \
            PerInstance, name,                                                       \
            testing::ValuesIn(VtsRemotelyProvisionedComponentTests::build_params()), \
            ::android::PrintInstanceNameToString)

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
    }

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(IRemotelyProvisionedComponent::descriptor);
        return params;
    }

  protected:
    std::shared_ptr<IRemotelyProvisionedComponent> provisionable_;
};

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
                                       .EcdsaSigningKey(256)
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

    AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
    AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
    EXPECT_TRUE(verify_attestation_record("foo", "bar", sw_enforced, hw_enforced,
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
    ASSERT_TRUE(status.isOk());

    check_maced_pubkey(macedPubKey, testMode, nullptr);
}

class CertificateRequestTest : public VtsRemotelyProvisionedComponentTests {
  protected:
    CertificateRequestTest() : eekId_(string_to_bytevec("eekid")), challenge_(randomBytes(32)) {
        generateTestEekChain(3);
    }

    void generateTestEekChain(size_t eekLength) {
        auto chain = generateEekChain(eekLength, eekId_);
        EXPECT_TRUE(chain) << chain.message();
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

    void checkProtectedData(const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
                            const bytevec& keysToSignMac, const ProtectedData& protectedData,
                            std::vector<BccEntryData>* bccOutput = nullptr) {
        auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData.protectedData);
        ASSERT_TRUE(parsedProtectedData) << protDataErrMsg;
        ASSERT_TRUE(parsedProtectedData->asArray());
        ASSERT_EQ(parsedProtectedData->asArray()->size(), kCoseEncryptEntryCount);

        auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
        ASSERT_TRUE(senderPubkey) << senderPubkey.message();
        EXPECT_EQ(senderPubkey->second, eekId_);

        auto sessionKey =
                x25519_HKDF_DeriveKey(testEekChain_.last_pubkey, testEekChain_.last_privkey,
                                      senderPubkey->first, false /* senderIsA */);
        ASSERT_TRUE(sessionKey) << sessionKey.message();

        auto protectedDataPayload =
                decryptCoseEncrypt(*sessionKey, parsedProtectedData.get(), bytevec{} /* aad */);
        ASSERT_TRUE(protectedDataPayload) << protectedDataPayload.message();

        auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(*protectedDataPayload);
        ASSERT_TRUE(parsedPayload) << "Failed to parse payload: " << payloadErrMsg;
        ASSERT_TRUE(parsedPayload->asArray());
        EXPECT_EQ(parsedPayload->asArray()->size(), 2U);

        auto& signedMac = parsedPayload->asArray()->get(0);
        auto& bcc = parsedPayload->asArray()->get(1);
        ASSERT_TRUE(signedMac && signedMac->asArray());
        ASSERT_TRUE(bcc && bcc->asArray());

        // BCC is [ pubkey, + BccEntry]
        auto bccContents = validateBcc(bcc->asArray());
        ASSERT_TRUE(bccContents) << "\n" << bccContents.message() << "\n" << prettyPrint(bcc.get());
        ASSERT_GT(bccContents->size(), 0U);

        auto [deviceInfoMap, __2, deviceInfoErrMsg] = cppbor::parse(deviceInfo.deviceInfo);
        ASSERT_TRUE(deviceInfoMap) << "Failed to parse deviceInfo: " << deviceInfoErrMsg;
        ASSERT_TRUE(deviceInfoMap->asMap());

        auto& signingKey = bccContents->back().pubKey;
        auto macKey = verifyAndParseCoseSign1(signedMac->asArray(), signingKey,
                                              cppbor::Array()  // SignedMacAad
                                                      .add(challenge_)
                                                      .add(std::move(deviceInfoMap))
                                                      .add(keysToSignMac)
                                                      .encode());
        ASSERT_TRUE(macKey) << macKey.message();

        auto coseMac0 = cppbor::Array()
                                .add(cppbor::Map()  // protected
                                             .add(ALGORITHM, HMAC_256)
                                             .canonicalize()
                                             .encode())
                                .add(cppbor::Map())        // unprotected
                                .add(keysToSign.encode())  // payload (keysToSign)
                                .add(keysToSignMac);       // tag

        auto macPayload = verifyAndParseCoseMac0(&coseMac0, *macKey);
        ASSERT_TRUE(macPayload) << macPayload.message();

        if (bccOutput) {
            *bccOutput = std::move(*bccContents);
        }
    }

    bytevec eekId_;
    size_t testEekLength_;
    EekChain testEekChain_;
    bytevec challenge_;
    std::vector<MacedPublicKey> keysToSign_;
    cppbor::Array cborKeysToSign_;
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

        checkProtectedData(deviceInfo, cppbor::Array(), keysToSignMac, protectedData);
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
    auto status = provisionable_->generateCertificateRequest(
            testMode, {} /* keysToSign */, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    std::vector<BccEntryData> firstBcc;
    checkProtectedData(deviceInfo, /*keysToSign=*/cppbor::Array(), keysToSignMac, protectedData,
                       &firstBcc);

    status = provisionable_->generateCertificateRequest(
            testMode, {} /* keysToSign */, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    std::vector<BccEntryData> secondBcc;
    checkProtectedData(deviceInfo, /*keysToSign=*/cppbor::Array(), keysToSignMac, protectedData,
                       &secondBcc);

    // Verify that none of the keys in the first BCC are repeated in the second one.
    for (const auto& i : firstBcc) {
        for (auto& j : secondBcc) {
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
            testMode, {} /* keysToSign */, getProdEekChain(), challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
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

        checkProtectedData(deviceInfo, cborKeysToSign_, keysToSignMac, protectedData);
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
            testMode, keysToSign_, getProdEekChain(), challenge_, &deviceInfo, &protectedData,
            &keysToSignMac);
    EXPECT_TRUE(status.isOk());
}

/**
 * Generate a non-empty certificate request in test mode, but with the MAC corrupted on the keypair.
 */
TEST_P(CertificateRequestTest, NonEmptyRequestCorruptMac_testMode) {
    bool testMode = true;
    generateKeys(testMode, 1 /* numKeys */);
    MacedPublicKey keyWithCorruptMac = corrupt_maced_key(keysToSign_[0]).moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
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
    MacedPublicKey keyWithCorruptMac = corrupt_maced_key(keysToSign_[0]).moveValue();

    bytevec keysToSignMac;
    DeviceInfo deviceInfo;
    ProtectedData protectedData;
    auto status = provisionable_->generateCertificateRequest(
            testMode, {keyWithCorruptMac}, getProdEekChain(), challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
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

    auto prodEekChain = getProdEekChain();
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
    auto [chain, _, parseErr] = cppbor::parse(getProdEekChain());
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
    auto status = provisionable_->generateCertificateRequest(
            false /* testMode */, keysToSign_, testEekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST);
}

INSTANTIATE_REM_PROV_AIDL_TEST(CertificateRequestTest);

}  // namespace aidl::android::hardware::security::keymint::test
