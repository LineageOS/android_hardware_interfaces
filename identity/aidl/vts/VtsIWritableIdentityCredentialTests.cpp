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

#define LOG_TAG "VtsIWritableIdentityCredentialTests"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
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

#include "VtsIdentityTestUtils.h"

namespace android::hardware::identity {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

class IdentityCredentialTests : public testing::TestWithParam<string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_P(IdentityCredentialTests, verifyAttestationWithEmptyChallenge) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    vector<uint8_t> attestationChallenge;
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {};
    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA, result.serviceSpecificErrorCode());
}

TEST_P(IdentityCredentialTests, verifyAttestationSuccessWithChallenge) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge1NotSoRandomChallenge1NotSoRandomChallenge1";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificate;
    vector<uint8_t> attestationApplicationId = {1};

    result = writableCredential->getAttestationCertificate(
            attestationApplicationId, attestationChallenge, &attestationCertificate);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    test_utils::validateAttestationCertificate(attestationCertificate, attestationChallenge,
                                               attestationApplicationId, false);
}

TEST_P(IdentityCredentialTests, verifyAttestationDoubleCallFails) {
    Status result;

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge1";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {1} /* atteestationApplicationId */);
    test_utils::validateAttestationCertificate(attData.attestationCertificate,
                                               attData.attestationChallenge,
                                               attData.attestationApplicationId, false);

    string challenge2 = "NotSoRandomChallenge2";
    test_utils::AttestationData attData2(writableCredential, challenge2,
                                         {} /* atteestationApplicationId */);
    EXPECT_FALSE(attData2.result.isOk()) << attData2.result.exceptionCode() << "; "
                                         << attData2.result.exceptionMessage() << endl;
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, attData2.result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, attData2.result.serviceSpecificErrorCode());
}

TEST_P(IdentityCredentialTests, verifyStartPersonalization) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    // First call should go through
    const vector<int32_t> entryCounts = {2, 4};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(5, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    // Call personalization again to check if repeat call is allowed.
    result = writableCredential->startPersonalization(7, entryCounts);

    // Second call to startPersonalization should have failed.
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_FAILED, result.serviceSpecificErrorCode());
}

TEST_P(IdentityCredentialTests, verifyStartPersonalizationMin) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {1, 1};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(1, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialTests, verifyStartPersonalizationOne) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    // Verify minimal number of profile count and entry count
    const vector<int32_t> entryCounts = {1};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(1, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialTests, verifyStartPersonalizationLarge) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    // Verify set a large number of profile count and entry count is ok
    const vector<int32_t> entryCounts = {3000};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(25, entryCounts);
    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialTests, verifyProfileNumberMismatchShouldFail) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    // Enter mismatched entry and profile numbers
    const vector<int32_t> entryCounts = {5, 6};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(5, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate = test_utils::generateReaderCertificate("12345");
    ASSERT_TRUE(readerCertificate);

    const vector<test_utils::TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                                          {1, readerCertificate.value(), false, 0},
                                                          {2, readerCertificate.value(), true, 1},
                                                          // Profile 4 (no authentication)
                                                          {4, {}, false, 0}};

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // finishAddingEntries should fail because the number of addAccessControlProfile mismatched with
    // startPersonalization, and begintest_utils::addEntry was not called.
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA, result.serviceSpecificErrorCode());
}

TEST_P(IdentityCredentialTests, verifyDuplicateProfileId) {
    Status result;
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    const vector<int32_t> entryCounts = {3, 6};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    const vector<test_utils::TestProfile> testProfiles = {// first profile should go though
                                                          {1, {}, true, 2},
                                                          // same id, different
                                                          // authentication requirement
                                                          {1, {}, true, 1},
                                                          // same id, different certificate
                                                          {1, {}, false, 0}};

    bool expectOk = true;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        int64_t secureUserId = testProfile.userAuthenticationRequired ? 66 : 0;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, secureUserId, &profile);

        if (expectOk) {
            expectOk = false;
            // for profile should be allowed though as there are no duplications
            // yet.
            ASSERT_TRUE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage()
                    << "test profile id = " << testProfile.id << endl;

            ASSERT_EQ(testProfile.id, profile.id);
            ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate.encodedCertificate);
            ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
            ASSERT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
            ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
        } else {
            // should not allow duplicate id profiles.
            ASSERT_FALSE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage()
                    << ". Test profile id = " << testProfile.id
                    << ", timeout=" << testProfile.timeoutMillis << endl;
            ASSERT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
            ASSERT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA,
                      result.serviceSpecificErrorCode());
        }
    }
}

TEST_P(IdentityCredentialTests, verifyOneProfileAndEntryPass) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge1";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {} /* atteestationApplicationId */);
    EXPECT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    optional<vector<uint8_t>> readerCertificate1 = test_utils::generateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    const vector<int32_t> entryCounts = {1u};
    size_t expectedPoPSize = 185 + readerCertificate1.value().size();
    // OK to fail, not available in v1 HAL
    writableCredential->setExpectedProofOfProvisioningSize(expectedPoPSize);
    result = writableCredential->startPersonalization(1, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    const vector<test_utils::TestProfile> testProfiles = {{1, readerCertificate1.value(), true, 1}};

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    const vector<test_utils::TestEntryData> testEntries1 = {
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{1}},
    };

    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        ASSERT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty =
            support::cborPrettyPrint(proofOfProvisioning.value(), 32, {"readerCertificate"});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 1,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 1,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'Name Space' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  false,\n"
            "]",
            cborPretty);

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
}

TEST_P(IdentityCredentialTests, verifyManyProfilesAndEntriesPass) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {} /* atteestationApplicationId */);
    EXPECT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    optional<vector<uint8_t>> readerCertificate1 = test_utils::generateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 = test_utils::generateReaderCertificate("1256");
    ASSERT_TRUE(readerCertificate2);

    const vector<test_utils::TestProfile> testProfiles = {
            {1, readerCertificate1.value(), true, 1},
            {2, readerCertificate2.value(), true, 2},
    };
    const vector<int32_t> entryCounts = {1u, 3u, 1u, 1u, 2u};
    size_t expectedPoPSize =
            525021 + readerCertificate1.value().size() + readerCertificate2.value().size();
    // OK to fail, not available in v1 HAL
    writableCredential->setExpectedProofOfProvisioningSize(expectedPoPSize);
    result = writableCredential->startPersonalization(testProfiles.size(), entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    vector<uint8_t> portraitImage1;
    test_utils::setImageData(portraitImage1);

    vector<uint8_t> portraitImage2;
    test_utils::setImageData(portraitImage2);

    const vector<test_utils::TestEntryData> testEntries1 = {
            {"Name Space 1", "Last name", string("Turing"), vector<int32_t>{1, 2}},
            {"Name Space2", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{1}},
            {"Name Space2", "Work address", string("Maida Vale2, London, England"),
             vector<int32_t>{2}},
            {"Name Space2", "Trailer address", string("Maida, London, England"),
             vector<int32_t>{1}},
            {"Image", "Portrait image", portraitImage1, vector<int32_t>{1}},
            {"Image2", "Work image", portraitImage2, vector<int32_t>{1, 2}},
            {"Name Space3", "xyzw", string("random stuff"), vector<int32_t>{1, 2}},
            {"Name Space3", "Something", string("Some string"), vector<int32_t>{2}},
    };

    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    string cborPretty = support::cborPrettyPrint(proofOfProvisioning.value(),
                                                 32,  //
                                                 {"readerCertificate"});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 1,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 1,\n"
            "    },\n"
            "    {\n"
            "      'id' : 2,\n"
            "      'readerCertificate' : <not printed>,\n"
            "      'userAuthenticationRequired' : true,\n"
            "      'timeoutMillis' : 2,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'Name Space 1' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "    ],\n"
            "    'Name Space2' : [\n"
            "      {\n"
            "        'name' : 'Home address',\n"
            "        'value' : 'Maida Vale, London, England',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Work address',\n"
            "        'value' : 'Maida Vale2, London, England',\n"
            "        'accessControlProfiles' : [2, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Trailer address',\n"
            "        'value' : 'Maida, London, England',\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image' : [\n"
            "      {\n"
            "        'name' : 'Portrait image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [1, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image2' : [\n"
            "      {\n"
            "        'name' : 'Work image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "    ],\n"
            "    'Name Space3' : [\n"
            "      {\n"
            "        'name' : 'xyzw',\n"
            "        'value' : 'random stuff',\n"
            "        'accessControlProfiles' : [1, 2, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Something',\n"
            "        'value' : 'Some string',\n"
            "        'accessControlProfiles' : [2, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  false,\n"
            "]",
            cborPretty);

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
}

TEST_P(IdentityCredentialTests, verifyEmptyNameSpaceMixedWithNonEmptyWorks) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {} /* atteestationApplicationId */);
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    optional<vector<uint8_t>> readerCertificate1 = test_utils::generateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 =
            test_utils::generateReaderCertificate("123456987987987987987987");
    ASSERT_TRUE(readerCertificate2);

    const vector<int32_t> entryCounts = {2u, 2u};
    size_t expectedPoPSize =
            377 + readerCertificate1.value().size() + readerCertificate2.value().size();
    ;
    // OK to fail, not available in v1 HAL
    writableCredential->setExpectedProofOfProvisioningSize(expectedPoPSize);
    result = writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    const vector<test_utils::TestProfile> testProfiles = {{0, readerCertificate1.value(), false, 0},
                                                          {1, readerCertificate2.value(), true, 1},
                                                          {2, {}, false, 0}};

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    const vector<test_utils::TestEntryData> testEntries1 = {
            // test empty name space
            {"", "t name", string("Turing"), vector<int32_t>{2}},
            {"", "Birth", string("19120623"), vector<int32_t>{2}},
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"Name Space", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
    };

    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;
}

TEST_P(IdentityCredentialTests, verifyInterleavingEntryNameSpaceOrderingFails) {
    Status result;

    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    string challenge = "NotSoRandomChallenge";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {} /* atteestationApplicationId */);
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    // Enter mismatched entry and profile numbers.
    // Technically the 2nd name space of "Name Space" occurs intermittently, 2
    // before "Image" and 2 after image, which is not correct.  All of same name
    // space should occur together.  Let's see if this fails.
    const vector<int32_t> entryCounts = {2u, 1u, 2u};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    result = writableCredential->startPersonalization(3, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    optional<vector<uint8_t>> readerCertificate1 = test_utils::generateReaderCertificate("123456");
    ASSERT_TRUE(readerCertificate1);

    optional<vector<uint8_t>> readerCertificate2 =
            test_utils::generateReaderCertificate("123456987987987987987987");
    ASSERT_TRUE(readerCertificate2);

    const vector<test_utils::TestProfile> testProfiles = {{0, readerCertificate1.value(), false, 0},
                                                          {1, readerCertificate2.value(), true, 1},
                                                          {2, {}, false, 0}};

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    const vector<test_utils::TestEntryData> testEntries1 = {
            // test empty name space
            {"Name Space", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"Name Space", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
    };

    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;
    for (const auto& entry : testEntries1) {
        EXPECT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }
    const test_utils::TestEntryData testEntry2 = {"Image", "Portrait image", string("asdfs"),
                                                  vector<int32_t>{0, 1}};

    EXPECT_TRUE(test_utils::addEntry(writableCredential, testEntry2, hwInfo.dataChunkSize,
                                     encryptedBlobs, true));

    // We expect this to fail because the namespace is out of order, all "Name Space"
    // should have been called together
    const vector<test_utils::TestEntryData> testEntries3 = {
            {"Name Space", "First name", string("Alan"), vector<int32_t>{0, 1}},
            {"Name Space", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{0}},
    };

    for (const auto& entry : testEntries3) {
        EXPECT_FALSE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                          encryptedBlobs, false));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    result =
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature);

    // should fail because test_utils::addEntry should have failed earlier.
    EXPECT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                << endl;
    EXPECT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    EXPECT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA, result.serviceSpecificErrorCode());
}

TEST_P(IdentityCredentialTests, verifyAccessControlProfileIdOutOfRange) {
    sp<IWritableIdentityCredential> writableCredential;
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    false /* testCredential */));

    const vector<int32_t> entryCounts = {1};
    writableCredential->setExpectedProofOfProvisioningSize(123456);
    Status result = writableCredential->startPersonalization(1, entryCounts);
    ASSERT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                               << endl;

    SecureAccessControlProfile profile;

    // This should fail because the id is >= 32
    result = writableCredential->addAccessControlProfile(32,     // id
                                                         {},     // readerCertificate
                                                         false,  // userAuthenticationRequired
                                                         0,      // timeoutMillis
                                                         42,     // secureUserId
                                                         &profile);
    ASSERT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage();
    ASSERT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA, result.serviceSpecificErrorCode());

    // This should fail because the id is < 0
    result = writableCredential->addAccessControlProfile(-1,     // id
                                                         {},     // readerCertificate
                                                         false,  // userAuthenticationRequired
                                                         0,      // timeoutMillis
                                                         42,     // secureUserId
                                                         &profile);
    ASSERT_FALSE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage();
    ASSERT_EQ(binder::Status::EX_SERVICE_SPECIFIC, result.exceptionCode());
    ASSERT_EQ(IIdentityCredentialStore::STATUS_INVALID_DATA, result.serviceSpecificErrorCode());
}

INSTANTIATE_TEST_SUITE_P(
        Identity, IdentityCredentialTests,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);

}  // namespace android::hardware::identity
