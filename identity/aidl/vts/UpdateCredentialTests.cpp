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

#define LOG_TAG "UpdateCredentialTests"

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

class UpdateCredentialTests : public testing::TestWithParam<string> {
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

void UpdateCredentialTests::provisionData() {
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

    size_t proofOfProvisioningSize = 112;
    // Not in v1 HAL, may fail
    wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize);

    ASSERT_TRUE(wc->startPersonalization(1 /* numAccessControlProfiles */,
                                         {1} /* numDataElementsPerNamespace */)
                        .isOk());

    // Access control profile 0: open access - don't care about the returned SACP
    SecureAccessControlProfile sacp;
    ASSERT_TRUE(wc->addAccessControlProfile(1, {}, false, 0, 0, &sacp).isOk());

    // Single entry - don't care about the returned encrypted data
    vector<uint8_t> encryptedData;
    vector<uint8_t> tstrLastName = cppbor::Tstr("Prince").encode();
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "Last name", tstrLastName.size()).isOk());
    ASSERT_TRUE(wc->addEntryValue(tstrLastName, &encryptedData).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    Status status = wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty = cppbor::prettyPrint(proofOfProvisioning.value(), 32, {});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 1,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'ns' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Prince',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);
    // Make sure it's signed by the CredentialKey in the returned cert chain.
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey_));
}

TEST_P(UpdateCredentialTests, updateCredential) {
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

    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credential->updateCredential(&wc).isOk());

    // Getting an attestation cert should fail (because it's an update).
    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    Status result = wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                                  &certChain);
    ASSERT_FALSE(result.isOk());
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());

    // Now provision some new data...
    //
    size_t proofOfProvisioningSize = 117;
    // Not in v1 HAL, may fail
    wc->setExpectedProofOfProvisioningSize(proofOfProvisioningSize);

    ASSERT_TRUE(wc->startPersonalization(1 /* numAccessControlProfiles */,
                                         {1} /* numDataElementsPerNamespace */)
                        .isOk());

    // Access control profile 0: open access - don't care about the returned SACP
    SecureAccessControlProfile sacp;
    ASSERT_TRUE(wc->addAccessControlProfile(2, {}, false, 0, 0, &sacp).isOk());

    // Single entry - don't care about the returned encrypted data
    vector<uint8_t> encryptedData;
    vector<uint8_t> tstrLastName = cppbor::Tstr("T.A.F.K.A.P").encode();
    ASSERT_TRUE(wc->beginAddEntry({2}, "ns", "Last name", tstrLastName.size()).isOk());
    ASSERT_TRUE(wc->addEntryValue(tstrLastName, &encryptedData).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    Status status = wc->finishAddingEntries(&credentialData_, &proofOfProvisioningSignature);
    EXPECT_TRUE(status.isOk()) << status.exceptionCode() << ": " << status.exceptionMessage();
    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty = cppbor::prettyPrint(proofOfProvisioning.value(), 32, {});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 2,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'ns' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'T.A.F.K.A.P',\n"
            "        'accessControlProfiles' : [2, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);
    // Make sure it's signed by the same CredentialKey we originally provisioned with.
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey_));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(UpdateCredentialTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, UpdateCredentialTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
