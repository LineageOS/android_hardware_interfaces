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

#define LOG_TAG "ReaderAuthTests"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/keymaster/HardwareAuthToken.h>
#include <aidl/android/hardware/keymaster/VerificationToken.h>
#include <android-base/logging.h>
#include <android/hardware/identity/IIdentityCredentialStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cppbor.h>
#include <cppbor_parse.h>
#include <gtest/gtest.h>
#include <future>
#include <map>
#include <utility>

#include "Util.h"

namespace android::hardware::identity {

using std::endl;
using std::make_pair;
using std::map;
using std::optional;
using std::pair;
using std::string;
using std::tie;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using ::android::hardware::keymaster::HardwareAuthToken;
using ::android::hardware::keymaster::VerificationToken;

class ReaderAuthTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    void provisionData();
    void retrieveData(const vector<uint8_t>& readerPrivateKey,
                      const vector<vector<uint8_t>>& readerCertChain, bool expectSuccess,
                      bool leaveOutAccessibleToAllFromRequestMessage);

    // Set by provisionData
    vector<uint8_t> readerPublicKey_;
    vector<uint8_t> readerPrivateKey_;
    vector<uint8_t> intermediateAPublicKey_;
    vector<uint8_t> intermediateAPrivateKey_;
    vector<uint8_t> intermediateBPublicKey_;
    vector<uint8_t> intermediateBPrivateKey_;
    vector<uint8_t> intermediateCPublicKey_;
    vector<uint8_t> intermediateCPrivateKey_;

    vector<uint8_t> cert_A_SelfSigned_;

    vector<uint8_t> cert_B_SelfSigned_;

    vector<uint8_t> cert_B_SignedBy_C_;

    vector<uint8_t> cert_C_SelfSigned_;

    vector<uint8_t> cert_reader_SelfSigned_;
    vector<uint8_t> cert_reader_SignedBy_A_;
    vector<uint8_t> cert_reader_SignedBy_B_;

    SecureAccessControlProfile sacp0_;
    SecureAccessControlProfile sacp1_;
    SecureAccessControlProfile sacp2_;
    SecureAccessControlProfile sacp3_;

    vector<uint8_t> encContentAccessibleByA_;
    vector<uint8_t> encContentAccessibleByAorB_;
    vector<uint8_t> encContentAccessibleByB_;
    vector<uint8_t> encContentAccessibleByC_;
    vector<uint8_t> encContentAccessibleByAll_;
    vector<uint8_t> encContentAccessibleByNone_;

    vector<uint8_t> credentialData_;

    // Set by retrieveData()
    bool canGetAccessibleByA_;
    bool canGetAccessibleByAorB_;
    bool canGetAccessibleByB_;
    bool canGetAccessibleByC_;
    bool canGetAccessibleByAll_;
    bool canGetAccessibleByNone_;

    sp<IIdentityCredentialStore> credentialStore_;
};

pair<vector<uint8_t>, vector<uint8_t>> generateReaderKey() {
    optional<vector<uint8_t>> keyPKCS8 = support::createEcKeyPair();
    optional<vector<uint8_t>> publicKey = support::ecKeyPairGetPublicKey(keyPKCS8.value());
    optional<vector<uint8_t>> privateKey = support::ecKeyPairGetPrivateKey(keyPKCS8.value());
    return make_pair(publicKey.value(), privateKey.value());
}

vector<uint8_t> generateReaderCert(const vector<uint8_t>& publicKey,
                                   const vector<uint8_t>& signingKey) {
    time_t validityNotBefore = 0;
    time_t validityNotAfter = 0xffffffff;
    optional<vector<uint8_t>> cert = support::ecPublicKeyGenerateCertificate(
            publicKey, signingKey, "24601", "Issuer", "Subject", validityNotBefore,
            validityNotAfter, {});
    return cert.value();
}

void ReaderAuthTests::provisionData() {
    // Keys and certificates for intermediates.
    tie(intermediateAPublicKey_, intermediateAPrivateKey_) = generateReaderKey();
    tie(intermediateBPublicKey_, intermediateBPrivateKey_) = generateReaderKey();
    tie(intermediateCPublicKey_, intermediateCPrivateKey_) = generateReaderKey();

    cert_A_SelfSigned_ = generateReaderCert(intermediateAPublicKey_, intermediateAPrivateKey_);

    cert_B_SelfSigned_ = generateReaderCert(intermediateBPublicKey_, intermediateBPrivateKey_);

    cert_B_SignedBy_C_ = generateReaderCert(intermediateBPublicKey_, intermediateCPrivateKey_);

    cert_C_SelfSigned_ = generateReaderCert(intermediateCPublicKey_, intermediateCPrivateKey_);

    // Key and self-signed certificate reader
    tie(readerPublicKey_, readerPrivateKey_) = generateReaderKey();
    cert_reader_SelfSigned_ = generateReaderCert(readerPublicKey_, readerPrivateKey_);

    // Certificate for reader signed by intermediates
    cert_reader_SignedBy_A_ = generateReaderCert(readerPublicKey_, intermediateAPrivateKey_);
    cert_reader_SignedBy_B_ = generateReaderCert(readerPublicKey_, intermediateBPrivateKey_);

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credentialStore_->createCredential(docType, testCredential, &wc).isOk());

    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    ASSERT_TRUE(wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                              &certChain)
                        .isOk());

    size_t proofOfProvisioningSize =
            465 + cert_A_SelfSigned_.size() + cert_B_SelfSigned_.size() + cert_C_SelfSigned_.size();
    ASSERT_TRUE(wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize).isOk());

    // Not in v1 HAL, may fail
    wc->startPersonalization(4 /* numAccessControlProfiles */,
                             {6} /* numDataElementsPerNamespace */);

    // AIDL expects certificates wrapped in the Certificate type...
    Certificate cert_A;
    Certificate cert_B;
    Certificate cert_C;
    cert_A.encodedCertificate = cert_A_SelfSigned_;
    cert_B.encodedCertificate = cert_B_SelfSigned_;
    cert_C.encodedCertificate = cert_C_SelfSigned_;

    // Access control profile 0: accessible by A
    ASSERT_TRUE(wc->addAccessControlProfile(0, cert_A, false, 0, 0, &sacp0_).isOk());

    // Access control profile 1: accessible by B
    ASSERT_TRUE(wc->addAccessControlProfile(1, cert_B, false, 0, 0, &sacp1_).isOk());

    // Access control profile 2: accessible by C
    ASSERT_TRUE(wc->addAccessControlProfile(2, cert_C, false, 0, 0, &sacp2_).isOk());

    // Access control profile 3: open access
    ASSERT_TRUE(wc->addAccessControlProfile(3, {}, false, 0, 0, &sacp3_).isOk());

    // Data Element: "Accessible by A"
    ASSERT_TRUE(wc->beginAddEntry({0}, "ns", "Accessible by A", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByA_).isOk());

    // Data Element: "Accessible by A or B"
    ASSERT_TRUE(wc->beginAddEntry({0, 1}, "ns", "Accessible by A or B", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByAorB_).isOk());

    // Data Element: "Accessible by B"
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "Accessible by B", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByB_).isOk());

    // Data Element: "Accessible by C"
    ASSERT_TRUE(wc->beginAddEntry({2}, "ns", "Accessible by C", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByC_).isOk());

    // Data Element: "Accessible by All"
    ASSERT_TRUE(wc->beginAddEntry({3}, "ns", "Accessible by All", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByAll_).isOk());

    // Data Element: "Accessible by None"
    ASSERT_TRUE(wc->beginAddEntry({}, "ns", "Accessible by None", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByNone_).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    ASSERT_TRUE(wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature).isOk());
}

RequestDataItem buildRequestDataItem(const string& name, size_t size,
                                     vector<int32_t> accessControlProfileIds) {
    RequestDataItem item;
    item.name = name;
    item.size = size;
    item.accessControlProfileIds = accessControlProfileIds;
    return item;
}

void ReaderAuthTests::retrieveData(const vector<uint8_t>& readerPrivateKey,
                                   const vector<vector<uint8_t>>& readerCertChain,
                                   bool expectSuccess,
                                   bool leaveOutAccessibleToAllFromRequestMessage) {
    canGetAccessibleByA_ = false;
    canGetAccessibleByAorB_ = false;
    canGetAccessibleByB_ = false;
    canGetAccessibleByC_ = false;
    canGetAccessibleByAll_ = false;
    canGetAccessibleByNone_ = false;

    sp<IIdentityCredential> c;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &c)
                        .isOk());

    optional<vector<uint8_t>> readerEKeyPair = support::createEcKeyPair();
    optional<vector<uint8_t>> readerEPublicKey =
            support::ecKeyPairGetPublicKey(readerEKeyPair.value());
    ASSERT_TRUE(c->setReaderEphemeralPublicKey(readerEPublicKey.value()).isOk());

    vector<uint8_t> eKeyPair;
    ASSERT_TRUE(c->createEphemeralKeyPair(&eKeyPair).isOk());
    optional<vector<uint8_t>> ePublicKey = support::ecKeyPairGetPublicKey(eKeyPair);

    // Calculate requestData field and sign it with the reader key.
    auto [getXYSuccess, ephX, ephY] = support::ecPublicKeyGetXandY(ePublicKey.value());
    ASSERT_TRUE(getXYSuccess);
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", ephX).add("ephY", ephY);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    cppbor::Array sessionTranscript = cppbor::Array()
                                              .add(cppbor::SemanticTag(24, deviceEngagementBytes))
                                              .add(cppbor::SemanticTag(24, eReaderPubBytes));
    vector<uint8_t> sessionTranscriptBytes = sessionTranscript.encode();

    vector<uint8_t> itemsRequestBytes;
    if (leaveOutAccessibleToAllFromRequestMessage) {
        itemsRequestBytes =
                cppbor::Map("nameSpaces",
                            cppbor::Map().add("ns", cppbor::Map()
                                                            .add("Accessible by A", false)
                                                            .add("Accessible by A or B", false)
                                                            .add("Accessible by B", false)
                                                            .add("Accessible by C", false)
                                                            .add("Accessible by None", false)))
                        .encode();
    } else {
        itemsRequestBytes =
                cppbor::Map("nameSpaces",
                            cppbor::Map().add("ns", cppbor::Map()
                                                            .add("Accessible by A", false)
                                                            .add("Accessible by A or B", false)
                                                            .add("Accessible by B", false)
                                                            .add("Accessible by C", false)
                                                            .add("Accessible by All", false)
                                                            .add("Accessible by None", false)))
                        .encode();
    }
    vector<uint8_t> encodedReaderAuthentication =
            cppbor::Array()
                    .add("ReaderAuthentication")
                    .add(sessionTranscript.clone())
                    .add(cppbor::SemanticTag(24, itemsRequestBytes))
                    .encode();
    vector<uint8_t> encodedReaderAuthenticationBytes =
            cppbor::SemanticTag(24, encodedReaderAuthentication).encode();

    optional<vector<uint8_t>> readerSignature =
            support::coseSignEcDsa(readerPrivateKey,                  // private key for reader
                                   {},                                // content
                                   encodedReaderAuthenticationBytes,  // detached content
                                   support::certificateChainJoin(readerCertChain));
    ASSERT_TRUE(readerSignature);

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(c->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());

    RequestNamespace rns;
    rns.namespaceName = "ns";
    rns.items.push_back(buildRequestDataItem("Accessible by A", 1, {0}));
    rns.items.push_back(buildRequestDataItem("Accessible by A or B", 1, {0, 1}));
    rns.items.push_back(buildRequestDataItem("Accessible by B", 1, {1}));
    rns.items.push_back(buildRequestDataItem("Accessible by C", 1, {2}));
    rns.items.push_back(buildRequestDataItem("Accessible by All", 1, {3}));
    rns.items.push_back(buildRequestDataItem("Accessible by None", 1, {}));
    // OK to fail, not available in v1 HAL
    c->setRequestedNamespaces({rns}).isOk();

    // It doesn't matter since no user auth is needed in this particular test,
    // but for good measure, clear out the tokens we pass to the HAL.
    HardwareAuthToken authToken;
    VerificationToken verificationToken;
    authToken.challenge = 0;
    authToken.userId = 0;
    authToken.authenticatorId = 0;
    authToken.authenticatorType = ::android::hardware::keymaster::HardwareAuthenticatorType::NONE;
    authToken.timestamp.milliSeconds = 0;
    authToken.mac.clear();
    verificationToken.challenge = 0;
    verificationToken.timestamp.milliSeconds = 0;
    verificationToken.securityLevel = ::android::hardware::keymaster::SecurityLevel::SOFTWARE;
    verificationToken.mac.clear();
    // OK to fail, not available in v1 HAL
    c->setVerificationToken(verificationToken);

    Status status = c->startRetrieval(
            {sacp0_, sacp1_, sacp2_, sacp3_}, authToken, itemsRequestBytes, signingKeyBlob,
            sessionTranscriptBytes, readerSignature.value(), {6 /* numDataElementsPerNamespace */});
    if (expectSuccess) {
        ASSERT_TRUE(status.isOk());
    } else {
        ASSERT_FALSE(status.isOk());
        return;
    }

    vector<uint8_t> decrypted;

    status = c->startRetrieveEntryValue("ns", "Accessible by A", 1, {0});
    if (status.isOk()) {
        canGetAccessibleByA_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByA_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by A or B", 1, {0, 1});
    if (status.isOk()) {
        canGetAccessibleByAorB_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByAorB_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by B", 1, {1});
    if (status.isOk()) {
        canGetAccessibleByB_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByB_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by C", 1, {2});
    if (status.isOk()) {
        canGetAccessibleByC_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByC_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by All", 1, {3});
    if (status.isOk()) {
        canGetAccessibleByAll_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByAll_, &decrypted).isOk());
    }

    status = c->startRetrieveEntryValue("ns", "Accessible by None", 1, {});
    if (status.isOk()) {
        canGetAccessibleByNone_ = true;
        ASSERT_TRUE(c->retrieveEntryValue(encContentAccessibleByNone_, &decrypted).isOk());
    }

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpaces;
    ASSERT_TRUE(c->finishRetrieval(&mac, &deviceNameSpaces).isOk());
}

TEST_P(ReaderAuthTests, presentingChain_Reader) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_reader_SelfSigned_}, true /* expectSuccess */,
                 false /* leaveOutAccessibleToAllFromRequestMessage */);
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_FALSE(canGetAccessibleByAorB_);
    EXPECT_FALSE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(ReaderAuthTests, presentingChain_Reader_A) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_reader_SignedBy_A_, cert_A_SelfSigned_},
                 true /* expectSuccess */, false /* leaveOutAccessibleToAllFromRequestMessage */);
    EXPECT_TRUE(canGetAccessibleByA_);
    EXPECT_TRUE(canGetAccessibleByAorB_);
    EXPECT_FALSE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(ReaderAuthTests, presentingChain_Reader_B) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_reader_SignedBy_B_, cert_B_SelfSigned_},
                 true /* expectSuccess */, false /* leaveOutAccessibleToAllFromRequestMessage */);
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_TRUE(canGetAccessibleByAorB_);
    EXPECT_TRUE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

// This test proves that for the purpose of determining inclusion of an ACP certificate
// in a presented reader chain, certificate equality is done by comparing public keys,
// not bitwise comparison of the certificates.
//
// Specifically for this test, the ACP is configured with cert_B_SelfSigned_ and the
// reader is presenting cert_B_SignedBy_C_. Both certificates have the same public
// key - intermediateBPublicKey_ - but they are signed by different keys.
//
TEST_P(ReaderAuthTests, presentingChain_Reader_B_C) {
    provisionData();
    retrieveData(readerPrivateKey_,
                 {cert_reader_SignedBy_B_, cert_B_SignedBy_C_, cert_C_SelfSigned_},
                 true /* expectSuccess */, false /* leaveOutAccessibleToAllFromRequestMessage */);
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_TRUE(canGetAccessibleByAorB_);
    EXPECT_TRUE(canGetAccessibleByB_);
    EXPECT_TRUE(canGetAccessibleByC_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

// This test presents a reader chain where the chain is invalid because
// the 2nd certificate in the chain isn't signed by the 3rd one.
//
TEST_P(ReaderAuthTests, presentingInvalidChain) {
    provisionData();
    retrieveData(readerPrivateKey_,
                 {cert_reader_SignedBy_B_, cert_B_SelfSigned_, cert_C_SelfSigned_},
                 false /* expectSuccess */, false /* leaveOutAccessibleToAllFromRequestMessage */);
}

// This tests presents a valid reader chain but where requestMessage isn't
// signed by the private key corresponding to the public key in the top-level
// certificate.
//
TEST_P(ReaderAuthTests, presentingMessageSignedNotByTopLevel) {
    provisionData();
    retrieveData(intermediateBPrivateKey_,
                 {cert_reader_SignedBy_B_, cert_B_SignedBy_C_, cert_C_SelfSigned_},
                 false /* expectSuccess */, false /* leaveOutAccessibleToAllFromRequestMessage */);
}

// This test leaves out "Accessible by All" data element from the signed request
// message (the CBOR from the reader) while still including this data element at
// the API level. The call on the API level for said element will fail with
// STATUS_NOT_IN_REQUEST_MESSAGE but this doesn't prevent the other elements
// from being returned (if authorized, of course).
//
// This test verifies that.
//
TEST_P(ReaderAuthTests, limitedMessage) {
    provisionData();
    retrieveData(readerPrivateKey_, {cert_reader_SelfSigned_}, true /* expectSuccess */,
                 true /* leaveOutAccessibleToAllFromRequestMessage */);
    EXPECT_FALSE(canGetAccessibleByA_);
    EXPECT_FALSE(canGetAccessibleByAorB_);
    EXPECT_FALSE(canGetAccessibleByB_);
    EXPECT_FALSE(canGetAccessibleByC_);
    EXPECT_FALSE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(ReaderAuthTests, ephemeralKeyNotInSessionTranscript) {
    provisionData();

    sp<IIdentityCredential> c;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &c)
                        .isOk());

    optional<vector<uint8_t>> readerEKeyPair = support::createEcKeyPair();
    optional<vector<uint8_t>> readerEPublicKey =
            support::ecKeyPairGetPublicKey(readerEKeyPair.value());
    ASSERT_TRUE(c->setReaderEphemeralPublicKey(readerEPublicKey.value()).isOk());

    vector<uint8_t> eKeyPair;
    ASSERT_TRUE(c->createEphemeralKeyPair(&eKeyPair).isOk());
    optional<vector<uint8_t>> ePublicKey = support::ecKeyPairGetPublicKey(eKeyPair);

    // Calculate requestData field and sign it with the reader key.
    auto [getXYSuccess, ephX, ephY] = support::ecPublicKeyGetXandY(ePublicKey.value());
    ASSERT_TRUE(getXYSuccess);
    // Instead of include the X and Y coordinates (|ephX| and |ephY|), add NUL bytes instead.
    vector<uint8_t> nulls(32);
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", nulls).add("ephY", nulls);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    cppbor::Array sessionTranscript = cppbor::Array()
                                              .add(cppbor::SemanticTag(24, deviceEngagementBytes))
                                              .add(cppbor::SemanticTag(24, eReaderPubBytes));
    vector<uint8_t> sessionTranscriptBytes = sessionTranscript.encode();

    vector<uint8_t> itemsRequestBytes;
    itemsRequestBytes =
            cppbor::Map("nameSpaces",
                        cppbor::Map().add("ns", cppbor::Map()
                                                        .add("Accessible by A", false)
                                                        .add("Accessible by A or B", false)
                                                        .add("Accessible by B", false)
                                                        .add("Accessible by C", false)
                                                        .add("Accessible by None", false)))
                    .encode();
    vector<uint8_t> encodedReaderAuthentication =
            cppbor::Array()
                    .add("ReaderAuthentication")
                    .add(sessionTranscript.clone())
                    .add(cppbor::SemanticTag(24, itemsRequestBytes))
                    .encode();
    vector<uint8_t> encodedReaderAuthenticationBytes =
            cppbor::SemanticTag(24, encodedReaderAuthentication).encode();

    vector<vector<uint8_t>> readerCertChain = {cert_reader_SelfSigned_};
    optional<vector<uint8_t>> readerSignature =
            support::coseSignEcDsa(readerPrivateKey_,                 // private key for reader
                                   {},                                // content
                                   encodedReaderAuthenticationBytes,  // detached content
                                   support::certificateChainJoin(readerCertChain));
    ASSERT_TRUE(readerSignature);

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(c->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());

    RequestNamespace rns;
    rns.namespaceName = "ns";
    rns.items.push_back(buildRequestDataItem("Accessible by A", 1, {0}));
    rns.items.push_back(buildRequestDataItem("Accessible by A or B", 1, {0, 1}));
    rns.items.push_back(buildRequestDataItem("Accessible by B", 1, {1}));
    rns.items.push_back(buildRequestDataItem("Accessible by C", 1, {2}));
    rns.items.push_back(buildRequestDataItem("Accessible by All", 1, {3}));
    rns.items.push_back(buildRequestDataItem("Accessible by None", 1, {}));
    // OK to fail, not available in v1 HAL
    c->setRequestedNamespaces({rns}).isOk();

    // It doesn't matter since no user auth is needed in this particular test,
    // but for good measure, clear out the tokens we pass to the HAL.
    HardwareAuthToken authToken;
    VerificationToken verificationToken;
    authToken.challenge = 0;
    authToken.userId = 0;
    authToken.authenticatorId = 0;
    authToken.authenticatorType = ::android::hardware::keymaster::HardwareAuthenticatorType::NONE;
    authToken.timestamp.milliSeconds = 0;
    authToken.mac.clear();
    verificationToken.challenge = 0;
    verificationToken.timestamp.milliSeconds = 0;
    verificationToken.securityLevel =
            ::android::hardware::keymaster::SecurityLevel::TRUSTED_ENVIRONMENT;
    verificationToken.mac.clear();
    // OK to fail, not available in v1 HAL
    c->setVerificationToken(verificationToken);

    // Finally check that STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND is returned.
    // This proves that the TA checked for X and Y coordinatets and didn't find
    // them.
    Status status = c->startRetrieval(
            {sacp0_, sacp1_, sacp2_, sacp3_}, authToken, itemsRequestBytes, signingKeyBlob,
            sessionTranscriptBytes, readerSignature.value(), {6 /* numDataElementsPerNamespace */});
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(binder::Status::EX_SERVICE_SPECIFIC, status.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND,
              status.serviceSpecificErrorCode());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ReaderAuthTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, ReaderAuthTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
