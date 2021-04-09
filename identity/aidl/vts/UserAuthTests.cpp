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

#define LOG_TAG "UserAuthTests"

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

class UserAuthTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    void provisionData();
    void setupRetrieveData();
    pair<HardwareAuthToken, VerificationToken> mintTokens(uint64_t challengeForAuthToken,
                                                          int64_t ageOfAuthTokenMilliSeconds);
    void retrieveData(HardwareAuthToken authToken, VerificationToken verificationToken,
                      bool expectSuccess, bool useSessionTranscript);

    // Set by provisionData
    SecureAccessControlProfile sacp0_;
    SecureAccessControlProfile sacp1_;
    SecureAccessControlProfile sacp2_;

    vector<uint8_t> encContentUserAuthPerSession_;
    vector<uint8_t> encContentUserAuthTimeout_;
    vector<uint8_t> encContentAccessibleByAll_;
    vector<uint8_t> encContentAccessibleByNone_;

    vector<uint8_t> credentialData_;

    // Set by setupRetrieveData().
    int64_t authChallenge_;
    cppbor::Map sessionTranscript_;
    sp<IIdentityCredential> credential_;

    // Set by retrieveData()
    bool canGetUserAuthPerSession_;
    bool canGetUserAuthTimeout_;
    bool canGetAccessibleByAll_;
    bool canGetAccessibleByNone_;

    sp<IIdentityCredentialStore> credentialStore_;
};

void UserAuthTests::provisionData() {
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

    size_t proofOfProvisioningSize = 381;
    // Not in v1 HAL, may fail
    wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize);

    ASSERT_TRUE(wc->startPersonalization(3 /* numAccessControlProfiles */,
                                         {4} /* numDataElementsPerNamespace */)
                        .isOk());

    // Access control profile 0: user auth every session (timeout = 0)
    ASSERT_TRUE(wc->addAccessControlProfile(0, {}, true, 0, 65 /* secureUserId */, &sacp0_).isOk());

    // Access control profile 1: user auth, 60 seconds timeout
    ASSERT_TRUE(
            wc->addAccessControlProfile(1, {}, true, 60000, 65 /* secureUserId */, &sacp1_).isOk());

    // Access control profile 2: open access
    ASSERT_TRUE(wc->addAccessControlProfile(2, {}, false, 0, 0, &sacp2_).isOk());

    // Data Element: "UserAuth Per Session"
    ASSERT_TRUE(wc->beginAddEntry({0}, "ns", "UserAuth Per Session", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentUserAuthPerSession_).isOk());

    // Data Element: "UserAuth Timeout"
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "UserAuth Timeout", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentUserAuthTimeout_).isOk());

    // Data Element: "Accessible by All"
    ASSERT_TRUE(wc->beginAddEntry({2}, "ns", "Accessible by All", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByAll_).isOk());

    // Data Element: "Accessible by None"
    ASSERT_TRUE(wc->beginAddEntry({}, "ns", "Accessible by None", 1).isOk());
    ASSERT_TRUE(wc->addEntryValue({9}, &encContentAccessibleByNone_).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    Status status = wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
}

// From ReaderAuthTest.cpp - TODO: consolidate with Util.h
pair<vector<uint8_t>, vector<uint8_t>> generateReaderKey();
vector<uint8_t> generateReaderCert(const vector<uint8_t>& publicKey,
                                   const vector<uint8_t>& signingKey);
RequestDataItem buildRequestDataItem(const string& name, size_t size,
                                     vector<int32_t> accessControlProfileIds);

cppbor::Map calcSessionTranscript(const vector<uint8_t>& ePublicKey) {
    auto [getXYSuccess, ephX, ephY] = support::ecPublicKeyGetXandY(ePublicKey);
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", ephX).add("ephY", ephY);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    // Let SessionTranscript be a map here (it's an array in EndToEndTest) just
    // to check that the implementation can deal with either.
    cppbor::Map sessionTranscript;
    sessionTranscript.add(42, cppbor::SemanticTag(24, deviceEngagementBytes));
    sessionTranscript.add(43, cppbor::SemanticTag(24, eReaderPubBytes));
    return sessionTranscript;
}

void UserAuthTests::setupRetrieveData() {
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &credential_)
                        .isOk());

    optional<vector<uint8_t>> readerEKeyPair = support::createEcKeyPair();
    optional<vector<uint8_t>> readerEPublicKey =
            support::ecKeyPairGetPublicKey(readerEKeyPair.value());
    ASSERT_TRUE(credential_->setReaderEphemeralPublicKey(readerEPublicKey.value()).isOk());

    vector<uint8_t> eKeyPair;
    ASSERT_TRUE(credential_->createEphemeralKeyPair(&eKeyPair).isOk());
    optional<vector<uint8_t>> ePublicKey = support::ecKeyPairGetPublicKey(eKeyPair);
    sessionTranscript_ = calcSessionTranscript(ePublicKey.value());

    Status status = credential_->createAuthChallenge(&authChallenge_);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
}

void UserAuthTests::retrieveData(HardwareAuthToken authToken, VerificationToken verificationToken,
                                 bool expectSuccess, bool useSessionTranscript) {
    canGetUserAuthPerSession_ = false;
    canGetUserAuthTimeout_ = false;
    canGetAccessibleByAll_ = false;
    canGetAccessibleByNone_ = false;

    vector<uint8_t> itemsRequestBytes;
    vector<uint8_t> sessionTranscriptBytes;
    if (useSessionTranscript) {
        sessionTranscriptBytes = sessionTranscript_.encode();

        itemsRequestBytes =
                cppbor::Map("nameSpaces",
                            cppbor::Map().add("ns", cppbor::Map()
                                                            .add("UserAuth Per Session", false)
                                                            .add("UserAuth Timeout", false)
                                                            .add("Accessible by All", false)
                                                            .add("Accessible by None", false)))
                        .encode();
        vector<uint8_t> dataToSign = cppbor::Array()
                                             .add("ReaderAuthentication")
                                             .add(sessionTranscript_.clone())
                                             .add(cppbor::SemanticTag(24, itemsRequestBytes))
                                             .encode();
    }

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(
            credential_->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());

    RequestNamespace rns;
    rns.namespaceName = "ns";
    rns.items.push_back(buildRequestDataItem("UserAuth Per Session", 1, {0}));
    rns.items.push_back(buildRequestDataItem("UserAuth Timeout", 1, {1}));
    rns.items.push_back(buildRequestDataItem("Accessible by All", 1, {2}));
    rns.items.push_back(buildRequestDataItem("Accessible by None", 1, {}));
    // OK to fail, not available in v1 HAL
    credential_->setRequestedNamespaces({rns}).isOk();

    // OK to fail, not available in v1 HAL
    credential_->setVerificationToken(verificationToken);

    Status status = credential_->startRetrieval({sacp0_, sacp1_, sacp2_}, authToken,
                                                itemsRequestBytes, signingKeyBlob,
                                                sessionTranscriptBytes, {} /* readerSignature */,
                                                {4 /* numDataElementsPerNamespace */});
    if (expectSuccess) {
        ASSERT_TRUE(status.isOk());
    } else {
        ASSERT_FALSE(status.isOk());
        return;
    }

    vector<uint8_t> decrypted;

    status = credential_->startRetrieveEntryValue("ns", "UserAuth Per Session", 1, {0});
    if (status.isOk()) {
        canGetUserAuthPerSession_ = true;
        ASSERT_TRUE(
                credential_->retrieveEntryValue(encContentUserAuthPerSession_, &decrypted).isOk());
    }

    status = credential_->startRetrieveEntryValue("ns", "UserAuth Timeout", 1, {1});
    if (status.isOk()) {
        canGetUserAuthTimeout_ = true;
        ASSERT_TRUE(credential_->retrieveEntryValue(encContentUserAuthTimeout_, &decrypted).isOk());
    }

    status = credential_->startRetrieveEntryValue("ns", "Accessible by All", 1, {2});
    if (status.isOk()) {
        canGetAccessibleByAll_ = true;
        ASSERT_TRUE(credential_->retrieveEntryValue(encContentAccessibleByAll_, &decrypted).isOk());
    }

    status = credential_->startRetrieveEntryValue("ns", "Accessible by None", 1, {});
    if (status.isOk()) {
        canGetAccessibleByNone_ = true;
        ASSERT_TRUE(
                credential_->retrieveEntryValue(encContentAccessibleByNone_, &decrypted).isOk());
    }

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpaces;
    ASSERT_TRUE(credential_->finishRetrieval(&mac, &deviceNameSpaces).isOk());
}

pair<HardwareAuthToken, VerificationToken> UserAuthTests::mintTokens(
        uint64_t challengeForAuthToken, int64_t ageOfAuthTokenMilliSeconds) {
    HardwareAuthToken authToken;
    VerificationToken verificationToken;

    uint64_t epochMilliseconds = 1000ULL * 1000ULL * 1000ULL * 1000ULL;

    authToken.challenge = challengeForAuthToken;
    authToken.userId = 65;
    authToken.authenticatorId = 0;
    authToken.authenticatorType = ::android::hardware::keymaster::HardwareAuthenticatorType::NONE;
    authToken.timestamp.milliSeconds = epochMilliseconds - ageOfAuthTokenMilliSeconds;
    authToken.mac.clear();
    verificationToken.challenge = authChallenge_;
    verificationToken.timestamp.milliSeconds = epochMilliseconds;
    verificationToken.securityLevel =
            ::android::hardware::keymaster::SecurityLevel::TRUSTED_ENVIRONMENT;
    verificationToken.mac.clear();
    return make_pair(authToken, verificationToken);
}

TEST_P(UserAuthTests, GoodChallenge) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(authChallenge_,  // challengeForAuthToken
                                                     0);              // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_TRUE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, OtherChallenge) {
    provisionData();
    setupRetrieveData();
    uint64_t otherChallenge = authChallenge_ ^ 0x12345678;
    auto [authToken, verificationToken] = mintTokens(otherChallenge,  // challengeForAuthToken
                                                     0);              // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, NoChallenge) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,   // challengeForAuthToken
                                                     0);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, AuthTokenAgeZero) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,   // challengeForAuthToken
                                                     0);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, AuthTokenFromTheFuture) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,           // challengeForAuthToken
                                                     -1 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_FALSE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, AuthTokenInsideTimeout) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,           // challengeForAuthToken
                                                     30 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

TEST_P(UserAuthTests, AuthTokenOutsideTimeout) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,           // challengeForAuthToken
                                                     61 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_FALSE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

// The API works even when there's no SessionTranscript / itemsRequest.
// Verify that.
TEST_P(UserAuthTests, NoSessionTranscript) {
    provisionData();
    setupRetrieveData();
    auto [authToken, verificationToken] = mintTokens(0,          // challengeForAuthToken
                                                     1 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 false /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

// This test verifies that it's possible to do multiple requests as long
// as the sessionTranscript doesn't change.
//
TEST_P(UserAuthTests, MultipleRequestsSameSessionTranscript) {
    provisionData();
    setupRetrieveData();

    // First we try with a stale authToken
    //
    auto [authToken, verificationToken] = mintTokens(0,           // challengeForAuthToken
                                                     61 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_FALSE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);

    // Then we get a new authToken and try again.
    tie(authToken, verificationToken) = mintTokens(0,          // challengeForAuthToken
                                                   5 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_TRUE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);
}

// Like MultipleRequestsSameSessionTranscript but we change the sessionTranscript
// between the two calls. This test verifies that change is detected and the
// second request fails.
//
TEST_P(UserAuthTests, MultipleRequestsSessionTranscriptChanges) {
    provisionData();
    setupRetrieveData();

    // First we try with a stale authToken
    //
    auto [authToken, verificationToken] = mintTokens(0,           // challengeForAuthToken
                                                     61 * 1000);  // ageOfAuthTokenMilliSeconds
    retrieveData(authToken, verificationToken, true /* expectSuccess */,
                 true /* useSessionTranscript */);
    EXPECT_FALSE(canGetUserAuthPerSession_);
    EXPECT_FALSE(canGetUserAuthTimeout_);
    EXPECT_TRUE(canGetAccessibleByAll_);
    EXPECT_FALSE(canGetAccessibleByNone_);

    // Then we get a new authToken and try again.
    tie(authToken, verificationToken) = mintTokens(0,          // challengeForAuthToken
                                                   5 * 1000);  // ageOfAuthTokenMilliSeconds

    // Change sessionTranscript...
    optional<vector<uint8_t>> eKeyPairNew = support::createEcKeyPair();
    optional<vector<uint8_t>> ePublicKeyNew = support::ecKeyPairGetPublicKey(eKeyPairNew.value());
    sessionTranscript_ = calcSessionTranscript(ePublicKeyNew.value());

    // ... and expect failure.
    retrieveData(authToken, verificationToken, false /* expectSuccess */,
                 true /* useSessionTranscript */);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(UserAuthTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, UserAuthTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
