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
#define LOG_TAG "VtsHalIdentityEndToEndTest"

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
#include <tuple>

#include "VtsIdentityTestUtils.h"

namespace android::hardware::identity {

using std::endl;
using std::make_tuple;
using std::map;
using std::optional;
using std::string;
using std::tuple;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

using ::android::hardware::keymaster::HardwareAuthToken;
using ::android::hardware::keymaster::VerificationToken;

using test_utils::validateAttestationCertificate;

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

tuple<bool, string, vector<uint8_t>, vector<uint8_t>> extractFromTestCredentialData(
        const vector<uint8_t>& credentialData) {
    string docType;
    vector<uint8_t> storageKey;
    vector<uint8_t> credentialPrivKey;

    auto [item, _, message] = cppbor::parse(credentialData);
    if (item == nullptr) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }

    const cppbor::Array* arrayItem = item->asArray();
    if (arrayItem == nullptr || arrayItem->size() != 3) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }

    const cppbor::Tstr* docTypeItem = (*arrayItem)[0]->asTstr();
    const cppbor::Bool* testCredentialItem =
            ((*arrayItem)[1]->asSimple() != nullptr ? ((*arrayItem)[1]->asSimple()->asBool())
                                                    : nullptr);
    const cppbor::Bstr* encryptedCredentialKeysItem = (*arrayItem)[2]->asBstr();
    if (docTypeItem == nullptr || testCredentialItem == nullptr ||
        encryptedCredentialKeysItem == nullptr) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }

    docType = docTypeItem->value();

    vector<uint8_t> hardwareBoundKey = support::getTestHardwareBoundKey();
    const vector<uint8_t>& encryptedCredentialKeys = encryptedCredentialKeysItem->value();
    const vector<uint8_t> docTypeVec(docType.begin(), docType.end());
    optional<vector<uint8_t>> decryptedCredentialKeys =
            support::decryptAes128Gcm(hardwareBoundKey, encryptedCredentialKeys, docTypeVec);
    if (!decryptedCredentialKeys) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }

    auto [dckItem, dckPos, dckMessage] = cppbor::parse(decryptedCredentialKeys.value());
    if (dckItem == nullptr) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }
    const cppbor::Array* dckArrayItem = dckItem->asArray();
    if (dckArrayItem == nullptr || dckArrayItem->size() != 2) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }
    const cppbor::Bstr* storageKeyItem = (*dckArrayItem)[0]->asBstr();
    const cppbor::Bstr* credentialPrivKeyItem = (*dckArrayItem)[1]->asBstr();
    if (storageKeyItem == nullptr || credentialPrivKeyItem == nullptr) {
        return make_tuple(false, docType, storageKey, credentialPrivKey);
    }
    storageKey = storageKeyItem->value();
    credentialPrivKey = credentialPrivKeyItem->value();
    return make_tuple(true, docType, storageKey, credentialPrivKey);
}

TEST_P(IdentityAidl, createAndRetrieveCredential) {
    // First, generate a key-pair for the reader since its public key will be
    // part of the request data.
    vector<uint8_t> readerKey;
    optional<vector<uint8_t>> readerCertificate =
            test_utils::generateReaderCertificate("1234", &readerKey);
    ASSERT_TRUE(readerCertificate);

    // Make the portrait image really big (just shy of 256 KiB) to ensure that
    // the chunking code gets exercised.
    vector<uint8_t> portraitImage;
    test_utils::setImageData(portraitImage);

    // Access control profiles:
    const vector<test_utils::TestProfile> testProfiles = {// Profile 0 (reader authentication)
                                                          {0, readerCertificate.value(), false, 0},
                                                          // Profile 1 (no authentication)
                                                          {1, {}, false, 0}};

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

    // Here's the actual test data:
    const vector<test_utils::TestEntryData> testEntries = {
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
    ASSERT_TRUE(test_utils::setupWritableCredential(writableCredential, credentialStore_,
                                                    true /* testCredential */));

    string challenge = "attestationChallenge";
    test_utils::AttestationData attData(writableCredential, challenge,
                                        {1} /* atteestationApplicationId */);
    ASSERT_TRUE(attData.result.isOk())
            << attData.result.exceptionCode() << "; " << attData.result.exceptionMessage() << endl;

    validateAttestationCertificate(attData.attestationCertificate, attData.attestationChallenge,
                                   attData.attestationApplicationId, true);

    // This is kinda of a hack but we need to give the size of
    // ProofOfProvisioning that we'll expect to receive.
    const int32_t expectedProofOfProvisioningSize = 262861 - 326 + readerCertificate.value().size();
    // OK to fail, not available in v1 HAL
    writableCredential->setExpectedProofOfProvisioningSize(expectedProofOfProvisioningSize);
    ASSERT_TRUE(
            writableCredential->startPersonalization(testProfiles.size(), testEntriesEntryCounts)
                    .isOk());

    optional<vector<SecureAccessControlProfile>> secureProfiles =
            test_utils::addAccessControlProfiles(writableCredential, testProfiles);
    ASSERT_TRUE(secureProfiles);

    // Uses TestEntryData* pointer as key and values are the encrypted blobs. This
    // is a little hacky but it works well enough.
    map<const test_utils::TestEntryData*, vector<vector<uint8_t>>> encryptedBlobs;

    for (const auto& entry : testEntries) {
        ASSERT_TRUE(test_utils::addEntry(writableCredential, entry, hwInfo.dataChunkSize,
                                         encryptedBlobs, true));
    }

    vector<uint8_t> credentialData;
    vector<uint8_t> proofOfProvisioningSignature;
    ASSERT_TRUE(
            writableCredential->finishAddingEntries(&credentialData, &proofOfProvisioningSignature)
                    .isOk());

    // Validate the proofOfProvisioning which was returned
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

    optional<vector<uint8_t>> credentialPubKey = support::certificateChainGetTopMostKey(
            attData.attestationCertificate[0].encodedCertificate);
    ASSERT_TRUE(credentialPubKey);
    EXPECT_TRUE(support::coseCheckEcDsaSignature(proofOfProvisioningSignature,
                                                 {},  // Additional data
                                                 credentialPubKey.value()));
    writableCredential = nullptr;

    // Extract doctype, storage key, and credentialPrivKey from credentialData... this works
    // only because we asked for a test-credential meaning that the HBK is all zeroes.
    auto [exSuccess, exDocType, exStorageKey, exCredentialPrivKey] =
            extractFromTestCredentialData(credentialData);
    ASSERT_TRUE(exSuccess);
    ASSERT_EQ(exDocType, "org.iso.18013-5.2019.mdl");
    // ... check that the public key derived from the private key matches what was
    // in the certificate.
    optional<vector<uint8_t>> exCredentialKeyPair =
            support::ecPrivateKeyToKeyPair(exCredentialPrivKey);
    ASSERT_TRUE(exCredentialKeyPair);
    optional<vector<uint8_t>> exCredentialPubKey =
            support::ecKeyPairGetPublicKey(exCredentialKeyPair.value());
    ASSERT_TRUE(exCredentialPubKey);
    ASSERT_EQ(exCredentialPubKey.value(), credentialPubKey.value());

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
    vector<uint8_t> sessionTranscriptEncoded = sessionTranscript.encode();

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
    vector<uint8_t> encodedReaderAuthentication =
            cppbor::Array()
                    .add("ReaderAuthentication")
                    .add(sessionTranscript.clone())
                    .add(cppbor::Semantic(24, itemsRequestBytes))
                    .encode();
    vector<uint8_t> encodedReaderAuthenticationBytes =
            cppbor::Semantic(24, encodedReaderAuthentication).encode();
    optional<vector<uint8_t>> readerSignature =
            support::coseSignEcDsa(readerKey, {},                     // content
                                   encodedReaderAuthenticationBytes,  // detached content
                                   readerCertificate.value());
    ASSERT_TRUE(readerSignature);

    // Generate the key that will be used to sign AuthenticatedData.
    vector<uint8_t> signingKeyBlob;
    Certificate signingKeyCertificate;
    ASSERT_TRUE(credential->generateSigningKeyPair(&signingKeyBlob, &signingKeyCertificate).isOk());
    optional<vector<uint8_t>> signingPubKey =
            support::certificateChainGetTopMostKey(signingKeyCertificate.encodedCertificate);
    EXPECT_TRUE(signingPubKey);
    test_utils::verifyAuthKeyCertificate(signingKeyCertificate.encodedCertificate);

    // Since we're using a test-credential we know storageKey meaning we can get the
    // private key. Do this, derive the public key from it, and check this matches what
    // is in the certificate...
    const vector<uint8_t> exDocTypeVec(exDocType.begin(), exDocType.end());
    optional<vector<uint8_t>> exSigningPrivKey =
            support::decryptAes128Gcm(exStorageKey, signingKeyBlob, exDocTypeVec);
    ASSERT_TRUE(exSigningPrivKey);
    optional<vector<uint8_t>> exSigningKeyPair =
            support::ecPrivateKeyToKeyPair(exSigningPrivKey.value());
    ASSERT_TRUE(exSigningKeyPair);
    optional<vector<uint8_t>> exSigningPubKey =
            support::ecKeyPairGetPublicKey(exSigningKeyPair.value());
    ASSERT_TRUE(exSigningPubKey);
    ASSERT_EQ(exSigningPubKey.value(), signingPubKey.value());

    vector<RequestNamespace> requestedNamespaces = test_utils::buildRequestNamespaces(testEntries);
    // OK to fail, not available in v1 HAL
    credential->setRequestedNamespaces(requestedNamespaces);
    // OK to fail, not available in v1 HAL
    credential->setVerificationToken(verificationToken);
    ASSERT_TRUE(credential
                        ->startRetrieval(secureProfiles.value(), authToken, itemsRequestBytes,
                                         signingKeyBlob, sessionTranscriptEncoded,
                                         readerSignature.value(), testEntriesEntryCounts)
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

        // TODO: also use |exStorageKey| to decrypt data and check it's the same as whatt
        // the HAL returns...
    }

    vector<uint8_t> mac;
    vector<uint8_t> deviceNameSpacesEncoded;
    ASSERT_TRUE(credential->finishRetrieval(&mac, &deviceNameSpacesEncoded).isOk());
    cborPretty = support::cborPrettyPrint(deviceNameSpacesEncoded, 32, {});
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

    string docType = "org.iso.18013-5.2019.mdl";
    optional<vector<uint8_t>> readerEphemeralPrivateKey =
            support::ecKeyPairGetPrivateKey(readerEphemeralKeyPair.value());
    optional<vector<uint8_t>> eMacKey = support::calcEMacKey(
            readerEphemeralPrivateKey.value(),                           // Private Key
            signingPubKey.value(),                                       // Public Key
            cppbor::Semantic(24, sessionTranscript.encode()).encode());  // SessionTranscriptBytes
    optional<vector<uint8_t>> calculatedMac =
            support::calcMac(sessionTranscript.encode(),  // SessionTranscript
                             docType,                     // DocType
                             deviceNameSpacesEncoded,     // DeviceNamespaces
                             eMacKey.value());            // EMacKey
    ASSERT_TRUE(calculatedMac);
    EXPECT_EQ(mac, calculatedMac);

    // Also perform an additional empty request. This is what mDL applications
    // are envisioned to do - one call to get the data elements, another to get
    // an empty DeviceSignedItems and corresponding MAC.
    //
    credential->setRequestedNamespaces({});  // OK to fail, not available in v1 HAL
    ASSERT_TRUE(credential
                        ->startRetrieval(
                                secureProfiles.value(), authToken, {},         // itemsRequestBytes
                                signingKeyBlob, sessionTranscriptEncoded, {},  // readerSignature,
                                testEntriesEntryCounts)
                        .isOk());
    ASSERT_TRUE(credential->finishRetrieval(&mac, &deviceNameSpacesEncoded).isOk());
    cborPretty = support::cborPrettyPrint(deviceNameSpacesEncoded, 32, {});
    ASSERT_EQ("{}", cborPretty);
    // Calculate DeviceAuthentication and MAC (MACing key hasn't changed)
    calculatedMac = support::calcMac(sessionTranscript.encode(),  // SessionTranscript
                                     docType,                     // DocType
                                     deviceNameSpacesEncoded,     // DeviceNamespaces
                                     eMacKey.value());            // EMacKey
    ASSERT_TRUE(calculatedMac);
    EXPECT_EQ(mac, calculatedMac);

    // Some mDL apps might send a request but with a single empty
    // namespace. Check that too.
    RequestNamespace emptyRequestNS;
    emptyRequestNS.namespaceName = "PersonalData";
    credential->setRequestedNamespaces({emptyRequestNS});  // OK to fail, not available in v1 HAL
    ASSERT_TRUE(credential
                        ->startRetrieval(
                                secureProfiles.value(), authToken, {},         // itemsRequestBytes
                                signingKeyBlob, sessionTranscriptEncoded, {},  // readerSignature,
                                testEntriesEntryCounts)
                        .isOk());
    ASSERT_TRUE(credential->finishRetrieval(&mac, &deviceNameSpacesEncoded).isOk());
    cborPretty = support::cborPrettyPrint(deviceNameSpacesEncoded, 32, {});
    ASSERT_EQ("{}", cborPretty);
    // Calculate DeviceAuthentication and MAC (MACing key hasn't changed)
    calculatedMac = support::calcMac(sessionTranscript.encode(),  // SessionTranscript
                                     docType,                     // DocType
                                     deviceNameSpacesEncoded,     // DeviceNamespaces
                                     eMacKey.value());            // EMacKey
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
