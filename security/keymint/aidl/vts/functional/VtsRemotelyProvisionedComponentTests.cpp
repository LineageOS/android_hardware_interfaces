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

#include <RemotelyProvisionedComponent.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <aidl/android/hardware/security/keymint/SecurityLevel.h>
#include <android/binder_manager.h>
#include <cppbor_parse.h>
#include <cppcose/cppcose.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <keymaster/keymaster_configuration.h>
#include <remote_prov/remote_prov_utils.h>

namespace aidl::android::hardware::security::keymint::test {

using ::std::string;
using ::std::vector;

namespace {

#define INSTANTIATE_REM_PROV_AIDL_TEST(name)                                         \
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

void check_cose_key(const vector<uint8_t>& data, bool testMode) {
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(data);
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;

    // The following check assumes that canonical CBOR encoding is used for the COSE_Key.
    if (testMode) {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "  -70000 : null,\n"                              // test marker
                                 "}"));
    } else {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "}"));
    }
}

void check_maced_pubkey(const MacedPublicKey& macedPubKey, bool testMode,
                        vector<uint8_t>* payload_value) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    ASSERT_TRUE(coseMac0) << "COSE Mac0 parse failed " << mac0ParseErr;

    ASSERT_NE(coseMac0->asArray(), nullptr);
    ASSERT_EQ(coseMac0->asArray()->size(), kCoseMac0EntryCount);

    auto protParms = coseMac0->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
    ASSERT_NE(protParms, nullptr);

    // Header label:value of 'alg': HMAC-256
    ASSERT_EQ(cppbor::prettyPrint(protParms->value()), "{\n  1 : 5,\n}");

    auto unprotParms = coseMac0->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
    ASSERT_NE(unprotParms, nullptr);
    ASSERT_EQ(unprotParms->size(), 0);

    // The payload is a bstr holding an encoded COSE_Key
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    ASSERT_NE(payload, nullptr);
    check_cose_key(payload->value(), testMode);

    auto coseMac0Tag = coseMac0->asArray()->get(kCoseMac0Tag)->asBstr();
    ASSERT_TRUE(coseMac0Tag);
    auto extractedTag = coseMac0Tag->value();
    EXPECT_EQ(extractedTag.size(), 32U);

    // Compare with tag generated with kTestMacKey.  Should only match in test mode
    auto testTag = cppcose::generateCoseMac0Mac(remote_prov::kTestMacKey, {} /* external_aad */,
                                                payload->value());
    ASSERT_TRUE(testTag) << "Tag calculation failed: " << testTag.message();

    if (testMode) {
        EXPECT_EQ(*testTag, extractedTag);
    } else {
        EXPECT_NE(*testTag, extractedTag);
    }
    if (payload_value != nullptr) {
        *payload_value = payload->value();
    }
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

ErrMsgOr<EekChain> corrupt_sig_chain(const EekChain& eek, int which) {
    auto [chain, _, parseErr] = cppbor::parse(eek.chain);
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
    return EekChain{corruptChain.encode(), eek.last_pubkey, eek.last_privkey};
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
 * Generate and validate a production-mode key.  MAC tag can't be verified.
 */
TEST_P(GenerateKeyTests, generateEcdsaP256Key_prodMode) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    bool testMode = false;
    auto status = provisionable_->generateEcdsaP256KeyPair(testMode, &macedPubKey, &privateKeyBlob);
    ASSERT_TRUE(status.isOk());

    check_maced_pubkey(macedPubKey, testMode, nullptr);
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
        generateEek(3);
    }

    void generateEek(size_t eekLength) {
        auto chain = generateEekChain(eekLength, eekId_);
        EXPECT_TRUE(chain) << chain.message();
        if (chain) eekChain_ = chain.moveValue();
        eekLength_ = eekLength;
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

    void checkProtectedData(bool testMode, const cppbor::Array& keysToSign,
                            const bytevec& keysToSignMac, const ProtectedData& protectedData) {
        auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData.protectedData);
        ASSERT_TRUE(parsedProtectedData) << protDataErrMsg;
        ASSERT_TRUE(parsedProtectedData->asArray());
        ASSERT_EQ(parsedProtectedData->asArray()->size(), kCoseEncryptEntryCount);

        auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
        ASSERT_TRUE(senderPubkey) << senderPubkey.message();
        EXPECT_EQ(senderPubkey->second, eekId_);

        auto sessionKey = x25519_HKDF_DeriveKey(eekChain_.last_pubkey, eekChain_.last_privkey,
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

        auto& signingKey = bccContents->back().pubKey;
        auto macKey = verifyAndParseCoseSign1(testMode, signedMac->asArray(), signingKey,
                                              cppbor::Array()  // DeviceInfo
                                                      .add(challenge_)
                                                      .add(cppbor::Map())
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
    }

    bytevec eekId_;
    size_t eekLength_;
    EekChain eekChain_;
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
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, {} /* keysToSign */, eekChain_.chain, challenge_, &deviceInfo,
                &protectedData, &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        checkProtectedData(testMode, cppbor::Array(), keysToSignMac, protectedData);
    }
}

/**
 * Generate an empty certificate request in prod mode.  Generation will fail because we don't have a
 * valid GEEK.
 *
 * TODO(swillden): Get a valid GEEK and use it so the generation can succeed, though we won't be
 * able to decrypt.
 */
TEST_P(CertificateRequestTest, EmptyRequest_prodMode) {
    bool testMode = false;
    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, {} /* keysToSign */, eekChain_.chain, challenge_, &deviceInfo,
                &protectedData, &keysToSignMac);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in test mode.  Decrypt, parse and validate the contents.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_testMode) {
    bool testMode = true;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        checkProtectedData(testMode, cborKeysToSign_, keysToSignMac, protectedData);
    }
}

/**
 * Generate a non-empty certificate request in prod mode.  Must fail because we don't have a valid
 * GEEK.
 *
 * TODO(swillden): Get a valid GEEK and use it so the generation can succeed, though we won't be
 * able to decrypt.
 */
TEST_P(CertificateRequestTest, NonEmptyRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t eekLength : {2, 3, 7}) {
        SCOPED_TRACE(testing::Message() << "EEK of length " << eekLength);
        generateEek(eekLength);

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, eekChain_.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        EXPECT_FALSE(status.isOk());
        EXPECT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in prod mode that has a corrupt EEK chain.
 * Confirm that the request is rejected.
 *
 * TODO(drysdale): Update to use a valid GEEK, so that the test actually confirms that the
 * implementation is checking signatures.
 */
TEST_P(CertificateRequestTest, NonEmptyCorruptEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    for (size_t ii = 0; ii < eekLength_; ii++) {
        auto chain = corrupt_sig_chain(eekChain_, ii);
        ASSERT_TRUE(chain) << chain.message();
        EekChain corruptEek = chain.moveValue();

        bytevec keysToSignMac;
        DeviceInfo deviceInfo;
        ProtectedData protectedData;
        auto status = provisionable_->generateCertificateRequest(
                testMode, keysToSign_, corruptEek.chain, challenge_, &deviceInfo, &protectedData,
                &keysToSignMac);
        ASSERT_FALSE(status.isOk());
        ASSERT_EQ(status.getServiceSpecificError(),
                  BnRemotelyProvisionedComponent::STATUS_INVALID_EEK);
    }
}

/**
 * Generate a non-empty certificate request in prod mode that has an incomplete EEK chain.
 * Confirm that the request is rejected.
 *
 * TODO(drysdale): Update to use a valid GEEK, so that the test actually confirms that the
 * implementation is checking signatures.
 */
TEST_P(CertificateRequestTest, NonEmptyIncompleteEekRequest_prodMode) {
    bool testMode = false;
    generateKeys(testMode, 4 /* numKeys */);

    // Build an EEK chain that omits the first self-signed cert.
    auto truncatedChain = cppbor::Array();
    auto [chain, _, parseErr] = cppbor::parse(eekChain_.chain);
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
            true /* testMode */, keysToSign_, eekChain_.chain, challenge_, &deviceInfo,
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
            false /* testMode */, keysToSign_, eekChain_.chain, challenge_, &deviceInfo,
            &protectedData, &keysToSignMac);
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(status.getServiceSpecificError(),
              BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST);
}

INSTANTIATE_REM_PROV_AIDL_TEST(CertificateRequestTest);

}  // namespace aidl::android::hardware::security::keymint::test
