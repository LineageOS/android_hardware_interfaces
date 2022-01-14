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

#define LOG_TAG "PresentationSessionTests"

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

class PresentationSessionTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
        halApiVersion_ = credentialStore_->getInterfaceVersion();
    }

    void provisionData();

    void provisionSingleDocument(const string& docType, vector<uint8_t>* outCredentialData,
                                 vector<uint8_t>* outCredentialPubKey);

    // Set by provisionData
    vector<uint8_t> credential1Data_;
    vector<uint8_t> credential1PubKey_;
    vector<uint8_t> credential2Data_;
    vector<uint8_t> credential2PubKey_;

    sp<IIdentityCredentialStore> credentialStore_;
    int halApiVersion_;
};

void PresentationSessionTests::provisionData() {
    provisionSingleDocument("org.iso.18013-5.2019.mdl", &credential1Data_, &credential1PubKey_);
    provisionSingleDocument("org.blah.OtherhDocTypeXX", &credential2Data_, &credential2PubKey_);
}

void PresentationSessionTests::provisionSingleDocument(const string& docType,
                                                       vector<uint8_t>* outCredentialData,
                                                       vector<uint8_t>* outCredentialPubKey) {
    bool testCredential = true;
    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credentialStore_->createCredential(docType, testCredential, &wc).isOk());

    vector<uint8_t> attestationApplicationId;
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    ASSERT_TRUE(wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                              &certChain)
                        .isOk());

    optional<vector<uint8_t>> optCredentialPubKey =
            support::certificateChainGetTopMostKey(certChain[0].encodedCertificate);
    ASSERT_TRUE(optCredentialPubKey);
    *outCredentialPubKey = optCredentialPubKey.value();

    size_t proofOfProvisioningSize = 106;
    // Not in v1 HAL, may fail
    wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize);

    ASSERT_TRUE(wc->startPersonalization(1 /* numAccessControlProfiles */,
                                         {1} /* numDataElementsPerNamespace */)
                        .isOk());

    // Access control profile 0: open access - don't care about the returned SACP
    SecureAccessControlProfile sacp;
    ASSERT_TRUE(wc->addAccessControlProfile(1, {}, false, 0, 0, &sacp).isOk());

    // Single entry - don't care about the returned encrypted data
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "Some Data", 1).isOk());
    vector<uint8_t> encryptedData;
    ASSERT_TRUE(wc->addEntryValue({9}, &encryptedData).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    Status status = wc->finishAddingEntries(outCredentialData, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
}

// This checks that any methods called on an IIdentityCredential obtained via a session
// returns STATUS_FAILED except for startRetrieval(), startRetrieveEntryValue(),
// retrieveEntryValue(), finishRetrieval(), setRequestedNamespaces(), setVerificationToken()
//
TEST_P(PresentationSessionTests, returnsFailureOnUnsupportedMethods) {
    if (halApiVersion_ < 4) {
        GTEST_SKIP() << "Need HAL API version 4, have " << halApiVersion_;
    }

    provisionData();

    sp<IPresentationSession> session;
    ASSERT_TRUE(credentialStore_
                        ->createPresentationSession(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                &session)
                        .isOk());

    sp<IIdentityCredential> credential;
    ASSERT_TRUE(session->getCredential(credential1Data_, &credential).isOk());

    Status result;

    vector<uint8_t> signatureProofOfDeletion;
    result = credential->deleteCredential(&signatureProofOfDeletion);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    vector<uint8_t> ephemeralKeyPair;
    result = credential->createEphemeralKeyPair(&ephemeralKeyPair);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    result = credential->setReaderEphemeralPublicKey({});
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    int64_t authChallenge;
    result = credential->createAuthChallenge(&authChallenge);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    Certificate certificate;
    vector<uint8_t> signingKeyBlob;
    result = credential->generateSigningKeyPair(&signingKeyBlob, &certificate);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    result = credential->deleteCredentialWithChallenge({}, &signatureProofOfDeletion);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    vector<uint8_t> signatureProofOfOwnership;
    result = credential->proveOwnership({}, &signatureProofOfOwnership);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    sp<IWritableIdentityCredential> writableCredential;
    result = credential->updateCredential(&writableCredential);
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());
}

// TODO: need to add tests to check that the returned IIdentityCredential works
//       as intended.

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PresentationSessionTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, PresentationSessionTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
