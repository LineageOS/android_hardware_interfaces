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

class TestCredentialTests : public testing::TestWithParam<string> {
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

TEST_P(TestCredentialTests, testCredential) {
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

    // Now analyze credentialData..
    auto [item, _, message] = cppbor::parse(credentialData);
    ASSERT_NE(item, nullptr);
    const cppbor::Array* arrayItem = item->asArray();
    ASSERT_NE(arrayItem, nullptr);
    ASSERT_EQ(arrayItem->size(), 3);
    const cppbor::Tstr* docTypeItem = (*arrayItem)[0]->asTstr();
    const cppbor::Bool* testCredentialItem =
            ((*arrayItem)[1]->asSimple() != nullptr ? ((*arrayItem)[1]->asSimple()->asBool())
                                                    : nullptr);
    EXPECT_EQ(docTypeItem->value(), docType);
    EXPECT_EQ(testCredentialItem->value(), true);

    vector<uint8_t> hardwareBoundKey = support::getTestHardwareBoundKey();
    const cppbor::Bstr* encryptedCredentialKeysItem = (*arrayItem)[2]->asBstr();
    const vector<uint8_t>& encryptedCredentialKeys = encryptedCredentialKeysItem->value();
    const vector<uint8_t> docTypeVec(docType.begin(), docType.end());
    optional<vector<uint8_t>> decryptedCredentialKeys =
            support::decryptAes128Gcm(hardwareBoundKey, encryptedCredentialKeys, docTypeVec);
    ASSERT_TRUE(decryptedCredentialKeys);
    auto [dckItem, dckPos, dckMessage] = cppbor::parse(decryptedCredentialKeys.value());
    ASSERT_NE(dckItem, nullptr) << dckMessage;
    const cppbor::Array* dckArrayItem = dckItem->asArray();
    ASSERT_NE(dckArrayItem, nullptr);
    // In HAL API version 1 and 2 this array has two items, in version 3 and later it has three.
    if (halApiVersion_ < 3) {
        ASSERT_EQ(dckArrayItem->size(), 2);
    } else {
        ASSERT_EQ(dckArrayItem->size(), 3);
    }
    const cppbor::Bstr* storageKeyItem = (*dckArrayItem)[0]->asBstr();
    const vector<uint8_t> storageKey = storageKeyItem->value();
    // const cppbor::Bstr* credentialPrivKeyItem = (*dckArrayItem)[1]->asBstr();
    // const vector<uint8_t> credentialPrivKey = credentialPrivKeyItem->value();

    // Check storageKey can be used to decrypt |encryptedData| to |tstrLastName|
    vector<uint8_t> additionalData = cppbor::Map()
                                             .add("Namespace", "ns")
                                             .add("Name", "Last name")
                                             .add("AccessControlProfileIds", cppbor::Array().add(1))
                                             .encode();
    optional<vector<uint8_t>> decryptedDataItemValue =
            support::decryptAes128Gcm(storageKey, encryptedData, additionalData);
    ASSERT_TRUE(decryptedDataItemValue);
    EXPECT_EQ(decryptedDataItemValue.value(), tstrLastName);

    // Check that SHA-256(ProofOfProvisioning) matches (only in HAL API version 3)
    if (halApiVersion_ >= 3) {
        const cppbor::Bstr* popSha256Item = (*dckArrayItem)[2]->asBstr();
        const vector<uint8_t> popSha256 = popSha256Item->value();
        ASSERT_EQ(popSha256, support::sha256(proofOfProvisioning.value()));
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(TestCredentialTests);
INSTANTIATE_TEST_SUITE_P(
        Identity, TestCredentialTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
