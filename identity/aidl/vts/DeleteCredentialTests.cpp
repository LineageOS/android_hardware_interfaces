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

#define LOG_TAG "DeleteCredentialTests"

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

class DeleteCredentialTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
        halApiVersion_ = credentialStore_->getInterfaceVersion();
    }

    void provisionData();

    // Set by provisionData
    vector<uint8_t> credentialData_;
    vector<uint8_t> credentialPubKey_;

    sp<IIdentityCredentialStore> credentialStore_;
    int halApiVersion_;
};

void DeleteCredentialTests::provisionData() {
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

    optional<vector<uint8_t>> optCredentialPubKey =
            support::certificateChainGetTopMostKey(certChain[0].encodedCertificate);
    ASSERT_TRUE(optCredentialPubKey);
    credentialPubKey_ = optCredentialPubKey.value();

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
    Status status = wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
}

TEST_P(DeleteCredentialTests, Delete) {
    provisionData();

    sp<IIdentityCredential> credential;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &credential)
                        .isOk());

    vector<uint8_t> proofOfDeletionSignature;
    ASSERT_TRUE(credential->deleteCredential(&proofOfDeletionSignature).isOk());
    optional<vector<uint8_t>> proofOfDeletion =
            support::coseSignGetPayload(proofOfDeletionSignature);
    ASSERT_TRUE(proofOfDeletion);
    string cborPretty = cppbor::prettyPrint(proofOfDeletion.value(), 32, {});
    EXPECT_EQ("['ProofOfDeletion', 'org.iso.18013-5.2019.mdl', true, ]", cborPretty);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfDeletionSignature, {},  // Additional data
                                                 credentialPubKey_));
}

TEST_P(DeleteCredentialTests, DeleteWithChallenge) {
    if (halApiVersion_ < 3) {
        GTEST_SKIP() << "Need HAL API version 3, have " << halApiVersion_;
    }

    provisionData();

    sp<IIdentityCredential> credential;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData_, &credential)
                        .isOk());

    vector<uint8_t> challenge = {65, 66, 67};
    vector<uint8_t> proofOfDeletionSignature;
    ASSERT_TRUE(
            credential->deleteCredentialWithChallenge(challenge, &proofOfDeletionSignature).isOk());
    optional<vector<uint8_t>> proofOfDeletion =
            support::coseSignGetPayload(proofOfDeletionSignature);
    ASSERT_TRUE(proofOfDeletion);
    string cborPretty = cppbor::prettyPrint(proofOfDeletion.value(), 32, {});
    EXPECT_EQ("['ProofOfDeletion', 'org.iso.18013-5.2019.mdl', {0x41, 0x42, 0x43}, true, ]",
              cborPretty);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfDeletionSignature, {},  // Additional data
                                                 credentialPubKey_));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DeleteCredentialTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, DeleteCredentialTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
