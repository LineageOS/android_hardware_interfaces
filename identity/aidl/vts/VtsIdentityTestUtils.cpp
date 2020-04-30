/*
 * Copyright 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VtsIdentityTestUtils.h"

#include <aidl/Gtest.h>
#include <map>

namespace android::hardware::identity::test_utils {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::android::sp;
using ::android::String16;
using ::android::binder::Status;

bool SetupWritableCredential(sp<IWritableIdentityCredential>& writableCredential,
                             sp<IIdentityCredentialStore>& credentialStore) {
    if (credentialStore == nullptr) {
        return false;
    }

    string docType = "org.iso.18013-5.2019.mdl";
    bool testCredential = true;
    Status result = credentialStore->createCredential(docType, testCredential, &writableCredential);

    if (result.isOk() && writableCredential != nullptr) {
        return true;
    } else {
        return false;
    }
}

optional<vector<uint8_t>> GenerateReaderCertificate(string serialDecimal) {
    vector<uint8_t> privKey;
    return GenerateReaderCertificate(serialDecimal, privKey);
}

optional<vector<uint8_t>> GenerateReaderCertificate(string serialDecimal,
                                                    vector<uint8_t>& readerPrivateKey) {
    optional<vector<uint8_t>> readerKeyPKCS8 = support::createEcKeyPair();
    if (!readerKeyPKCS8) {
        return {};
    }

    optional<vector<uint8_t>> readerPublicKey =
            support::ecKeyPairGetPublicKey(readerKeyPKCS8.value());
    optional<vector<uint8_t>> readerKey = support::ecKeyPairGetPrivateKey(readerKeyPKCS8.value());
    if (!readerPublicKey || !readerKey) {
        return {};
    }

    readerPrivateKey = readerKey.value();

    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential VTS Test";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    return support::ecPublicKeyGenerateCertificate(readerPublicKey.value(), readerKey.value(),
                                                   serialDecimal, issuer, subject,
                                                   validityNotBefore, validityNotAfter);
}

optional<vector<SecureAccessControlProfile>> AddAccessControlProfiles(
        sp<IWritableIdentityCredential>& writableCredential,
        const vector<TestProfile>& testProfiles) {
    Status result;

    vector<SecureAccessControlProfile> secureProfiles;

    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, 0, &profile);

        // Don't use assert so all errors can be outputed.  Then return
        // instead of exit even on errors so caller can decide.
        EXPECT_TRUE(result.isOk()) << result.exceptionCode() << "; " << result.exceptionMessage()
                                   << "test profile id = " << testProfile.id << endl;
        EXPECT_EQ(testProfile.id, profile.id);
        EXPECT_EQ(testProfile.readerCertificate, profile.readerCertificate.encodedCertificate);
        EXPECT_EQ(testProfile.userAuthenticationRequired, profile.userAuthenticationRequired);
        EXPECT_EQ(testProfile.timeoutMillis, profile.timeoutMillis);
        EXPECT_EQ(support::kAesGcmTagSize + support::kAesGcmIvSize, profile.mac.size());

        if (!result.isOk() || testProfile.id != profile.id ||
            testProfile.readerCertificate != profile.readerCertificate.encodedCertificate ||
            testProfile.userAuthenticationRequired != profile.userAuthenticationRequired ||
            testProfile.timeoutMillis != profile.timeoutMillis ||
            support::kAesGcmTagSize + support::kAesGcmIvSize != profile.mac.size()) {
            return {};
        }

        secureProfiles.push_back(profile);
    }

    return secureProfiles;
}

// Most test expects this function to pass. So we will print out additional
// value if failed so more debug data can be provided.
bool AddEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
              int dataChunkSize, map<const TestEntryData*, vector<vector<uint8_t>>>& encryptedBlobs,
              bool expectSuccess) {
    Status result;
    vector<vector<uint8_t>> chunks = support::chunkVector(entry.valueCbor, dataChunkSize);

    result = writableCredential->beginAddEntry(entry.profileIds, entry.nameSpace, entry.name,
                                               entry.valueCbor.size());

    if (expectSuccess) {
        EXPECT_TRUE(result.isOk())
                << result.exceptionCode() << "; " << result.exceptionMessage() << endl
                << "entry name = " << entry.name << ", name space=" << entry.nameSpace << endl;
    }

    if (!result.isOk()) {
        return false;
    }

    vector<vector<uint8_t>> encryptedChunks;
    for (const auto& chunk : chunks) {
        vector<uint8_t> encryptedContent;
        result = writableCredential->addEntryValue(chunk, &encryptedContent);
        if (expectSuccess) {
            EXPECT_TRUE(result.isOk())
                    << result.exceptionCode() << "; " << result.exceptionMessage() << endl
                    << "entry name = " << entry.name << ", name space = " << entry.nameSpace
                    << endl;

            EXPECT_GT(encryptedContent.size(), 0u) << "entry name = " << entry.name
                                                   << ", name space = " << entry.nameSpace << endl;
        }

        if (!result.isOk() || encryptedContent.size() <= 0u) {
            return false;
        }

        encryptedChunks.push_back(encryptedContent);
    }

    encryptedBlobs[&entry] = encryptedChunks;
    return true;
}

bool ValidateAttestationCertificate(vector<Certificate>& inputCertificates) {
    return (inputCertificates.size() >= 2);
    // TODO: add parsing of the certificate and make sure it is genuine.
}

void SetImageData(vector<uint8_t>& image) {
    image.resize(256 * 1024 - 10);
    for (size_t n = 0; n < image.size(); n++) {
        image[n] = (uint8_t)n;
    }
}

vector<RequestNamespace> buildRequestNamespaces(const vector<TestEntryData> entries) {
    vector<RequestNamespace> ret;
    RequestNamespace curNs;
    for (const TestEntryData& testEntry : entries) {
        if (testEntry.nameSpace != curNs.namespaceName) {
            if (curNs.namespaceName.size() > 0) {
                ret.push_back(curNs);
            }
            curNs.namespaceName = testEntry.nameSpace;
            curNs.items.clear();
        }

        RequestDataItem item;
        item.name = testEntry.name;
        item.size = testEntry.valueCbor.size();
        item.accessControlProfileIds = testEntry.profileIds;
        curNs.items.push_back(item);
    }
    if (curNs.namespaceName.size() > 0) {
        ret.push_back(curNs);
    }
    return ret;
}

}  // namespace android::hardware::identity::test_utils
