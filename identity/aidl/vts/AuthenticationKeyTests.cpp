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

#define LOG_TAG "TestCredentialTests"

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

class AuthenticationKeyTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        string halInstanceName = GetParam();
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(halInstanceName.c_str()));
        ASSERT_NE(credentialStore_, nullptr);
        halApiVersion_ = credentialStore_->getInterfaceVersion();
    }

    sp<IIdentityCredentialStore> credentialStore_;
    int halApiVersion_;
};

TEST_P(AuthenticationKeyTests, proofOfProvisionInAuthKeyCert) {
    if (halApiVersion_ < 3) {
        GTEST_SKIP() << "Need HAL API version 3, have " << halApiVersion_;
    }

    string docType = "org.iso.18013-5.2019.mdl";
    sp<IWritableIdentityCredential> wc;
    ASSERT_TRUE(credentialStore_
                        ->createCredential(docType,
                                           true,  // testCredential
                                           &wc)
                        .isOk());

    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge = {1};
    vector<Certificate> certChain;
    ASSERT_TRUE(wc->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                              &certChain)
                        .isOk());

    optional<vector<uint8_t>> optCredentialPubKey =
            support::certificateChainGetTopMostKey(certChain[0].encodedCertificate);
    ASSERT_TRUE(optCredentialPubKey);
    vector<uint8_t> credentialPubKey;
    credentialPubKey = optCredentialPubKey.value();

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
    vector<uint8_t> tstrLastName = cppbor::Tstr("Turing").encode();
    ASSERT_TRUE(wc->beginAddEntry({1}, "ns", "Last name", tstrLastName.size()).isOk());
    ASSERT_TRUE(wc->addEntryValue(tstrLastName, &encryptedData).isOk());

    vector<uint8_t> proofOfProvisioningSignature;
    vector<uint8_t> credentialData;
    Status status = wc->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);
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
            "        'value' : 'Turing',\n"
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
                                                 credentialPubKey));

    // Now get a credential and have it create AuthenticationKey so we can check
    // the certificate.
    sp<IIdentityCredential> credential;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData, &credential)
                        .isOk());
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(credential->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());
    optional<vector<uint8_t>> signingPubKey =
            support::certificateChainGetTopMostKey(signingKeyCertificate.encodedCertificate);
    EXPECT_TRUE(signingPubKey);

    // SHA-256(ProofOfProvisioning) is embedded in CBOR with the following CDDL
    //
    //   ProofOfBinding = [
    //     "ProofOfBinding",
    //     bstr,                  // Contains the SHA-256 of ProofOfProvisioning
    //   ]
    //
    // Check that.
    //
    optional<vector<uint8_t>> proofOfBinding = support::certificateGetExtension(
            signingKeyCertificate.encodedCertificate, "1.3.6.1.4.1.11129.2.1.26");
    ASSERT_TRUE(proofOfBinding);
    auto [item, _, message] = cppbor::parse(proofOfBinding.value());
    ASSERT_NE(item, nullptr) << message;
    const cppbor::Array* arrayItem = item->asArray();
    ASSERT_NE(arrayItem, nullptr);
    ASSERT_EQ(arrayItem->size(), 2);
    const cppbor::Tstr* strItem = (*arrayItem)[0]->asTstr();
    ASSERT_NE(strItem, nullptr);
    EXPECT_EQ(strItem->value(), "ProofOfBinding");
    const cppbor::Bstr* popSha256Item = (*arrayItem)[1]->asBstr();
    ASSERT_NE(popSha256Item, nullptr);
    EXPECT_EQ(popSha256Item->value(), support::sha256(proofOfProvisioning.value()));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AuthenticationKeyTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, AuthenticationKeyTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
