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

#define LOG_TAG "Util"

#include "Util.h"

#include <android-base/logging.h>

#include <KeyMintAidlTestBase.h>
#include <aidl/Gtest.h>
#include <aidl/android/hardware/security/keymint/MacedPublicKey.h>
#include <android-base/stringprintf.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <keymasterV4_1/attestation_record.h>
#include <keymint_support/openssl_utils.h>
#include <openssl/evp.h>

#include <charconv>
#include <map>

namespace android::hardware::identity::test_utils {

using std::endl;
using std::map;
using std::optional;
using std::string;
using std::vector;

using ::aidl::android::hardware::security::keymint::test::check_maced_pubkey;
using ::aidl::android::hardware::security::keymint::test::p256_pub_key;
using ::android::sp;
using ::android::String16;
using ::android::base::StringPrintf;
using ::android::binder::Status;
using ::android::hardware::security::keymint::MacedPublicKey;
using ::keymaster::X509_Ptr;

bool setupWritableCredential(sp<IWritableIdentityCredential>& writableCredential,
                             sp<IIdentityCredentialStore>& credentialStore, bool testCredential) {
    if (credentialStore == nullptr) {
        return false;
    }

    string docType = "org.iso.18013-5.2019.mdl";
    Status result = credentialStore->createCredential(docType, testCredential, &writableCredential);

    if (result.isOk() && writableCredential != nullptr) {
        return true;
    } else {
        return false;
    }
}

optional<vector<vector<uint8_t>>> createFakeRemotelyProvisionedCertificateChain(
        const MacedPublicKey& macedPublicKey) {
    // The helper library uses the NDK symbols, so play a little trickery here to convert
    // the data into the proper type so we can reuse the helper function to get the pubkey.
    ::aidl::android::hardware::security::keymint::MacedPublicKey ndkMacedPublicKey;
    ndkMacedPublicKey.macedKey = macedPublicKey.macedKey;

    vector<uint8_t> publicKeyBits;
    check_maced_pubkey(ndkMacedPublicKey, /*testMode=*/true, &publicKeyBits);

    ::aidl::android::hardware::security::keymint::EVP_PKEY_Ptr publicKey;
    p256_pub_key(publicKeyBits, &publicKey);

    // Generate an arbitrary root key for our chain
    bssl::UniquePtr<EC_KEY> ecRootKey(EC_KEY_new());
    bssl::UniquePtr<EVP_PKEY> rootKey(EVP_PKEY_new());
    if (ecRootKey.get() == nullptr || rootKey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }

    bssl::UniquePtr<EC_GROUP> group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        LOG(ERROR) << "Error creating EC group by curve name";
        return {};
    }

    if (EC_KEY_set_group(ecRootKey.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ecRootKey.get()) != 1 || EC_KEY_check_key(ecRootKey.get()) < 0) {
        LOG(ERROR) << "Error generating key";
        return {};
    }

    if (EVP_PKEY_set1_EC_KEY(rootKey.get(), ecRootKey.get()) != 1) {
        LOG(ERROR) << "Error getting private key";
        return {};
    }

    // The VTS test does not fully validate the chain, so we're ok without the proper CA extensions.
    map<string, vector<uint8_t>> extensions;

    // Now make a self-signed cert
    optional<vector<uint8_t>> root = support::ecPublicKeyGenerateCertificate(
            rootKey.get(), rootKey.get(),
            /*serialDecimal=*/"31415",
            /*subject=*/"Android IdentityCredential VTS Test Root Certificate",
            /*subject=*/"Android IdentityCredential VTS Test Root Certificate",
            /*validityNotBefore=*/time(nullptr),
            /*validityNotAfter=*/time(nullptr) + 365 * 24 * 3600, extensions);
    if (!root) {
        LOG(ERROR) << "Error generating root cert";
        return std::nullopt;
    }

    // Now sign a CA cert so that we have a chain that's good enough to satisfy
    // the VTS tests.
    optional<vector<uint8_t>> intermediate = support::ecPublicKeyGenerateCertificate(
            publicKey.get(), rootKey.get(),
            /*serialDecimal=*/"42",
            /*subject=*/"Android IdentityCredential VTS Test Root Certificate",
            /*subject=*/"Android IdentityCredential VTS Test Attestation Certificate",
            /*validityNotBefore=*/time(nullptr),
            /*validityNotAfter=*/time(nullptr) + 365 * 24 * 3600, extensions);
    if (!intermediate) {
        LOG(ERROR) << "Error generating intermediate cert";
        return std::nullopt;
    }

    return vector<vector<uint8_t>>{std::move(*intermediate), std::move(*root)};
}

optional<vector<uint8_t>> generateReaderCertificate(string serialDecimal) {
    vector<uint8_t> privKey;
    return generateReaderCertificate(serialDecimal, &privKey);
}

optional<vector<uint8_t>> generateReaderCertificate(string serialDecimal,
                                                    vector<uint8_t>* outReaderPrivateKey) {
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

    if (outReaderPrivateKey == nullptr) {
        return {};
    }

    *outReaderPrivateKey = readerKey.value();

    string issuer = "Android Open Source Project";
    string subject = "Android IdentityCredential VTS Test";
    time_t validityNotBefore = time(nullptr);
    time_t validityNotAfter = validityNotBefore + 365 * 24 * 3600;

    return support::ecPublicKeyGenerateCertificate(readerPublicKey.value(), readerKey.value(),
                                                   serialDecimal, issuer, subject,
                                                   validityNotBefore, validityNotAfter, {});
}

optional<vector<SecureAccessControlProfile>> addAccessControlProfiles(
        sp<IWritableIdentityCredential>& writableCredential,
        const vector<TestProfile>& testProfiles) {
    Status result;

    vector<SecureAccessControlProfile> secureProfiles;

    for (const auto& testProfile : testProfiles) {
        SecureAccessControlProfile profile;
        Certificate cert;
        cert.encodedCertificate = testProfile.readerCertificate;
        int64_t secureUserId = testProfile.userAuthenticationRequired ? 66 : 0;
        result = writableCredential->addAccessControlProfile(
                testProfile.id, cert, testProfile.userAuthenticationRequired,
                testProfile.timeoutMillis, secureUserId, &profile);

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
bool addEntry(sp<IWritableIdentityCredential>& writableCredential, const TestEntryData& entry,
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

void setImageData(vector<uint8_t>& image) {
    image.resize(256 * 1024 - 10);
    for (size_t n = 0; n < image.size(); n++) {
        image[n] = (uint8_t)n;
    }
}

string x509NameToRfc2253String(X509_NAME* name) {
    char* buf;
    size_t bufSize;
    BIO* bio;

    bio = BIO_new(BIO_s_mem());
    X509_NAME_print_ex(bio, name, 0, XN_FLAG_RFC2253);
    bufSize = BIO_get_mem_data(bio, &buf);
    string ret = string(buf, bufSize);
    BIO_free(bio);

    return ret;
}

int parseDigits(const char** s, int numDigits) {
    int result;
    auto [_, ec] = std::from_chars(*s, *s + numDigits, result);
    if (ec != std::errc()) {
        LOG(ERROR) << "Error parsing " << numDigits << " digits "
                   << " from " << s;
        return 0;
    }
    *s += numDigits;
    return result;
}

bool parseAsn1Time(const ASN1_TIME* asn1Time, time_t* outTime) {
    struct tm tm;

    memset(&tm, '\0', sizeof(tm));
    const char* timeStr = (const char*)asn1Time->data;
    const char* s = timeStr;
    if (asn1Time->type == V_ASN1_UTCTIME) {
        tm.tm_year = parseDigits(&s, 2);
        if (tm.tm_year < 70) {
            tm.tm_year += 100;
        }
    } else if (asn1Time->type == V_ASN1_GENERALIZEDTIME) {
        tm.tm_year = parseDigits(&s, 4) - 1900;
        tm.tm_year -= 1900;
    } else {
        LOG(ERROR) << "Unsupported ASN1_TIME type " << asn1Time->type;
        return false;
    }
    tm.tm_mon = parseDigits(&s, 2) - 1;
    tm.tm_mday = parseDigits(&s, 2);
    tm.tm_hour = parseDigits(&s, 2);
    tm.tm_min = parseDigits(&s, 2);
    tm.tm_sec = parseDigits(&s, 2);
    // This may need to be updated if someone create certificates using +/- instead of Z.
    //
    if (*s != 'Z') {
        LOG(ERROR) << "Expected Z in string '" << timeStr << "' at offset " << (s - timeStr);
        return false;
    }

    time_t t = timegm(&tm);
    if (t == -1) {
        LOG(ERROR) << "Error converting broken-down time to time_t";
        return false;
    }
    *outTime = t;
    return true;
}

void validateAttestationCertificate(const vector<Certificate>& credentialKeyCertChain,
                                    const vector<uint8_t>& expectedChallenge,
                                    const vector<uint8_t>& expectedAppId, bool isTestCredential) {
    ASSERT_GE(credentialKeyCertChain.size(), 2);

    vector<uint8_t> certBytes = credentialKeyCertChain[0].encodedCertificate;
    const uint8_t* certData = certBytes.data();
    X509_Ptr cert = X509_Ptr(d2i_X509(nullptr, &certData, certBytes.size()));

    vector<uint8_t> batchCertBytes = credentialKeyCertChain[1].encodedCertificate;
    const uint8_t* batchCertData = batchCertBytes.data();
    X509_Ptr batchCert = X509_Ptr(d2i_X509(nullptr, &batchCertData, batchCertBytes.size()));

    // First get some values from the batch certificate which is checked
    // against the top-level certificate (subject, notAfter)
    //

    X509_NAME* batchSubject = X509_get_subject_name(batchCert.get());
    ASSERT_NE(nullptr, batchSubject);
    time_t batchNotAfter;
    ASSERT_TRUE(parseAsn1Time(X509_get0_notAfter(batchCert.get()), &batchNotAfter));

    // Check all the requirements from IWritableIdentityCredential::getAttestationCertificate()...
    //

    //  - version: INTEGER 2 (means v3 certificate).
    EXPECT_EQ(2, X509_get_version(cert.get()));

    //  - serialNumber: INTEGER 1 (fixed value: same on all certs).
    EXPECT_EQ(1, ASN1_INTEGER_get(X509_get_serialNumber(cert.get())));

    //  - signature: must be set to ECDSA.
    EXPECT_EQ(NID_ecdsa_with_SHA256, X509_get_signature_nid(cert.get()));

    //  - subject: CN shall be set to "Android Identity Credential Key". (fixed value:
    //    same on all certs)
    X509_NAME* subject = X509_get_subject_name(cert.get());
    ASSERT_NE(nullptr, subject);
    EXPECT_EQ("CN=Android Identity Credential Key", x509NameToRfc2253String(subject));

    //  - issuer: Same as the subject field of the batch attestation key.
    X509_NAME* issuer = X509_get_issuer_name(cert.get());
    ASSERT_NE(nullptr, issuer);
    EXPECT_EQ(x509NameToRfc2253String(batchSubject), x509NameToRfc2253String(issuer));

    //  - validity: Should be from current time and expire at the same time as the
    //    attestation batch certificate used.
    //
    //  Allow for 10 seconds drift to account for the time drift between Secure HW
    //  and this environment plus the difference between when the certificate was
    //  created and until now
    //
    time_t notBefore;
    ASSERT_TRUE(parseAsn1Time(X509_get0_notBefore(cert.get()), &notBefore));
    uint64_t now = time(nullptr);
    int64_t diffSecs = now - notBefore;
    int64_t allowDriftSecs = 10;
    EXPECT_LE(-allowDriftSecs, diffSecs);
    EXPECT_GE(allowDriftSecs, diffSecs);

    time_t notAfter;
    ASSERT_TRUE(parseAsn1Time(X509_get0_notAfter(cert.get()), &notAfter));
    EXPECT_EQ(notAfter, batchNotAfter);

    auto [err, attRec] = keymaster::V4_1::parse_attestation_record(certBytes);
    ASSERT_EQ(keymaster::V4_1::ErrorCode::OK, err);

    //  - subjectPublicKeyInfo: must contain attested public key.

    //  - The attestationVersion field in the attestation extension must be at least 3.
    EXPECT_GE(attRec.attestation_version, 3);

    //  - The attestationSecurityLevel field must be set to either Software (0),
    //    TrustedEnvironment (1), or StrongBox (2) depending on how attestation is
    //    implemented.
    EXPECT_GE(attRec.attestation_security_level,
              keymaster::V4_0::SecurityLevel::TRUSTED_ENVIRONMENT);

    //  - The keymasterVersion field in the attestation extension must be set to the.
    //    same value as used for Android Keystore keys.
    //
    // Nothing to check here...

    //  - The keymasterSecurityLevel field in the attestation extension must be set to
    //    either Software (0), TrustedEnvironment (1), or StrongBox (2) depending on how
    //    the Trusted Application backing the HAL implementation is implemented.
    EXPECT_GE(attRec.keymaster_security_level, keymaster::V4_0::SecurityLevel::TRUSTED_ENVIRONMENT);

    //  - The attestationChallenge field must be set to the passed-in challenge.
    EXPECT_EQ(expectedChallenge.size(), attRec.attestation_challenge.size());
    EXPECT_TRUE(memcmp(expectedChallenge.data(), attRec.attestation_challenge.data(),
                       attRec.attestation_challenge.size()) == 0);

    //  - The uniqueId field must be empty.
    EXPECT_EQ(attRec.unique_id.size(), 0);

    //  - The softwareEnforced field in the attestation extension must include
    //    Tag::ATTESTATION_APPLICATION_ID which must be set to the bytes of the passed-in
    //    attestationApplicationId.
    EXPECT_TRUE(attRec.software_enforced.Contains(keymaster::V4_0::TAG_ATTESTATION_APPLICATION_ID,
                                                  expectedAppId));

    //  - The teeEnforced field in the attestation extension must include
    //
    //    - Tag::IDENTITY_CREDENTIAL_KEY which indicates that the key is an Identity
    //      Credential key (which can only sign/MAC very specific messages) and not an Android
    //      Keystore key (which can be used to sign/MAC anything). This must not be set
    //      for test credentials.
    bool hasIcKeyTag =
            attRec.hardware_enforced.Contains(static_cast<android::hardware::keymaster::V4_0::Tag>(
                    keymaster::V4_1::Tag::IDENTITY_CREDENTIAL_KEY));
    if (isTestCredential) {
        EXPECT_FALSE(hasIcKeyTag);
    } else {
        EXPECT_TRUE(hasIcKeyTag);
    }

    //    - Tag::PURPOSE must be set to SIGN
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_PURPOSE,
                                                  keymaster::V4_0::KeyPurpose::SIGN));

    //    - Tag::KEY_SIZE must be set to the appropriate key size, in bits (e.g. 256)
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_KEY_SIZE, 256));

    //    - Tag::ALGORITHM must be set to EC
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_ALGORITHM,
                                                  keymaster::V4_0::Algorithm::EC));

    //    - Tag::NO_AUTH_REQUIRED must be set
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_NO_AUTH_REQUIRED));

    //    - Tag::DIGEST must be include SHA_2_256
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_DIGEST,
                                                  keymaster::V4_0::Digest::SHA_2_256));

    //    - Tag::EC_CURVE must be set to P_256
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_EC_CURVE,
                                                  keymaster::V4_0::EcCurve::P_256));

    //    - Tag::ROOT_OF_TRUST must be set
    //
    EXPECT_GE(attRec.root_of_trust.security_level,
              keymaster::V4_0::SecurityLevel::TRUSTED_ENVIRONMENT);

    //    - Tag::OS_VERSION and Tag::OS_PATCHLEVEL must be set
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_OS_VERSION));
    EXPECT_TRUE(attRec.hardware_enforced.Contains(keymaster::V4_0::TAG_OS_PATCHLEVEL));

    // TODO: we could retrieve osVersion and osPatchLevel from Android itself and compare it
    // with what was reported in the certificate.
}

void verifyAuthKeyCertificate(const vector<uint8_t>& authKeyCertChain) {
    const uint8_t* data = authKeyCertChain.data();
    auto cert = X509_Ptr(d2i_X509(nullptr, &data, authKeyCertChain.size()));

    //  - version: INTEGER 2 (means v3 certificate).
    EXPECT_EQ(X509_get_version(cert.get()), 2);

    //  - serialNumber: INTEGER 1 (fixed value: same on all certs).
    EXPECT_EQ(ASN1_INTEGER_get(X509_get_serialNumber(cert.get())), 1);

    //  - signature: must be set to ECDSA.
    EXPECT_EQ(X509_get_signature_nid(cert.get()), NID_ecdsa_with_SHA256);

    //  - subject: CN shall be set to "Android Identity Credential Authentication Key". (fixed
    //    value: same on all certs)
    X509_NAME* subject = X509_get_subject_name(cert.get());
    ASSERT_NE(subject, nullptr);
    EXPECT_EQ(x509NameToRfc2253String(subject),
              "CN=Android Identity Credential Authentication Key");

    //  - issuer: CN shall be set to "Android Identity Credential Key". (fixed value:
    //    same on all certs)
    X509_NAME* issuer = X509_get_issuer_name(cert.get());
    ASSERT_NE(issuer, nullptr);
    EXPECT_EQ(x509NameToRfc2253String(issuer), "CN=Android Identity Credential Key");

    //  - subjectPublicKeyInfo: must contain attested public key.

    //  - validity: should be from current time and one year in the future (365 days).
    time_t notBefore, notAfter;
    ASSERT_TRUE(parseAsn1Time(X509_get0_notAfter(cert.get()), &notAfter));
    ASSERT_TRUE(parseAsn1Time(X509_get0_notBefore(cert.get()), &notBefore));

    //  Allow for 10 seconds drift to account for the time drift between Secure HW
    //  and this environment plus the difference between when the certificate was
    //  created and until now
    //
    uint64_t now = time(nullptr);
    int64_t diffSecs = now - notBefore;
    int64_t allowDriftSecs = 10;
    EXPECT_LE(-allowDriftSecs, diffSecs);
    EXPECT_GE(allowDriftSecs, diffSecs);

    // The AIDL spec used to call for "one year in the future (365
    // days)" but was updated to say "current time and 31536000
    // seconds in the future (approximately 365 days)" to clarify that
    // this was the original intention.
    //
    // However a number of implementations interpreted this as a
    // "literal year" which started causing problems in March 2023
    // because 2024 is a leap year. Since the extra day doesn't really
    // matter (the validity period is specified in the MSO anyway and
    // that's what RPs use), we allow both interpretations.
    //
    // For simplicity, we just require that that notAfter is after
    // 31536000 and which also covers the case if there's a leap-day
    // and possible leap-seconds.
    //
    constexpr uint64_t kSecsIn365Days = 365 * 24 * 60 * 60;
    EXPECT_LE(notBefore + kSecsIn365Days, notAfter);
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
