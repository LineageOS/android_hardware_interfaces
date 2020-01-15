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

#define LOG_TAG "IdentityCredentialHidlHalTest"

#include <map>

#include <android-base/logging.h>
#include <android/hardware/identity/1.0/IIdentityCredentialStore.h>
#include <android/hardware/identity/1.0/types.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <cppbor.h>
#include <cppbor_parse.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using std::map;
using std::optional;
using std::string;
using std::vector;

namespace android {
namespace hardware {
namespace identity {
namespace test {

using ::android::hardware::identity::V1_0::IIdentityCredential;
using ::android::hardware::identity::V1_0::IIdentityCredentialStore;
using ::android::hardware::identity::V1_0::IWritableIdentityCredential;
using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;
using ::android::hardware::identity::V1_0::SecureAccessControlProfile;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;

// ---------------------------------------------------------------------------
// Test Data.
// ---------------------------------------------------------------------------

struct TestEntryData {
    TestEntryData(string nameSpace, string name, vector<uint16_t> profileIds)
        : nameSpace(nameSpace), name(name), profileIds(profileIds) {}

    TestEntryData(string nameSpace, string name, const string& value, vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Tstr(((const char*)value.data())).encode();
    }
    TestEntryData(string nameSpace, string name, const vector<uint8_t>& value,
                  vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bstr(value).encode();
    }
    TestEntryData(string nameSpace, string name, bool value, vector<uint16_t> profileIds)
        : TestEntryData(nameSpace, name, profileIds) {
        valueCbor = cppbor::Bool(value).encode();
    }
    TestEntryData(string nameSpace, string name, int64_t value, vector<uint16_t> profileIds)
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
    vector<uint16_t> profileIds;
};

struct TestProfile {
    uint16_t id;
    hidl_vec<uint8_t> readerCertificate;
    bool userAuthenticationRequired;
    uint64_t timeoutMillis;
};

/************************************
 *   TEST DATA FOR AUTHENTICATION
 ************************************/
// Test authentication token for user authentication

class IdentityCredentialStoreHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        string serviceName = GetParam();
        ASSERT_FALSE(serviceName.empty());
        credentialStore_ = IIdentityCredentialStore::getService(serviceName);
        ASSERT_NE(credentialStore_, nullptr);

        credentialStore_->getHardwareInformation(
                [&](const Result& result, const hidl_string& credentialStoreName,
                    const hidl_string& credentialStoreAuthorName, uint32_t chunkSize,
                    bool /* isDirectAccess */,
                    const hidl_vec<hidl_string> /* supportedDocTypes */) {
                    EXPECT_EQ("", result.message);
                    ASSERT_EQ(ResultCode::OK, result.code);
                    ASSERT_GT(credentialStoreName.size(), 0u);
                    ASSERT_GT(credentialStoreAuthorName.size(), 0u);
                    ASSERT_GE(chunkSize, 256u);  // Chunk sizes < APDU buffer won't be supported
                    dataChunkSize_ = chunkSize;
                });
    }
    virtual void TearDown() override {}

    uint32_t dataChunkSize_ = 0;

    sp<IIdentityCredentialStore> credentialStore_;
};

TEST_P(IdentityCredentialStoreHidlTest, HardwareConfiguration) {
    credentialStore_->getHardwareInformation(
            [&](const Result& result, const hidl_string& credentialStoreName,
                const hidl_string& credentialStoreAuthorName, uint32_t chunkSize,
                bool /* isDirectAccess */, const hidl_vec<hidl_string> /* supportedDocTypes */) {
                EXPECT_EQ("", result.message);
                ASSERT_EQ(ResultCode::OK, result.code);
                ASSERT_GT(credentialStoreName.size(), 0u);
                ASSERT_GT(credentialStoreAuthorName.size(), 0u);
                ASSERT_GE(chunkSize, 256u);  // Chunk sizes < APDU buffer won't be supported
            });
}

TEST_P(IdentityCredentialStoreHidlTest, createAndRetrieveCredential) {
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

    HardwareAuthToken authToken = {};

    // Here's the actual test data:
    const vector<TestEntryData> testEntries = {
            {"PersonalData", "Last name", string("Turing"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Birth date", string("19120623"), vector<uint16_t>{0, 1}},
            {"PersonalData", "First name", string("Alan"), vector<uint16_t>{0, 1}},
            {"PersonalData", "Home address", string("Maida Vale, London, England"),
             vector<uint16_t>{0}},
            {"Image", "Portrait image", portraitImage, vector<uint16_t>{0, 1}},
    };
    const vector<uint16_t> testEntriesEntryCounts = {static_cast<uint16_t>(testEntries.size() - 1),
                                                     1u};

    string cborPretty;
    sp<IWritableIdentityCredential> writableCredential;

    hidl_vec<uint8_t> empty{0};

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    Result result;
    credentialStore_->createCredential(
            docType, testCredential,
            [&](const Result& _result, const sp<IWritableIdentityCredential>& _writableCredential) {
                result = _result;
                writableCredential = _writableCredential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(writableCredential, nullptr);

    string challenge = "attestationChallenge";
    vector<uint8_t> attestationChallenge(challenge.begin(), challenge.end());
    vector<uint8_t> attestationCertificate;
    writableCredential->getAttestationCertificate(
            attestationChallenge,
            [&](const Result& _result, const hidl_vec<uint8_t>& _attestationCertificate) {
                result = _result;
                attestationCertificate = _attestationCertificate;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    writableCredential->startPersonalization(testProfiles.size(), testEntriesEntryCounts,
                                             [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<SecureAccessControlProfile> returnedSecureProfiles;
    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        writableCredential->addAccessControlProfile(
                testProfile.id, testProfile.readerCertificate,
                testProfile.userAuthenticationRequired, testProfile.timeoutMillis,
                0,  // secureUserId
                [&](const Result& _result, const SecureAccessControlProfile& _profile) {
                    result = _result;
                    profile = _profile;
                });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);
        ASSERT_EQ(testProfile.id, profile.id);
        ASSERT_EQ(testProfile.readerCertificate, profile.readerCertificate);
        ASSERT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
        ASSERT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
        ASSERT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());
        returnedSecureProfiles.push_back(profile);
    }

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    map<const TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        vector<vector<uint8_t>> chunks = support::chunkVector(entry.valueCbor, dataChunkSize_);

        writableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                          entry.valueCbor.size(),
                                          [&](const Result& _result) { result = _result; });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);

        vector<vector<uint8_t>> encryptedChunks;
        for (const auto& chunk : chunks) {
            writableCredential->addEntryValue(
                    chunk, [&](const Result& result, hidl_vec<uint8_t> encryptedContent) {
                        EXPECT_EQ("", result.message);
                        ASSERT_EQ(ResultCode::OK, result.code);
                        ASSERT_GT(encryptedContent.size(), 0u);
                        encryptedChunks.push_back(encryptedContent);
                    });
        }
        encryptedBlobs[&entry] = encryptedChunks;
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    writableCredential->finishAddingEntries(
            [&](const Result& _result, const hidl_vec<uint8_t>& _credentialData,
                const hidl_vec<uint8_t>& _proofOfProvisioningSignature) {
                result = _result;
                credentialData = _credentialData;
                proofOfProvisioningSignature = _proofOfProvisioningSignature;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

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
            support::certificateChainGetTopMostKey(attestationCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
    writableCredential = nullptr;

    // Now that the credential has been provisioned, read it back and check the
    // correct data is returned.
    sp<IIdentityCredential> credential;
    credentialStore_->getCredential(
            credentialData, [&](const Result& _result, const sp<IIdentityCredential>& _credential) {
                result = _result;
                credential = _credential;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
    ASSERT_NE(credential, nullptr);

    optional<vector<uint8_t>> readerEphemeralKeyPair = support::createEcKeyPair();
    ASSERT_TRUE(readerEphemeralKeyPair);
    optional<vector<uint8_t>> readerEphemeralPublicKey =
            support::ecKeyPairGetPublicKey(readerEphemeralKeyPair.value());
    credential->setReaderEphemeralPublicKey(readerEphemeralPublicKey.value(),
                                            [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<uint8_t> ephemeralKeyPair;
    credential->createEphemeralKeyPair(
            [&](const Result& _result, const hidl_vec<uint8_t>& _ephemeralKeyPair) {
                result = _result;
                ephemeralKeyPair = _ephemeralKeyPair;
            });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
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

    credential->startRetrieval(returnedSecureProfiles, authToken, itemsRequestBytes,
                               sessionTranscriptBytes, readerSignature.value(),
                               testEntriesEntryCounts,
                               [&](const Result& _result) { result = _result; });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    for (const auto& entry : testEntries) {
        credential->startRetrieveEntryValue(entry.nameSpace, entry.name, entry.valueCbor.size(),
                                            entry.profileIds,
                                            [&](const Result& _result) { result = _result; });
        EXPECT_EQ("", result.message);
        ASSERT_EQ(ResultCode::OK, result.code);

        auto it = encryptedBlobs.find(&entry);
        ASSERT_NE(it, encryptedBlobs.end());
        const vector<vector<uint8_t>>& encryptedChunks = it->second;

        vector<uint8_t> content;
        for (const auto& encryptedChunk : encryptedChunks) {
            vector<uint8_t> chunk;
            credential->retrieveEntryValue(
                    encryptedChunk, [&](const Result& _result, const hidl_vec<uint8_t>& _chunk) {
                        result = _result;
                        chunk = _chunk;
                    });
            EXPECT_EQ("", result.message);
            ASSERT_EQ(ResultCode::OK, result.code);
            content.insert(content.end(), chunk.begin(), chunk.end());
        }
        EXPECT_EQ(content, entry.valueCbor);
    }

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    vector<uint8_t> signingKeyCertificate;
    credential->generateSigningKeyPair([&](const Result& _result,
                                           const hidl_vec<uint8_t> _signingKeyBlob,
                                           const hidl_vec<uint8_t> _signingKeyCertificate) {
        result = _result;
        signingKeyBlob = _signingKeyBlob;
        signingKeyCertificate = _signingKeyCertificate;
    });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpacesBytes;
    credential->finishRetrieval(signingKeyBlob,
                                [&](const Result& _result, const hidl_vec<uint8_t> _mac,
                                    const hidl_vec<uint8_t> _deviceNameSpacesBytes) {
                                    result = _result;
                                    mac = _mac;
                                    deviceNameSpacesBytes = _deviceNameSpacesBytes;
                                });
    EXPECT_EQ("", result.message);
    ASSERT_EQ(ResultCode::OK, result.code);
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
            support::certificateChainGetTopMostKey(signingKeyCertificate);
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

INSTANTIATE_TEST_SUITE_P(PerInstance, IdentityCredentialStoreHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IIdentityCredentialStore::descriptor)),
                         android::hardware::PrintInstanceNameToString);

}  // namespace test
}  // namespace identity
}  // namespace hardware
}  // namespace android
