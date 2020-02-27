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
#define LOG_TAG "VtsHalIdentityTargetTest"

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

namespace android::hardware::identity {

using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using ::android::hardware::keymaster::HardwareAuthToken;

// ---------------------------------------------------------------------------
// Test Data.
// ---------------------------------------------------------------------------

struct TestEntryData {
    TestEntryData(string nameSpace, string name, vector<int32_t> profileIds)
        : nameSpace(nameSpace), name(name), profileIds(profileIds) {}

    TestEntryData(string nameSpace, string name, const string& value, vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Tstr(((const char*)value.data())).encode();
    }
    TestEntryData(string nameSpace, string name, const vector<uint8_t>& value,
                  vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bstr(value).encode();
    }
    TestEntryData(string nameSpace, string name, bool value, vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bool(value).encode();
    }
    TestEntryData(string nameSpace, string name, int64_t value, vector<int32_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        if (value >= 0) {
            valueCbor = cppbor::Uint(value).encode();
        } else {
            valueCbor = cppbor::Nint(-value).encode();
        }
    }

    string nameSpace;
    string name;
    vector<uint8_t> valueCbor;
    vector<int32_t> profileIds;
};

struct TestProfile {
    uint16_t id;
    vector<uint8_t> readerCertificate;
    bool userAuthenticationRequired;
    uint64_t timeoutMillis;
};

// ----------------------------------------------------------------

class IdentityAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        credentialStore_ = android::waitForDeclaredService<IIdentityCredentialStore>(
                String16(GetParam().c_str()));
        ASSERT_NE(credentialStore_, nullptr);
    }

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_P(IdentityAidl, hardwareInformation) {
    HardwareInformation info;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&info).isOk());
    ASSERT_GT(info.credentialStoreName.size(), 0);
    ASSERT_GT(info.credentialStoreAuthorName.size(), 0);
    ASSERT_GE(info.dataChunkSize, 256);
}

TEST_P(IdentityAidl, createAndRetrieveCredential) {
    // First, generate a key-pair for the reader since its public key will be
    // part of the request data.
    optional<vector<uint8_t>> readerKeyPKCS8 = support::createEcKeyPair();
    ASSERT_TRUE(readerKeyPKCS8);
    optional<vector<uint8_t>> readerPublicKey =
            support::ecKeyPairGetPublicKey(readerKeyPKCS8.value());
    optional<vector<uint8_t>> readerKey = support::ecKeyPairGetPrivateKey(readerKeyPKCS8.value());
    string serialDecimal = "1234";
    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential VTS Test";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;
    optional<vector<uint8_t>> readerCertificate = support::ecPublicKeyGenerateCertificate(
            readerPublicKey.value(), readerKey.value(), serialDecimal, issuer, subject,
            validityNotBefore, validityNotAfter);
    ASSERT_TRUE(readerCertificate);

    // Make the portrait image really big (just shy of 256 KiB) to ensure that
    // the chunking code gets exercised.
    vector<uint8_t> portraitImage;
    portraitImage.resize(256 * 1024 - 10);
    for (size_t n = 0; n < portraitImage.size(); n++) {
        portraitImage[n] = (uint8_t)n;
    }

    // Access control profiles:
    const vector<TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                              {0, readerCertificate.value(), false, 0},
                                              // Profile 1 (no authentication)
                                              {1, {}, false, 0}};

    HardwareAuthToken authToken;

    // Here's the actual test data:
    const vector<TestEntryData> testEntries = {
            {"PersonalData", "Last name", string("Turing"), vector<int32_t>{0, 1}},
            {"PersonalData", "Birth date", string("19120623"), vector<int32_t>{0, 1}},
            {"PersonalData", "First name", string("Alan"), vector<int32_t>{0, 1}},
            {"PersonalData", "Home address", string("Maida Vale, London, England"),
             vector<int32_t>{0}},
            {"Image", "Portrait image", portraitImage, vector<int32_t>{0, 1}},
    };
    const vector<int32_t> testEntriesEntryCounts = {static_cast<int32_t>(testEntries.size() - 1),
                                                    1u};
    HardwareInformation hwInfo;
    ASSERT_TRUE(credentialStore_->getHardwareInformation(&hwInfo).isOk());

    string cborPretty;
    sp<IWritableIdentityCredential> writableCredential;
    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    ASSERT_TRUE(credentialStore_->createCredential(docType, testCredential, &writableCredential)
                        .isOk());
    ASSERT_NE(writableCredential, nullptr);

    string challenge = "attestationChallenge";
    // TODO: set it to something random and check it's in the cert chain
    vector<uint8_t> attestationApplicationId = {};
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<Certificate> attestationCertificates;
    ASSERT_TRUE(writableCredential
                        ->getAttestationCertificate(attestationApplicationId, attestationChallenge,
                                                    &attestationCertificates)
                        .isOk());
    ASSERT_GE(attestationCertificates.size(), 2);

    ASSERT_TRUE(
            writableCredential->startPersonalization(testProfiles.size(), testEntriesEntryCounts)
                    .isOk());

    vector<SecureAccessControlProfile> returnedSecureProfiles;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        ASSERT_TRUE(writableCredential
                            ->addAccessControlProfile(testProfile.id, cert,
                                                      testProfile.userAuthenticationRequired,
                                                      testProfile.timeoutMillis,
                                                      0,  // secureUserId
                                                      &profile)
                            .isOk());
        ASSERT_EQ(testProfile.id, profile.id);
        ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate.encodedCertificate);
        ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
        ASSERT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
        ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
        returnedSecureProfiles.push_back(profile);
    }

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        vector<vector<uint8_t>> chunks =
                support::chunkVector(entry.valueCbor, hwInfo.dataChunkSize);

        ASSERT_TRUE(writableCredential
                            ->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                            entry.valueCbor.size())
                            .isOk());

        vector<vector<uint8_t>> encryptedChunks;
        for (const auto& chunk : chunks) {
            vector<uint8_t> encryptedChunk;
            ASSERT_TRUE(writableCredential->addEntryValue(chunk, &encryptedChunk).isOk());
            encryptedChunks.push_back(encryptedChunk);
        }
        encryptedBlobs[&entry] = encryptedChunks;
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    ASSERT_TRUE(
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature)
                    .isOk());

    optional<vector<uint8_t>> proofOfProvisioning =
            support::coseSignGetPayload(proofOfProvisioningSignature);
    ASSERT_TRUE(proofOfProvisioning);
    cborPretty = support::cborPrettyPrint(proofOfProvisioning.value(), 32, {"readerCertificate"});
    EXPECT_EQ(
            "[\n"
            "  'ProofOfProvisioning',\n"
            "  'org.iso.18013-5.2019.mdl',\n"
            "  [\n"
            "    {\n"
            "      'id' : 0,\n"
            "      'readerCertificate' : <not printed>,\n"
            "    },\n"
            "    {\n"
            "      'id' : 1,\n"
            "    },\n"
            "  ],\n"
            "  {\n"
            "    'PersonalData' : [\n"
            "      {\n"
            "        'name' : 'Last name',\n"
            "        'value' : 'Turing',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Birth date',\n"
            "        'value' : '19120623',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'First name',\n"
            "        'value' : 'Alan',\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "      {\n"
            "        'name' : 'Home address',\n"
            "        'value' : 'Maida Vale, London, England',\n"
            "        'accessControlProfiles' : [0, ],\n"
            "      },\n"
            "    ],\n"
            "    'Image' : [\n"
            "      {\n"
            "        'name' : 'Portrait image',\n"
            "        'value' : <bstr size=262134 sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "        'accessControlProfiles' : [0, 1, ],\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "  true,\n"
            "]",
            cborPretty);

    optional<vector<uint8_t>> credentialPubKey =
            support::certificateChainGetTopMostKey(attestationCertificates[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
    writableCredential = nullptr;

    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    sp<IIdentityCredential> credential;
    ASSERT_TRUE(credentialStore_
                        ->getCredential(
                                CipherSuite::CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256,
                                credentialData, &credential)
                        .isOk());
    ASSERT_NE(credential, nullptr);

    optional<vector<uint8_t>> readerEphemeralKeyPair = support::createEcKeyPair();
    ASSERT_TRUE(readerEphemeralKeyPair);
    optional<vector<uint8_t>> readerEphemeralPublicKey =
            support::ecKeyPairGetPublicKey(readerEphemeralKeyPair.value());
    ASSERT_TRUE(credential->setReaderEphemeralPublicKey(readerEphemeralPublicKey.value()).isOk());

    vector<uint8_t> ephemeralKeyPair;
    ASSERT_TRUE(credential->createEphemeralKeyPair(&ephemeralKeyPair).isOk());
    optional<vector<uint8_t>> ephemeralPublicKey = support::ecKeyPairGetPublicKey(ephemeralKeyPair);

    // Calculate requestData field and sign it with the reader key.
    auto [getXYSuccess, ephX, ephY] = support::ecPublicKeyGetXandY(ephemeralPublicKey.value());
    ASSERT_TRUE(getXYSuccess);
    cppbor::Map deviceEngagement = cppbor::Map().add("ephX", ephX).add("ephY", ephY);
    vector<uint8_t> deviceEngagementBytes = deviceEngagement.encode();
    vector<uint8_t> eReaderPubBytes = cppbor::Tstr("ignored").encode();
    cppbor::Array sessionTranscript = cppbor::Array()
                                              .add(cppbor::Semantic(24, deviceEngagementBytes))
                                              .add(cppbor::Semantic(24, eReaderPubBytes));
    vector<uint8_t> sessionTranscriptBytes = sessionTranscript.encode();

    vector<uint8_t> itemsRequestBytes =
            cppbor::Map("nameSpaces",
                        cppbor::Map()
                                .add("PersonalData", cppbor::Map()
                                                             .add("Last name", false)
                                                             .add("Birth date", false)
                                                             .add("First name", false)
                                                             .add("Home address", true))
                                .add("Image", cppbor::Map().add("Portrait image", false)))
                    .encode();
    cborPretty = support::cborPrettyPrint(itemsRequestBytes, 32, {"EphemeralPublicKey"});
    EXPECT_EQ(
            "{\n"
            "  'nameSpaces' : {\n"
            "    'PersonalData' : {\n"
            "      'Last name' : false,\n"
            "      'Birth date' : false,\n"
            "      'First name' : false,\n"
            "      'Home address' : true,\n"
            "    },\n"
            "    'Image' : {\n"
            "      'Portrait image' : false,\n"
            "    },\n"
            "  },\n"
            "}",
            cborPretty);
    vector<uint8_t> dataToSign = cppbor::Array()
                                         .add("ReaderAuthentication")
                                         .add(sessionTranscript.clone())
                                         .add(cppbor::Semantic(24, itemsRequestBytes))
                                         .encode();
    optional<vector<uint8_t>> readerSignature =
            support::coseSignEcDsa(readerKey.value(), {},  // content
                                   dataToSign,             // detached content
                                   readerCertificate.value());
    ASSERT_TRUE(readerSignature);

    ASSERT_TRUE(credential
                        ->startRetrieval(returnedSecureProfiles, authToken, itemsRequestBytes,
                                         sessionTranscriptBytes, readerSignature.value(),
                                         testEntriesEntryCounts)
                        .isOk());

    for (const auto& entry : testEntries) {
        ASSERT_TRUE(credential
                            ->startRetrieveEntryValue(entry.nameSpace, entry.name,
                                                      entry.valueCbor.size(), entry.profileIds)
                            .isOk());

        auto it = encryptedBlobs.find(&entry);
        ASSERT_NE(it, encryptedBlobs.end());
        const vector<vector<uint8_t>>& encryptedChunks = it->second;

        vector<uint8_t> content;
        for (const auto& encryptedChunk : encryptedChunks) {
            vector<uint8_t> chunk;
            ASSERT_TRUE(credential->retrieveEntryValue(encryptedChunk, &chunk).isOk());
            content.insert(content.end(), chunk.begin(), chunk.end());
        }
        EXPECT_EQ(content, entry.valueCbor);
    }

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(credential->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpacesBytes;
    ASSERT_TRUE(credential->finishRetrieval(signingKeyBlob, &mac, &deviceNameSpacesBytes).isOk());
    cborPretty = support::cborPrettyPrint(deviceNameSpacesBytes, 32, {});
    ASSERT_EQ(
            "{\n"
            "  'PersonalData' : {\n"
            "    'Last name' : 'Turing',\n"
            "    'Birth date' : '19120623',\n"
            "    'First name' : 'Alan',\n"
            "    'Home address' : 'Maida Vale, London, England',\n"
            "  },\n"
            "  'Image' : {\n"
            "    'Portrait image' : <bstr size=262134 "
            "sha1=941e372f654d86c32d88fae9e41b706afbfd02bb>,\n"
            "  },\n"
            "}",
            cborPretty);
    // The data that is MACed is ["DeviceAuthentication", sessionTranscriptBytes, docType,
    // deviceNameSpacesBytes] so build up that structure
    cppbor::Array deviceAuthentication;
    deviceAuthentication.add("DeviceAuthentication");
    deviceAuthentication.add(sessionTranscript.clone());
    deviceAuthentication.add(docType);
    deviceAuthentication.add(cppbor::Semantic(24, deviceNameSpacesBytes));
    vector<uint8_t> encodedDeviceAuthentication = deviceAuthentication.encode();
    optional<vector<uint8_t>> signingPublicKey =
            support::certificateChainGetTopMostKey(signingKeyCertificate.encodedCertificate);
    EXPECT_TRUE(signingPublicKey);

    // Derive the key used for MACing.
    optional<vector<uint8_t>> readerEphemeralPrivateKey =
            support::ecKeyPairGetPrivateKey(readerEphemeralKeyPair.value());
    optional<vector<uint8_t>> sharedSecret =
            support::ecdh(signingPublicKey.value(), readerEphemeralPrivateKey.value());
    ASSERT_TRUE(sharedSecret);
    vector<uint8_t> salt = {0x00};
    vector<uint8_t> info = {};
    optional<vector<uint8_t>> derivedKey = support::hkdf(sharedSecret.value(), salt, info, 32);
    ASSERT_TRUE(derivedKey);
    optional<vector<uint8_t>> calculatedMac =
            support::coseMac0(derivedKey.value(), {},        // payload
                              encodedDeviceAuthentication);  // detached content
    ASSERT_TRUE(calculatedMac);
    EXPECT_EQ(mac, calculatedMac);
}

INSTANTIATE_TEST_SUITE_P(
        Identity, IdentityAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IIdentityCredentialStore::descriptor)),
        android::PrintInstanceNameToString);
// INSTANTIATE_TEST_SUITE_P(Identity, IdentityAidl,
// testing::Values("android.hardware.identity.IIdentityCredentialStore/default"));

}  // namespace android::hardware::identity

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ::android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
