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

#define LOG_TAG "IdentityCredentialSupport"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#define _POSIX_C_SOURCE 199309L

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <chrono>
#include <iomanip>

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hkdf.h>
#include <openssl/hmac.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <charconv>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <android/hardware/keymaster/4.0/types.h>
#include <keymaster/authorization_set.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/contexts/soft_attestation_cert.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/km_openssl/attestation_utils.h>
#include <keymaster/km_openssl/certificate_utils.h>

namespace android {
namespace hardware {
namespace identity {
namespace support {

using ::std::pair;
using ::std::unique_ptr;

// ---------------------------------------------------------------------------
// Miscellaneous utilities.
// ---------------------------------------------------------------------------

void hexdump(const string& name, const vector<uint8_t>& data) {
    fprintf(stderr, "%s: dumping %zd bytes\n", name.c_str(), data.size());
    size_t n, m, o;
    for (n = 0; n < data.size(); n += 16) {
        fprintf(stderr, "%04zx  ", n);
        for (m = 0; m < 16 && n + m < data.size(); m++) {
            fprintf(stderr, "%02x ", data[n + m]);
        }
        for (o = m; o < 16; o++) {
            fprintf(stderr, "   ");
        }
        fprintf(stderr, " ");
        for (m = 0; m < 16 && n + m < data.size(); m++) {
            int c = data[n + m];
            fprintf(stderr, "%c", isprint(c) ? c : '.');
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

string encodeHex(const uint8_t* data, size_t dataLen) {
    static const char hexDigits[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    string ret;
    ret.resize(dataLen * 2);
    for (size_t n = 0; n < dataLen; n++) {
        uint8_t byte = data[n];
        ret[n * 2 + 0] = hexDigits[byte >> 4];
        ret[n * 2 + 1] = hexDigits[byte & 0x0f];
    }

    return ret;
}

string encodeHex(const string& str) {
    return encodeHex(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

string encodeHex(const vector<uint8_t>& data) {
    return encodeHex(data.data(), data.size());
}

// Returns -1 on error, otherwise an integer in the range 0 through 15, both inclusive.
int parseHexDigit(char hexDigit) {
    if (hexDigit >= '0' && hexDigit <= '9') {
        return int(hexDigit) - '0';
    } else if (hexDigit >= 'a' && hexDigit <= 'f') {
        return int(hexDigit) - 'a' + 10;
    } else if (hexDigit >= 'A' && hexDigit <= 'F') {
        return int(hexDigit) - 'A' + 10;
    }
    return -1;
}

optional<vector<uint8_t>> decodeHex(const string& hexEncoded) {
    vector<uint8_t> out;
    size_t hexSize = hexEncoded.size();
    if ((hexSize & 1) != 0) {
        LOG(ERROR) << "Size of data cannot be odd";
        return {};
    }

    out.resize(hexSize / 2);
    for (size_t n = 0; n < hexSize / 2; n++) {
        int upperNibble = parseHexDigit(hexEncoded[n * 2]);
        int lowerNibble = parseHexDigit(hexEncoded[n * 2 + 1]);
        if (upperNibble == -1 || lowerNibble == -1) {
            LOG(ERROR) << "Invalid hex digit at position " << n;
            return {};
        }
        out[n] = (upperNibble << 4) + lowerNibble;
    }

    return out;
}

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

using EvpCipherCtxPtr = bssl::UniquePtr<EVP_CIPHER_CTX>;
using EC_KEY_Ptr = bssl::UniquePtr<EC_KEY>;
using EVP_PKEY_Ptr = bssl::UniquePtr<EVP_PKEY>;
using EVP_PKEY_CTX_Ptr = bssl::UniquePtr<EVP_PKEY_CTX>;
using EC_GROUP_Ptr = bssl::UniquePtr<EC_GROUP>;
using EC_POINT_Ptr = bssl::UniquePtr<EC_POINT>;
using ECDSA_SIG_Ptr = bssl::UniquePtr<ECDSA_SIG>;
using X509_Ptr = bssl::UniquePtr<X509>;
using PKCS12_Ptr = bssl::UniquePtr<PKCS12>;
using BIGNUM_Ptr = bssl::UniquePtr<BIGNUM>;
using ASN1_INTEGER_Ptr = bssl::UniquePtr<ASN1_INTEGER>;
using ASN1_TIME_Ptr = bssl::UniquePtr<ASN1_TIME>;
using ASN1_OCTET_STRING_Ptr = bssl::UniquePtr<ASN1_OCTET_STRING>;
using ASN1_OBJECT_Ptr = bssl::UniquePtr<ASN1_OBJECT>;
using X509_NAME_Ptr = bssl::UniquePtr<X509_NAME>;
using X509_EXTENSION_Ptr = bssl::UniquePtr<X509_EXTENSION>;

// bool getRandom(size_t numBytes, vector<uint8_t>& output) {
optional<vector<uint8_t>> getRandom(size_t numBytes) {
    vector<uint8_t> output;
    output.resize(numBytes);
    if (RAND_bytes(output.data(), numBytes) != 1) {
        LOG(ERROR) << "RAND_bytes: failed getting " << numBytes << " random";
        return {};
    }
    return output;
}

optional<vector<uint8_t>> decryptAes128Gcm(const vector<uint8_t>& key,
                                           const vector<uint8_t>& encryptedData,
                                           const vector<uint8_t>& additionalAuthenticatedData) {
    int cipherTextSize = int(encryptedData.size()) - kAesGcmIvSize - kAesGcmTagSize;
    if (cipherTextSize < 0) {
        LOG(ERROR) << "encryptedData too small";
        return {};
    }
    unsigned char* nonce = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = nonce + kAesGcmIvSize;
    unsigned char* tag = cipherText + cipherTextSize;

    vector<uint8_t> plainText;
    plainText.resize(cipherTextSize);

    auto ctx = EvpCipherCtxPtr(EVP_CIPHER_CTX_new());
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "EVP_CIPHER_CTX_new: failed";
        return {};
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, NULL, NULL) != 1) {
        LOG(ERROR) << "EVP_DecryptInit_ex: failed";
        return {};
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kAesGcmIvSize, NULL) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting nonce length";
        return {};
    }

    if (EVP_DecryptInit_ex(ctx.get(), NULL, NULL, (unsigned char*)key.data(), nonce) != 1) {
        LOG(ERROR) << "EVP_DecryptInit_ex: failed";
        return {};
    }

    int numWritten;
    if (additionalAuthenticatedData.size() > 0) {
        if (EVP_DecryptUpdate(ctx.get(), NULL, &numWritten,
                              (unsigned char*)additionalAuthenticatedData.data(),
                              additionalAuthenticatedData.size()) != 1) {
            LOG(ERROR) << "EVP_DecryptUpdate: failed for additionalAuthenticatedData";
            return {};
        }
        if ((size_t)numWritten != additionalAuthenticatedData.size()) {
            LOG(ERROR) << "EVP_DecryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << additionalAuthenticatedData.size() << ") for additionalAuthenticatedData";
            return {};
        }
    }

    if (EVP_DecryptUpdate(ctx.get(), (unsigned char*)plainText.data(), &numWritten, cipherText,
                          cipherTextSize) != 1) {
        LOG(ERROR) << "EVP_DecryptUpdate: failed";
        return {};
    }
    if (numWritten != cipherTextSize) {
        LOG(ERROR) << "EVP_DecryptUpdate: Unexpected outl=" << numWritten << " (expected "
                   << cipherTextSize << ")";
        return {};
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kAesGcmTagSize, tag)) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting expected tag";
        return {};
    }

    int ret = EVP_DecryptFinal_ex(ctx.get(), (unsigned char*)plainText.data() + numWritten,
                                  &numWritten);
    if (ret != 1) {
        LOG(ERROR) << "EVP_DecryptFinal_ex: failed";
        return {};
    }
    if (numWritten != 0) {
        LOG(ERROR) << "EVP_DecryptFinal_ex: Unexpected non-zero outl=" << numWritten;
        return {};
    }

    return plainText;
}

optional<vector<uint8_t>> encryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& nonce,
                                           const vector<uint8_t>& data,
                                           const vector<uint8_t>& additionalAuthenticatedData) {
    if (key.size() != kAes128GcmKeySize) {
        LOG(ERROR) << "key is not kAes128GcmKeySize bytes";
        return {};
    }
    if (nonce.size() != kAesGcmIvSize) {
        LOG(ERROR) << "nonce is not kAesGcmIvSize bytes";
        return {};
    }

    // The result is the nonce (kAesGcmIvSize bytes), the ciphertext, and
    // finally the tag (kAesGcmTagSize bytes).
    vector<uint8_t> encryptedData;
    encryptedData.resize(data.size() + kAesGcmIvSize + kAesGcmTagSize);
    unsigned char* noncePtr = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = noncePtr + kAesGcmIvSize;
    unsigned char* tag = cipherText + data.size();
    memcpy(noncePtr, nonce.data(), kAesGcmIvSize);

    auto ctx = EvpCipherCtxPtr(EVP_CIPHER_CTX_new());
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "EVP_CIPHER_CTX_new: failed";
        return {};
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, NULL, NULL) != 1) {
        LOG(ERROR) << "EVP_EncryptInit_ex: failed";
        return {};
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kAesGcmIvSize, NULL) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting nonce length";
        return {};
    }

    if (EVP_EncryptInit_ex(ctx.get(), NULL, NULL, (unsigned char*)key.data(),
                           (unsigned char*)nonce.data()) != 1) {
        LOG(ERROR) << "EVP_EncryptInit_ex: failed";
        return {};
    }

    int numWritten;
    if (additionalAuthenticatedData.size() > 0) {
        if (EVP_EncryptUpdate(ctx.get(), NULL, &numWritten,
                              (unsigned char*)additionalAuthenticatedData.data(),
                              additionalAuthenticatedData.size()) != 1) {
            LOG(ERROR) << "EVP_EncryptUpdate: failed for additionalAuthenticatedData";
            return {};
        }
        if ((size_t)numWritten != additionalAuthenticatedData.size()) {
            LOG(ERROR) << "EVP_EncryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << additionalAuthenticatedData.size() << ") for additionalAuthenticatedData";
            return {};
        }
    }

    if (data.size() > 0) {
        if (EVP_EncryptUpdate(ctx.get(), cipherText, &numWritten, (unsigned char*)data.data(),
                              data.size()) != 1) {
            LOG(ERROR) << "EVP_EncryptUpdate: failed";
            return {};
        }
        if ((size_t)numWritten != data.size()) {
            LOG(ERROR) << "EVP_EncryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << data.size() << ")";
            return {};
        }
    }

    if (EVP_EncryptFinal_ex(ctx.get(), cipherText + numWritten, &numWritten) != 1) {
        LOG(ERROR) << "EVP_EncryptFinal_ex: failed";
        return {};
    }
    if (numWritten != 0) {
        LOG(ERROR) << "EVP_EncryptFinal_ex: Unexpected non-zero outl=" << numWritten;
        return {};
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kAesGcmTagSize, tag) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed getting tag";
        return {};
    }

    return encryptedData;
}

vector<uint8_t> certificateChainJoin(const vector<vector<uint8_t>>& certificateChain) {
    vector<uint8_t> ret;
    for (const vector<uint8_t>& certificate : certificateChain) {
        ret.insert(ret.end(), certificate.begin(), certificate.end());
    }
    return ret;
}

optional<vector<vector<uint8_t>>> certificateChainSplit(const vector<uint8_t>& certificateChain) {
    const unsigned char* pStart = (unsigned char*)certificateChain.data();
    const unsigned char* p = pStart;
    const unsigned char* pEnd = p + certificateChain.size();
    vector<vector<uint8_t>> certificates;
    while (p < pEnd) {
        size_t begin = p - pStart;
        auto x509 = X509_Ptr(d2i_X509(nullptr, &p, pEnd - p));
        size_t next = p - pStart;
        if (x509 == nullptr) {
            LOG(ERROR) << "Error parsing X509 certificate";
            return {};
        }
        vector<uint8_t> cert =
                vector<uint8_t>(certificateChain.begin() + begin, certificateChain.begin() + next);
        certificates.push_back(std::move(cert));
    }
    return certificates;
}

static bool parseX509Certificates(const vector<uint8_t>& certificateChain,
                                  vector<X509_Ptr>& parsedCertificates) {
    const unsigned char* p = (unsigned char*)certificateChain.data();
    const unsigned char* pEnd = p + certificateChain.size();
    parsedCertificates.resize(0);
    while (p < pEnd) {
        auto x509 = X509_Ptr(d2i_X509(nullptr, &p, pEnd - p));
        if (x509 == nullptr) {
            LOG(ERROR) << "Error parsing X509 certificate";
            return false;
        }
        parsedCertificates.push_back(std::move(x509));
    }
    return true;
}

bool certificateSignedByPublicKey(const vector<uint8_t>& certificate,
                                  const vector<uint8_t>& publicKey) {
    const unsigned char* p = certificate.data();
    auto x509 = X509_Ptr(d2i_X509(nullptr, &p, certificate.size()));
    if (x509 == nullptr) {
        LOG(ERROR) << "Error parsing X509 certificate";
        return false;
    }

    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), publicKey.data(), publicKey.size(), nullptr) !=
        1) {
        LOG(ERROR) << "Error decoding publicKey";
        return false;
    }
    auto ecKey = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return false;
    }
    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1) {
        LOG(ERROR) << "Error setting group";
        return false;
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        LOG(ERROR) << "Error setting point";
        return false;
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error setting key";
        return false;
    }

    if (X509_verify(x509.get(), pkey.get()) != 1) {
        return false;
    }

    return true;
}

// TODO: Right now the only check we perform is to check that each certificate
//       is signed by its successor. We should - but currently don't - also check
//       things like valid dates etc.
//
//       It would be nice to use X509_verify_cert() instead of doing our own thing.
//
bool certificateChainValidate(const vector<uint8_t>& certificateChain) {
    vector<X509_Ptr> certs;

    if (!parseX509Certificates(certificateChain, certs)) {
        LOG(ERROR) << "Error parsing X509 certificates";
        return false;
    }

    if (certs.size() == 1) {
        return true;
    }

    for (size_t n = 1; n < certs.size(); n++) {
        const X509_Ptr& keyCert = certs[n - 1];
        const X509_Ptr& signingCert = certs[n];
        EVP_PKEY_Ptr signingPubkey(X509_get_pubkey(signingCert.get()));
        if (X509_verify(keyCert.get(), signingPubkey.get()) != 1) {
            LOG(ERROR) << "Error validating cert at index " << n - 1
                       << " is signed by its successor";
            return false;
        }
    }

    return true;
}

bool checkEcDsaSignature(const vector<uint8_t>& digest, const vector<uint8_t>& signature,
                         const vector<uint8_t>& publicKey) {
    const unsigned char* p = (unsigned char*)signature.data();
    auto sig = ECDSA_SIG_Ptr(d2i_ECDSA_SIG(nullptr, &p, signature.size()));
    if (sig.get() == nullptr) {
        LOG(ERROR) << "Error decoding DER encoded signature";
        return false;
    }

    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), publicKey.data(), publicKey.size(), nullptr) !=
        1) {
        LOG(ERROR) << "Error decoding publicKey";
        return false;
    }
    auto ecKey = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return false;
    }
    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1) {
        LOG(ERROR) << "Error setting group";
        return false;
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        LOG(ERROR) << "Error setting point";
        return false;
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error setting key";
        return false;
    }

    int rc = ECDSA_do_verify(digest.data(), digest.size(), sig.get(), ecKey.get());
    if (rc != 1) {
        LOG(ERROR) << "Error verifying signature (rc=" << rc << ")";
        return false;
    }

    return true;
}

vector<uint8_t> sha256(const vector<uint8_t>& data) {
    vector<uint8_t> ret;
    ret.resize(SHA256_DIGEST_LENGTH);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final((unsigned char*)ret.data(), &ctx);
    return ret;
}

optional<vector<uint8_t>> signEcDsaDigest(const vector<uint8_t>& key,
                                          const vector<uint8_t>& dataDigest) {
    auto bn = BIGNUM_Ptr(BN_bin2bn(key.data(), key.size(), nullptr));
    if (bn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM";
        return {};
    }

    auto ec_key = EC_KEY_Ptr(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if (EC_KEY_set_private_key(ec_key.get(), bn.get()) != 1) {
        LOG(ERROR) << "Error setting private key from BIGNUM";
        return {};
    }

    ECDSA_SIG* sig = ECDSA_do_sign(dataDigest.data(), dataDigest.size(), ec_key.get());
    if (sig == nullptr) {
        LOG(ERROR) << "Error signing digest";
        return {};
    }
    size_t len = i2d_ECDSA_SIG(sig, nullptr);
    vector<uint8_t> signature;
    signature.resize(len);
    unsigned char* p = (unsigned char*)signature.data();
    i2d_ECDSA_SIG(sig, &p);
    ECDSA_SIG_free(sig);
    return signature;
}

optional<vector<uint8_t>> signEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data) {
    return signEcDsaDigest(key, sha256(data));
}

optional<vector<uint8_t>> hmacSha256(const vector<uint8_t>& key, const vector<uint8_t>& data) {
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    if (HMAC_Init_ex(&ctx, key.data(), key.size(), EVP_sha256(), nullptr /* impl */) != 1) {
        LOG(ERROR) << "Error initializing HMAC_CTX";
        return {};
    }
    if (HMAC_Update(&ctx, data.data(), data.size()) != 1) {
        LOG(ERROR) << "Error updating HMAC_CTX";
        return {};
    }
    vector<uint8_t> hmac;
    hmac.resize(32);
    unsigned int size = 0;
    if (HMAC_Final(&ctx, hmac.data(), &size) != 1) {
        LOG(ERROR) << "Error finalizing HMAC_CTX";
        return {};
    }
    if (size != 32) {
        LOG(ERROR) << "Expected 32 bytes from HMAC_Final, got " << size;
        return {};
    }
    return hmac;
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

// Generates the attestation certificate with the parameters passed in.  Note
// that the passed in |activeTimeMilliSeconds| |expireTimeMilliSeconds| are in
// milli seconds since epoch.  We are setting them to milliseconds due to
// requirement in AuthorizationSet KM_DATE fields.  The certificate created is
// actually in seconds.
//
// If 0 is passed for expiration time, the expiration time from batch
// certificate will be used.
//
optional<vector<vector<uint8_t>>> createAttestation(
        const EVP_PKEY* key, const vector<uint8_t>& applicationId, const vector<uint8_t>& challenge,
        uint64_t activeTimeMilliSeconds, uint64_t expireTimeMilliSeconds, bool isTestCredential) {
    // Pretend to be implemented in a trusted environment just so we can pass
    // the VTS tests. Of course, this is a pretend-only game since hopefully no
    // relying party is ever going to trust our batch key and those keys above
    // it.
    ::keymaster::PureSoftKeymasterContext context(::keymaster::KmVersion::KEYMINT_1,
                                                  KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT);

    keymaster_error_t error;
    ::keymaster::CertificateChain attestation_chain =
            context.GetAttestationChain(KM_ALGORITHM_EC, &error);
    if (KM_ERROR_OK != error) {
        LOG(ERROR) << "Error getting attestation chain " << error;
        return {};
    }
    if (expireTimeMilliSeconds == 0) {
        if (attestation_chain.entry_count < 1) {
            LOG(ERROR) << "Expected at least one entry in attestation chain";
            return {};
        }
        keymaster_blob_t* bcBlob = &(attestation_chain.entries[0]);
        const uint8_t* bcData = bcBlob->data;
        auto bc = X509_Ptr(d2i_X509(nullptr, &bcData, bcBlob->data_length));
        time_t bcNotAfter;
        if (!parseAsn1Time(X509_get0_notAfter(bc.get()), &bcNotAfter)) {
            LOG(ERROR) << "Error getting notAfter from batch certificate";
            return {};
        }
        expireTimeMilliSeconds = bcNotAfter * 1000;
    }

    ::keymaster::X509_NAME_Ptr subjectName;
    if (KM_ERROR_OK !=
        ::keymaster::make_name_from_str("Android Identity Credential Key", &subjectName)) {
        LOG(ERROR) << "Cannot create attestation subject";
        return {};
    }

    vector<uint8_t> subject(i2d_X509_NAME(subjectName.get(), NULL));
    unsigned char* subjectPtr = subject.data();

    i2d_X509_NAME(subjectName.get(), &subjectPtr);

    ::keymaster::AuthorizationSet auth_set(
            ::keymaster::AuthorizationSetBuilder()
                    .Authorization(::keymaster::TAG_CERTIFICATE_NOT_BEFORE, activeTimeMilliSeconds)
                    .Authorization(::keymaster::TAG_CERTIFICATE_NOT_AFTER, expireTimeMilliSeconds)
                    .Authorization(::keymaster::TAG_ATTESTATION_CHALLENGE, challenge.data(),
                                   challenge.size())
                    .Authorization(::keymaster::TAG_ACTIVE_DATETIME, activeTimeMilliSeconds)
                    // Even though identity attestation hal said the application
                    // id should be in software enforced authentication set,
                    // keymaster portable lib expect the input in this
                    // parameter because the software enforced in input to keymaster
                    // refers to the key software enforced properties. And this
                    // parameter refers to properties of the attestation which
                    // includes app id.
                    .Authorization(::keymaster::TAG_ATTESTATION_APPLICATION_ID,
                                   applicationId.data(), applicationId.size())
                    .Authorization(::keymaster::TAG_CERTIFICATE_SUBJECT, subject.data(),
                                   subject.size())
                    .Authorization(::keymaster::TAG_USAGE_EXPIRE_DATETIME, expireTimeMilliSeconds));

    // Unique id and device id is not applicable for identity credential attestation,
    // so we don't need to set those or application id.
    ::keymaster::AuthorizationSet swEnforced(::keymaster::AuthorizationSetBuilder().Authorization(
            ::keymaster::TAG_CREATION_DATETIME, activeTimeMilliSeconds));

    ::keymaster::AuthorizationSetBuilder hwEnforcedBuilder =
            ::keymaster::AuthorizationSetBuilder()
                    .Authorization(::keymaster::TAG_PURPOSE, KM_PURPOSE_SIGN)
                    .Authorization(::keymaster::TAG_KEY_SIZE, 256)
                    .Authorization(::keymaster::TAG_ALGORITHM, KM_ALGORITHM_EC)
                    .Authorization(::keymaster::TAG_NO_AUTH_REQUIRED)
                    .Authorization(::keymaster::TAG_DIGEST, KM_DIGEST_SHA_2_256)
                    .Authorization(::keymaster::TAG_EC_CURVE, KM_EC_CURVE_P_256)
                    .Authorization(::keymaster::TAG_OS_VERSION, 42)
                    .Authorization(::keymaster::TAG_OS_PATCHLEVEL, 43);

    // Only include TAG_IDENTITY_CREDENTIAL_KEY if it's not a test credential
    if (!isTestCredential) {
        hwEnforcedBuilder.Authorization(::keymaster::TAG_IDENTITY_CREDENTIAL_KEY);
    }
    ::keymaster::AuthorizationSet hwEnforced(hwEnforcedBuilder);

    ::keymaster::CertificateChain cert_chain_out = generate_attestation(
            key, swEnforced, hwEnforced, auth_set, {} /* attest_key */, context, &error);

    if (KM_ERROR_OK != error) {
        LOG(ERROR) << "Error generating attestation from EVP key: " << error;
        return {};
    }

    // translate certificate format from keymaster_cert_chain_t to vector<vector<uint8_t>>.
    vector<vector<uint8_t>> attestationCertificate;
    for (std::size_t i = 0; i < cert_chain_out.entry_count; i++) {
        attestationCertificate.insert(
                attestationCertificate.end(),
                vector<uint8_t>(
                        cert_chain_out.entries[i].data,
                        cert_chain_out.entries[i].data + cert_chain_out.entries[i].data_length));
    }

    return attestationCertificate;
}

optional<std::pair<vector<uint8_t>, vector<vector<uint8_t>>>> createEcKeyPairAndAttestation(
        const vector<uint8_t>& challenge, const vector<uint8_t>& applicationId,
        bool isTestCredential) {
    auto ec_key = ::keymaster::EC_KEY_Ptr(EC_KEY_new());
    auto pkey = ::keymaster::EVP_PKEY_Ptr(EVP_PKEY_new());
    auto group = ::keymaster::EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));

    if (ec_key.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        LOG(ERROR) << "Error generating key";
        return {};
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1) {
        LOG(ERROR) << "Error getting private key";
        return {};
    }

    uint64_t nowMs = time(nullptr) * 1000;
    uint64_t expireTimeMs = 0;  // Set to same as batch certificate

    optional<vector<vector<uint8_t>>> attestationCert = createAttestation(
            pkey.get(), applicationId, challenge, nowMs, expireTimeMs, isTestCredential);
    if (!attestationCert) {
        LOG(ERROR) << "Error create attestation from key and challenge";
        return {};
    }

    int size = i2d_PrivateKey(pkey.get(), nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }

    vector<uint8_t> keyPair(size);
    unsigned char* p = keyPair.data();
    i2d_PrivateKey(pkey.get(), &p);

    return make_pair(keyPair, attestationCert.value());
}

optional<vector<vector<uint8_t>>> createAttestationForEcPublicKey(
        const vector<uint8_t>& publicKey, const vector<uint8_t>& challenge,
        const vector<uint8_t>& applicationId) {
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), publicKey.data(), publicKey.size(), nullptr) !=
        1) {
        LOG(ERROR) << "Error decoding publicKey";
        return {};
    }
    auto ecKey = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }
    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1) {
        LOG(ERROR) << "Error setting group";
        return {};
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        LOG(ERROR) << "Error setting point";
        return {};
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error setting key";
        return {};
    }

    uint64_t nowMs = time(nullptr) * 1000;
    uint64_t expireTimeMs = 0;  // Set to same as batch certificate

    optional<vector<vector<uint8_t>>> attestationCert =
            createAttestation(pkey.get(), applicationId, challenge, nowMs, expireTimeMs,
                              false /* isTestCredential */);
    if (!attestationCert) {
        LOG(ERROR) << "Error create attestation from key and challenge";
        return {};
    }

    return attestationCert.value();
}

optional<vector<uint8_t>> createEcKeyPair() {
    auto ec_key = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ec_key.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }

    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        LOG(ERROR) << "Error creating EC group by curve name";
        return {};
    }

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        LOG(ERROR) << "Error generating key";
        return {};
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1) {
        LOG(ERROR) << "Error getting private key";
        return {};
    }

    int size = i2d_PrivateKey(pkey.get(), nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }
    vector<uint8_t> keyPair;
    keyPair.resize(size);
    unsigned char* p = keyPair.data();
    i2d_PrivateKey(pkey.get(), &p);
    return keyPair;
}

optional<vector<uint8_t>> ecKeyPairGetPublicKey(const vector<uint8_t>& keyPair) {
    const unsigned char* p = (const unsigned char*)keyPair.data();
    auto pkey = EVP_PKEY_Ptr(d2i_PrivateKey(EVP_PKEY_EC, nullptr, &p, keyPair.size()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "Error parsing keyPair";
        return {};
    }

    auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (ecKey.get() == nullptr) {
        LOG(ERROR) << "Failed getting EC key";
        return {};
    }

    auto ecGroup = EC_KEY_get0_group(ecKey.get());
    auto ecPoint = EC_KEY_get0_public_key(ecKey.get());
    int size = EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0,
                                  nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }

    vector<uint8_t> publicKey;
    publicKey.resize(size);
    EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, publicKey.data(),
                       publicKey.size(), nullptr);
    return publicKey;
}

optional<vector<uint8_t>> ecKeyPairGetPrivateKey(const vector<uint8_t>& keyPair) {
    const unsigned char* p = (const unsigned char*)keyPair.data();
    auto pkey = EVP_PKEY_Ptr(d2i_PrivateKey(EVP_PKEY_EC, nullptr, &p, keyPair.size()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "Error parsing keyPair";
        return {};
    }

    auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (ecKey.get() == nullptr) {
        LOG(ERROR) << "Failed getting EC key";
        return {};
    }

    const BIGNUM* bignum = EC_KEY_get0_private_key(ecKey.get());
    if (bignum == nullptr) {
        LOG(ERROR) << "Error getting bignum from private key";
        return {};
    }
    vector<uint8_t> privateKey;

    // Note that this may return fewer than 32 bytes so pad with zeroes since we
    // want to always return 32 bytes.
    size_t numBytes = BN_num_bytes(bignum);
    if (numBytes > 32) {
        LOG(ERROR) << "Size is " << numBytes << ", expected this to be 32 or less";
        return {};
    }
    privateKey.resize(32);
    for (size_t n = 0; n < 32 - numBytes; n++) {
        privateKey[n] = 0x00;
    }
    BN_bn2bin(bignum, privateKey.data() + 32 - numBytes);
    return privateKey;
}

optional<vector<uint8_t>> ecPrivateKeyToKeyPair(const vector<uint8_t>& privateKey) {
    auto bn = BIGNUM_Ptr(BN_bin2bn(privateKey.data(), privateKey.size(), nullptr));
    if (bn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM";
        return {};
    }

    auto ecKey = EC_KEY_Ptr(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if (EC_KEY_set_private_key(ecKey.get(), bn.get()) != 1) {
        LOG(ERROR) << "Error setting private key from BIGNUM";
        return {};
    }

    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error getting private key";
        return {};
    }

    int size = i2d_PrivateKey(pkey.get(), nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }
    vector<uint8_t> keyPair;
    keyPair.resize(size);
    unsigned char* p = keyPair.data();
    i2d_PrivateKey(pkey.get(), &p);
    return keyPair;
}

optional<vector<uint8_t>> ecKeyPairGetPkcs12(const vector<uint8_t>& keyPair, const string& name,
                                             const string& serialDecimal, const string& issuer,
                                             const string& subject, time_t validityNotBefore,
                                             time_t validityNotAfter) {
    const unsigned char* p = (const unsigned char*)keyPair.data();
    auto pkey = EVP_PKEY_Ptr(d2i_PrivateKey(EVP_PKEY_EC, nullptr, &p, keyPair.size()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "Error parsing keyPair";
        return {};
    }

    auto x509 = X509_Ptr(X509_new());
    if (!x509.get()) {
        LOG(ERROR) << "Error creating X509 certificate";
        return {};
    }

    if (!X509_set_version(x509.get(), 2 /* version 3, but zero-based */)) {
        LOG(ERROR) << "Error setting version to 3";
        return {};
    }

    if (X509_set_pubkey(x509.get(), pkey.get()) != 1) {
        LOG(ERROR) << "Error setting public key";
        return {};
    }

    BIGNUM* bignumSerial = nullptr;
    if (BN_dec2bn(&bignumSerial, serialDecimal.c_str()) == 0) {
        LOG(ERROR) << "Error parsing serial";
        return {};
    }
    auto bignumSerialPtr = BIGNUM_Ptr(bignumSerial);
    auto asnSerial = ASN1_INTEGER_Ptr(BN_to_ASN1_INTEGER(bignumSerial, nullptr));
    if (X509_set_serialNumber(x509.get(), asnSerial.get()) != 1) {
        LOG(ERROR) << "Error setting serial";
        return {};
    }

    auto x509Issuer = X509_NAME_Ptr(X509_NAME_new());
    if (x509Issuer.get() == nullptr ||
        X509_NAME_add_entry_by_txt(x509Issuer.get(), "CN", MBSTRING_ASC,
                                   (const uint8_t*)issuer.c_str(), issuer.size(), -1 /* loc */,
                                   0 /* set */) != 1 ||
        X509_set_issuer_name(x509.get(), x509Issuer.get()) != 1) {
        LOG(ERROR) << "Error setting issuer";
        return {};
    }

    auto x509Subject = X509_NAME_Ptr(X509_NAME_new());
    if (x509Subject.get() == nullptr ||
        X509_NAME_add_entry_by_txt(x509Subject.get(), "CN", MBSTRING_ASC,
                                   (const uint8_t*)subject.c_str(), subject.size(), -1 /* loc */,
                                   0 /* set */) != 1 ||
        X509_set_subject_name(x509.get(), x509Subject.get()) != 1) {
        LOG(ERROR) << "Error setting subject";
        return {};
    }

    auto asnNotBefore = ASN1_TIME_Ptr(ASN1_TIME_set(nullptr, validityNotBefore));
    if (asnNotBefore.get() == nullptr || X509_set_notBefore(x509.get(), asnNotBefore.get()) != 1) {
        LOG(ERROR) << "Error setting notBefore";
        return {};
    }

    auto asnNotAfter = ASN1_TIME_Ptr(ASN1_TIME_set(nullptr, validityNotAfter));
    if (asnNotAfter.get() == nullptr || X509_set_notAfter(x509.get(), asnNotAfter.get()) != 1) {
        LOG(ERROR) << "Error setting notAfter";
        return {};
    }

    if (X509_sign(x509.get(), pkey.get(), EVP_sha256()) == 0) {
        LOG(ERROR) << "Error signing X509 certificate";
        return {};
    }

    // Ideally we wouldn't encrypt it (we're only using this function for
    // sending a key-pair over binder to the Android app) but BoringSSL does not
    // support this: from pkcs8_x509.c in BoringSSL: "In OpenSSL, -1 here means
    // to use no encryption, which we do not currently support."
    //
    // Passing nullptr as |pass|, though, means "no password". So we'll do that.
    // Compare with the receiving side - CredstoreIdentityCredential.java - where
    // an empty char[] is passed as the password.
    //
    auto pkcs12 = PKCS12_Ptr(PKCS12_create(nullptr, name.c_str(), pkey.get(), x509.get(),
                                           nullptr,  // ca
                                           0,        // nid_key
                                           0,        // nid_cert
                                           0,        // iter,
                                           0,        // mac_iter,
                                           0));      // keytype
    if (pkcs12.get() == nullptr) {
        char buf[128];
        long errCode = ERR_get_error();
        ERR_error_string_n(errCode, buf, sizeof buf);
        LOG(ERROR) << "Error creating PKCS12, code " << errCode << ": " << buf;
        return {};
    }

    unsigned char* buffer = nullptr;
    int length = i2d_PKCS12(pkcs12.get(), &buffer);
    if (length < 0) {
        LOG(ERROR) << "Error encoding PKCS12";
        return {};
    }
    vector<uint8_t> pkcs12Bytes;
    pkcs12Bytes.resize(length);
    memcpy(pkcs12Bytes.data(), buffer, length);
    OPENSSL_free(buffer);

    return pkcs12Bytes;
}

optional<vector<uint8_t>> ecPublicKeyGenerateCertificate(
        const vector<uint8_t>& publicKey, const vector<uint8_t>& signingKey,
        const string& serialDecimal, const string& issuer, const string& subject,
        time_t validityNotBefore, time_t validityNotAfter,
        const map<string, vector<uint8_t>>& extensions) {
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), publicKey.data(), publicKey.size(), nullptr) !=
        1) {
        LOG(ERROR) << "Error decoding publicKey";
        return {};
    }
    auto ecKey = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }
    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1) {
        LOG(ERROR) << "Error setting group";
        return {};
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        LOG(ERROR) << "Error setting point";
        return {};
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error setting key";
        return {};
    }

    auto bn = BIGNUM_Ptr(BN_bin2bn(signingKey.data(), signingKey.size(), nullptr));
    if (bn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM for private key";
        return {};
    }
    auto privEcKey = EC_KEY_Ptr(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if (EC_KEY_set_private_key(privEcKey.get(), bn.get()) != 1) {
        LOG(ERROR) << "Error setting private key from BIGNUM";
        return {};
    }
    auto privPkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (EVP_PKEY_set1_EC_KEY(privPkey.get(), privEcKey.get()) != 1) {
        LOG(ERROR) << "Error setting private key";
        return {};
    }

    auto x509 = X509_Ptr(X509_new());
    if (!x509.get()) {
        LOG(ERROR) << "Error creating X509 certificate";
        return {};
    }

    if (!X509_set_version(x509.get(), 2 /* version 3, but zero-based */)) {
        LOG(ERROR) << "Error setting version to 3";
        return {};
    }

    if (X509_set_pubkey(x509.get(), pkey.get()) != 1) {
        LOG(ERROR) << "Error setting public key";
        return {};
    }

    BIGNUM* bignumSerial = nullptr;
    if (BN_dec2bn(&bignumSerial, serialDecimal.c_str()) == 0) {
        LOG(ERROR) << "Error parsing serial";
        return {};
    }
    auto bignumSerialPtr = BIGNUM_Ptr(bignumSerial);
    auto asnSerial = ASN1_INTEGER_Ptr(BN_to_ASN1_INTEGER(bignumSerial, nullptr));
    if (X509_set_serialNumber(x509.get(), asnSerial.get()) != 1) {
        LOG(ERROR) << "Error setting serial";
        return {};
    }

    auto x509Issuer = X509_NAME_Ptr(X509_NAME_new());
    if (x509Issuer.get() == nullptr ||
        X509_NAME_add_entry_by_txt(x509Issuer.get(), "CN", MBSTRING_ASC,
                                   (const uint8_t*)issuer.c_str(), issuer.size(), -1 /* loc */,
                                   0 /* set */) != 1 ||
        X509_set_issuer_name(x509.get(), x509Issuer.get()) != 1) {
        LOG(ERROR) << "Error setting issuer";
        return {};
    }

    auto x509Subject = X509_NAME_Ptr(X509_NAME_new());
    if (x509Subject.get() == nullptr ||
        X509_NAME_add_entry_by_txt(x509Subject.get(), "CN", MBSTRING_ASC,
                                   (const uint8_t*)subject.c_str(), subject.size(), -1 /* loc */,
                                   0 /* set */) != 1 ||
        X509_set_subject_name(x509.get(), x509Subject.get()) != 1) {
        LOG(ERROR) << "Error setting subject";
        return {};
    }

    auto asnNotBefore = ASN1_TIME_Ptr(ASN1_TIME_set(nullptr, validityNotBefore));
    if (asnNotBefore.get() == nullptr || X509_set_notBefore(x509.get(), asnNotBefore.get()) != 1) {
        LOG(ERROR) << "Error setting notBefore";
        return {};
    }

    auto asnNotAfter = ASN1_TIME_Ptr(ASN1_TIME_set(nullptr, validityNotAfter));
    if (asnNotAfter.get() == nullptr || X509_set_notAfter(x509.get(), asnNotAfter.get()) != 1) {
        LOG(ERROR) << "Error setting notAfter";
        return {};
    }

    for (auto const& [oidStr, blob] : extensions) {
        ASN1_OBJECT_Ptr oid(
                OBJ_txt2obj(oidStr.c_str(), 1));  // accept numerical dotted string form only
        if (!oid.get()) {
            LOG(ERROR) << "Error setting OID";
            return {};
        }
        ASN1_OCTET_STRING_Ptr octetString(ASN1_OCTET_STRING_new());
        if (!ASN1_OCTET_STRING_set(octetString.get(), blob.data(), blob.size())) {
            LOG(ERROR) << "Error setting octet string for extension";
            return {};
        }

        X509_EXTENSION_Ptr extension = X509_EXTENSION_Ptr(X509_EXTENSION_new());
        extension.reset(X509_EXTENSION_create_by_OBJ(nullptr, oid.get(), 0 /* not critical */,
                                                     octetString.get()));
        if (!extension.get()) {
            LOG(ERROR) << "Error setting extension";
            return {};
        }
        if (!X509_add_ext(x509.get(), extension.get(), -1)) {
            LOG(ERROR) << "Error adding extension";
            return {};
        }
    }

    if (X509_sign(x509.get(), privPkey.get(), EVP_sha256()) == 0) {
        LOG(ERROR) << "Error signing X509 certificate";
        return {};
    }

    unsigned char* buffer = nullptr;
    int length = i2d_X509(x509.get(), &buffer);
    if (length < 0) {
        LOG(ERROR) << "Error DER encoding X509 certificate";
        return {};
    }

    vector<uint8_t> certificate;
    certificate.resize(length);
    memcpy(certificate.data(), buffer, length);
    OPENSSL_free(buffer);
    return certificate;
}

optional<vector<uint8_t>> ecdh(const vector<uint8_t>& publicKey,
                               const vector<uint8_t>& privateKey) {
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), publicKey.data(), publicKey.size(), nullptr) !=
        1) {
        LOG(ERROR) << "Error decoding publicKey";
        return {};
    }
    auto ecKey = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return {};
    }
    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1) {
        LOG(ERROR) << "Error setting group";
        return {};
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        LOG(ERROR) << "Error setting point";
        return {};
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "Error setting key";
        return {};
    }

    auto bn = BIGNUM_Ptr(BN_bin2bn(privateKey.data(), privateKey.size(), nullptr));
    if (bn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM for private key";
        return {};
    }
    auto privEcKey = EC_KEY_Ptr(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if (EC_KEY_set_private_key(privEcKey.get(), bn.get()) != 1) {
        LOG(ERROR) << "Error setting private key from BIGNUM";
        return {};
    }
    auto privPkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    if (EVP_PKEY_set1_EC_KEY(privPkey.get(), privEcKey.get()) != 1) {
        LOG(ERROR) << "Error setting private key";
        return {};
    }

    auto ctx = EVP_PKEY_CTX_Ptr(EVP_PKEY_CTX_new(privPkey.get(), NULL));
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "Error creating context";
        return {};
    }

    if (EVP_PKEY_derive_init(ctx.get()) != 1) {
        LOG(ERROR) << "Error initializing context";
        return {};
    }

    if (EVP_PKEY_derive_set_peer(ctx.get(), pkey.get()) != 1) {
        LOG(ERROR) << "Error setting peer";
        return {};
    }

    /* Determine buffer length for shared secret */
    size_t secretLen = 0;
    if (EVP_PKEY_derive(ctx.get(), NULL, &secretLen) != 1) {
        LOG(ERROR) << "Error determing length of shared secret";
        return {};
    }
    vector<uint8_t> sharedSecret;
    sharedSecret.resize(secretLen);

    if (EVP_PKEY_derive(ctx.get(), sharedSecret.data(), &secretLen) != 1) {
        LOG(ERROR) << "Error deriving shared secret";
        return {};
    }
    return sharedSecret;
}

optional<vector<uint8_t>> hkdf(const vector<uint8_t>& sharedSecret, const vector<uint8_t>& salt,
                               const vector<uint8_t>& info, size_t size) {
    vector<uint8_t> derivedKey;
    derivedKey.resize(size);
    if (HKDF(derivedKey.data(), derivedKey.size(), EVP_sha256(), sharedSecret.data(),
             sharedSecret.size(), salt.data(), salt.size(), info.data(), info.size()) != 1) {
        LOG(ERROR) << "Error deriving key";
        return {};
    }
    return derivedKey;
}

void removeLeadingZeroes(vector<uint8_t>& vec) {
    while (vec.size() >= 1 && vec[0] == 0x00) {
        vec.erase(vec.begin());
    }
}

tuple<bool, vector<uint8_t>, vector<uint8_t>> ecPublicKeyGetXandY(
        const vector<uint8_t>& publicKey) {
    if (publicKey.size() != 65 || publicKey[0] != 0x04) {
        LOG(ERROR) << "publicKey is not in the expected format";
        return std::make_tuple(false, vector<uint8_t>(), vector<uint8_t>());
    }
    vector<uint8_t> x, y;
    x.resize(32);
    y.resize(32);
    memcpy(x.data(), publicKey.data() + 1, 32);
    memcpy(y.data(), publicKey.data() + 33, 32);

    removeLeadingZeroes(x);
    removeLeadingZeroes(y);

    return std::make_tuple(true, x, y);
}

optional<vector<uint8_t>> certificateChainGetTopMostKey(const vector<uint8_t>& certificateChain) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(certificateChain, certs)) {
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    auto pkey = EVP_PKEY_Ptr(X509_get_pubkey(certs[0].get()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "No public key";
        return {};
    }

    auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (ecKey.get() == nullptr) {
        LOG(ERROR) << "Failed getting EC key";
        return {};
    }

    auto ecGroup = EC_KEY_get0_group(ecKey.get());
    auto ecPoint = EC_KEY_get0_public_key(ecKey.get());
    int size = EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0,
                                  nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }
    vector<uint8_t> publicKey;
    publicKey.resize(size);
    EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, publicKey.data(),
                       publicKey.size(), nullptr);
    return publicKey;
}

optional<vector<uint8_t>> certificateGetExtension(const vector<uint8_t>& x509Certificate,
                                                  const string& oidStr) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(x509Certificate, certs)) {
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    ASN1_OBJECT_Ptr oid(
            OBJ_txt2obj(oidStr.c_str(), 1));  // accept numerical dotted string form only
    if (!oid.get()) {
        LOG(ERROR) << "Error setting OID";
        return {};
    }

    int location = X509_get_ext_by_OBJ(certs[0].get(), oid.get(), -1 /* search from beginning */);
    if (location == -1) {
        return {};
    }

    X509_EXTENSION* ext = X509_get_ext(certs[0].get(), location);
    if (ext == nullptr) {
        return {};
    }

    ASN1_OCTET_STRING* octetString = X509_EXTENSION_get_data(ext);
    if (octetString == nullptr) {
        return {};
    }
    vector<uint8_t> result;
    result.resize(octetString->length);
    memcpy(result.data(), octetString->data, octetString->length);
    return result;
}

optional<pair<size_t, size_t>> certificateFindPublicKey(const vector<uint8_t>& x509Certificate) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(x509Certificate, certs)) {
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    auto pkey = EVP_PKEY_Ptr(X509_get_pubkey(certs[0].get()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "No public key";
        return {};
    }

    auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (ecKey.get() == nullptr) {
        LOG(ERROR) << "Failed getting EC key";
        return {};
    }

    auto ecGroup = EC_KEY_get0_group(ecKey.get());
    auto ecPoint = EC_KEY_get0_public_key(ecKey.get());
    int size = EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0,
                                  nullptr);
    if (size == 0) {
        LOG(ERROR) << "Error generating public key encoding";
        return {};
    }
    vector<uint8_t> publicKey;
    publicKey.resize(size);
    EC_POINT_point2oct(ecGroup, ecPoint, POINT_CONVERSION_UNCOMPRESSED, publicKey.data(),
                       publicKey.size(), nullptr);

    size_t publicKeyOffset = 0;
    size_t publicKeySize = (size_t)size;
    void* location = memmem((const void*)x509Certificate.data(), x509Certificate.size(),
                            (const void*)publicKey.data(), publicKey.size());

    if (location == NULL) {
        LOG(ERROR) << "Error finding publicKey from x509Certificate";
        return {};
    }
    publicKeyOffset = (size_t)((const char*)location - (const char*)x509Certificate.data());

    return std::make_pair(publicKeyOffset, publicKeySize);
}

optional<pair<size_t, size_t>> certificateTbsCertificate(const vector<uint8_t>& x509Certificate) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(x509Certificate, certs)) {
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    unsigned char* buf = NULL;
    int len = i2d_re_X509_tbs(certs[0].get(), &buf);
    if ((len < 0) || (buf == NULL)) {
        LOG(ERROR) << "fail to extract tbsCertificate in x509Certificate";
        return {};
    }

    vector<uint8_t> tbsCertificate(len);
    memcpy(tbsCertificate.data(), buf, len);

    size_t tbsCertificateOffset = 0;
    size_t tbsCertificateSize = (size_t)len;
    void* location = memmem((const void*)x509Certificate.data(), x509Certificate.size(),
                            (const void*)tbsCertificate.data(), tbsCertificate.size());

    if (location == NULL) {
        LOG(ERROR) << "Error finding tbsCertificate from x509Certificate";
        return {};
    }
    tbsCertificateOffset = (size_t)((const char*)location - (const char*)x509Certificate.data());

    return std::make_pair(tbsCertificateOffset, tbsCertificateSize);
}

optional<pair<time_t, time_t>> certificateGetValidity(const vector<uint8_t>& x509Certificate) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(x509Certificate, certs)) {
        LOG(ERROR) << "Error parsing certificates";
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    time_t notBefore;
    time_t notAfter;
    if (!parseAsn1Time(X509_get0_notBefore(certs[0].get()), &notBefore)) {
        LOG(ERROR) << "Error parsing notBefore";
        return {};
    }

    if (!parseAsn1Time(X509_get0_notAfter(certs[0].get()), &notAfter)) {
        LOG(ERROR) << "Error parsing notAfter";
        return {};
    }

    return std::make_pair(notBefore, notAfter);
}

optional<pair<size_t, size_t>> certificateFindSignature(const vector<uint8_t>& x509Certificate) {
    vector<X509_Ptr> certs;
    if (!parseX509Certificates(x509Certificate, certs)) {
        return {};
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return {};
    }

    ASN1_BIT_STRING* psig;
    X509_ALGOR* palg;
    X509_get0_signature((const ASN1_BIT_STRING**)&psig, (const X509_ALGOR**)&palg, certs[0].get());

    vector<char> signature(psig->length);
    memcpy(signature.data(), psig->data, psig->length);

    size_t signatureOffset = 0;
    size_t signatureSize = (size_t)psig->length;
    void* location = memmem((const void*)x509Certificate.data(), x509Certificate.size(),
                            (const void*)signature.data(), signature.size());

    if (location == NULL) {
        LOG(ERROR) << "Error finding signature from x509Certificate";
        return {};
    }
    signatureOffset = (size_t)((const char*)location - (const char*)x509Certificate.data());

    return std::make_pair(signatureOffset, signatureSize);
}

// ---------------------------------------------------------------------------
// COSE Utility Functions
// ---------------------------------------------------------------------------

vector<uint8_t> coseBuildToBeSigned(const vector<uint8_t>& encodedProtectedHeaders,
                                    const vector<uint8_t>& data,
                                    const vector<uint8_t>& detachedContent) {
    cppbor::Array sigStructure;
    sigStructure.add("Signature1");
    sigStructure.add(encodedProtectedHeaders);

    // We currently don't support Externally Supplied Data (RFC 8152 section 4.3)
    // so external_aad is the empty bstr
    vector<uint8_t> emptyExternalAad;
    sigStructure.add(emptyExternalAad);

    // Next field is the payload, independently of how it's transported (RFC
    // 8152 section 4.4). Since our API specifies only one of |data| and
    // |detachedContent| can be non-empty, it's simply just the non-empty one.
    if (data.size() > 0) {
        sigStructure.add(data);
    } else {
        sigStructure.add(detachedContent);
    }
    return sigStructure.encode();
}

vector<uint8_t> coseEncodeHeaders(const cppbor::Map& protectedHeaders) {
    if (protectedHeaders.size() == 0) {
        cppbor::Bstr emptyBstr(vector<uint8_t>({}));
        return emptyBstr.encode();
    }
    return protectedHeaders.encode();
}

// From https://tools.ietf.org/html/rfc8152
const int COSE_LABEL_ALG = 1;
const int COSE_LABEL_X5CHAIN = 33;  // temporary identifier

// From "COSE Algorithms" registry
const int COSE_ALG_ECDSA_256 = -7;
const int COSE_ALG_HMAC_256_256 = 5;

bool ecdsaSignatureCoseToDer(const vector<uint8_t>& ecdsaCoseSignature,
                             vector<uint8_t>& ecdsaDerSignature) {
    if (ecdsaCoseSignature.size() != 64) {
        LOG(ERROR) << "COSE signature length is " << ecdsaCoseSignature.size() << ", expected 64";
        return false;
    }

    auto rBn = BIGNUM_Ptr(BN_bin2bn(ecdsaCoseSignature.data(), 32, nullptr));
    if (rBn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM for r";
        return false;
    }

    auto sBn = BIGNUM_Ptr(BN_bin2bn(ecdsaCoseSignature.data() + 32, 32, nullptr));
    if (sBn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM for s";
        return false;
    }

    ECDSA_SIG sig;
    sig.r = rBn.get();
    sig.s = sBn.get();

    size_t len = i2d_ECDSA_SIG(&sig, nullptr);
    ecdsaDerSignature.resize(len);
    unsigned char* p = (unsigned char*)ecdsaDerSignature.data();
    i2d_ECDSA_SIG(&sig, &p);

    return true;
}

bool ecdsaSignatureDerToCose(const vector<uint8_t>& ecdsaDerSignature,
                             vector<uint8_t>& ecdsaCoseSignature) {
    ECDSA_SIG* sig;
    const unsigned char* p = ecdsaDerSignature.data();
    sig = d2i_ECDSA_SIG(nullptr, &p, ecdsaDerSignature.size());
    if (sig == nullptr) {
        LOG(ERROR) << "Error decoding DER signature";
        return false;
    }

    ecdsaCoseSignature.clear();
    ecdsaCoseSignature.resize(64);
    if (BN_bn2binpad(ECDSA_SIG_get0_r(sig), ecdsaCoseSignature.data(), 32) != 32) {
        LOG(ERROR) << "Error encoding r";
        return false;
    }
    if (BN_bn2binpad(ECDSA_SIG_get0_s(sig), ecdsaCoseSignature.data() + 32, 32) != 32) {
        LOG(ERROR) << "Error encoding s";
        return false;
    }
    return true;
}

optional<vector<uint8_t>> coseSignEcDsaWithSignature(const vector<uint8_t>& signatureToBeSigned,
                                                     const vector<uint8_t>& data,
                                                     const vector<uint8_t>& certificateChain) {
    if (signatureToBeSigned.size() != 64) {
        LOG(ERROR) << "Invalid size for signatureToBeSigned, expected 64 got "
                   << signatureToBeSigned.size();
        return {};
    }

    cppbor::Map unprotectedHeaders;
    cppbor::Map protectedHeaders;

    protectedHeaders.add(COSE_LABEL_ALG, COSE_ALG_ECDSA_256);

    if (certificateChain.size() != 0) {
        optional<vector<vector<uint8_t>>> certs = support::certificateChainSplit(certificateChain);
        if (!certs) {
            LOG(ERROR) << "Error splitting certificate chain";
            return {};
        }
        if (certs.value().size() == 1) {
            unprotectedHeaders.add(COSE_LABEL_X5CHAIN, certs.value()[0]);
        } else {
            cppbor::Array certArray;
            for (const vector<uint8_t>& cert : certs.value()) {
                certArray.add(cert);
            }
            unprotectedHeaders.add(COSE_LABEL_X5CHAIN, std::move(certArray));
        }
    }

    vector<uint8_t> encodedProtectedHeaders = coseEncodeHeaders(protectedHeaders);

    cppbor::Array coseSign1;
    coseSign1.add(encodedProtectedHeaders);
    coseSign1.add(std::move(unprotectedHeaders));
    if (data.size() == 0) {
        cppbor::Null nullValue;
        coseSign1.add(std::move(nullValue));
    } else {
        coseSign1.add(data);
    }
    coseSign1.add(signatureToBeSigned);
    vector<uint8_t> signatureCoseSign1;
    signatureCoseSign1 = coseSign1.encode();

    return signatureCoseSign1;
}

optional<vector<uint8_t>> coseSignEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data,
                                        const vector<uint8_t>& detachedContent,
                                        const vector<uint8_t>& certificateChain) {
    cppbor::Map unprotectedHeaders;
    cppbor::Map protectedHeaders;

    if (data.size() > 0 && detachedContent.size() > 0) {
        LOG(ERROR) << "data and detachedContent cannot both be non-empty";
        return {};
    }

    protectedHeaders.add(COSE_LABEL_ALG, COSE_ALG_ECDSA_256);

    if (certificateChain.size() != 0) {
        optional<vector<vector<uint8_t>>> certs = support::certificateChainSplit(certificateChain);
        if (!certs) {
            LOG(ERROR) << "Error splitting certificate chain";
            return {};
        }
        if (certs.value().size() == 1) {
            unprotectedHeaders.add(COSE_LABEL_X5CHAIN, certs.value()[0]);
        } else {
            cppbor::Array certArray;
            for (const vector<uint8_t>& cert : certs.value()) {
                certArray.add(cert);
            }
            unprotectedHeaders.add(COSE_LABEL_X5CHAIN, std::move(certArray));
        }
    }

    vector<uint8_t> encodedProtectedHeaders = coseEncodeHeaders(protectedHeaders);
    vector<uint8_t> toBeSigned =
            coseBuildToBeSigned(encodedProtectedHeaders, data, detachedContent);

    optional<vector<uint8_t>> derSignature = signEcDsa(key, toBeSigned);
    if (!derSignature) {
        LOG(ERROR) << "Error signing toBeSigned data";
        return {};
    }
    vector<uint8_t> coseSignature;
    if (!ecdsaSignatureDerToCose(derSignature.value(), coseSignature)) {
        LOG(ERROR) << "Error converting ECDSA signature from DER to COSE format";
        return {};
    }

    cppbor::Array coseSign1;
    coseSign1.add(encodedProtectedHeaders);
    coseSign1.add(std::move(unprotectedHeaders));
    if (data.size() == 0) {
        cppbor::Null nullValue;
        coseSign1.add(std::move(nullValue));
    } else {
        coseSign1.add(data);
    }
    coseSign1.add(coseSignature);
    vector<uint8_t> signatureCoseSign1;
    signatureCoseSign1 = coseSign1.encode();
    return signatureCoseSign1;
}

bool coseCheckEcDsaSignature(const vector<uint8_t>& signatureCoseSign1,
                             const vector<uint8_t>& detachedContent,
                             const vector<uint8_t>& publicKey) {
    auto [item, _, message] = cppbor::parse(signatureCoseSign1);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in COSE_Sign1 is not valid CBOR: " << message;
        return false;
    }
    const cppbor::Array* array = item->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array";
        return false;
    }
    if (array->size() != 4) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array of size 4";
        return false;
    }

    const cppbor::Bstr* encodedProtectedHeadersBstr = (*array)[0]->asBstr();
    ;
    if (encodedProtectedHeadersBstr == nullptr) {
        LOG(ERROR) << "Value for encodedProtectedHeaders is not a bstr";
        return false;
    }
    const vector<uint8_t> encodedProtectedHeaders = encodedProtectedHeadersBstr->value();

    const cppbor::Map* unprotectedHeaders = (*array)[1]->asMap();
    if (unprotectedHeaders == nullptr) {
        LOG(ERROR) << "Value for unprotectedHeaders is not a map";
        return false;
    }

    vector<uint8_t> data;
    const cppbor::Simple* payloadAsSimple = (*array)[2]->asSimple();
    if (payloadAsSimple != nullptr) {
        if (payloadAsSimple->asNull() == nullptr) {
            LOG(ERROR) << "Value for payload is not null or a bstr";
            return false;
        }
    } else {
        const cppbor::Bstr* payloadAsBstr = (*array)[2]->asBstr();
        if (payloadAsBstr == nullptr) {
            LOG(ERROR) << "Value for payload is not null or a bstr";
            return false;
        }
        data = payloadAsBstr->value();  // TODO: avoid copy
    }

    if (data.size() > 0 && detachedContent.size() > 0) {
        LOG(ERROR) << "data and detachedContent cannot both be non-empty";
        return false;
    }

    const cppbor::Bstr* signatureBstr = (*array)[3]->asBstr();
    if (signatureBstr == nullptr) {
        LOG(ERROR) << "Value for signature is a bstr";
        return false;
    }
    const vector<uint8_t>& coseSignature = signatureBstr->value();

    vector<uint8_t> derSignature;
    if (!ecdsaSignatureCoseToDer(coseSignature, derSignature)) {
        LOG(ERROR) << "Error converting ECDSA signature from COSE to DER format";
        return false;
    }

    vector<uint8_t> toBeSigned =
            coseBuildToBeSigned(encodedProtectedHeaders, data, detachedContent);
    if (!checkEcDsaSignature(support::sha256(toBeSigned), derSignature, publicKey)) {
        LOG(ERROR) << "Signature check failed";
        return false;
    }
    return true;
}

// Extracts the signature (of the ToBeSigned CBOR) from a COSE_Sign1.
optional<vector<uint8_t>> coseSignGetSignature(const vector<uint8_t>& signatureCoseSign1) {
    auto [item, _, message] = cppbor::parse(signatureCoseSign1);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in COSE_Sign1 is not valid CBOR: " << message;
        return {};
    }
    const cppbor::Array* array = item->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array";
        return {};
    }
    if (array->size() != 4) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array of size 4";
        return {};
    }

    vector<uint8_t> signature;
    const cppbor::Bstr* signatureAsBstr = (*array)[3]->asBstr();
    if (signatureAsBstr == nullptr) {
        LOG(ERROR) << "Value for signature is not a bstr";
        return {};
    }
    // Copy payload into |data|
    signature = signatureAsBstr->value();

    return signature;
}

optional<vector<uint8_t>> coseSignGetPayload(const vector<uint8_t>& signatureCoseSign1) {
    auto [item, _, message] = cppbor::parse(signatureCoseSign1);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in COSE_Sign1 is not valid CBOR: " << message;
        return {};
    }
    const cppbor::Array* array = item->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array";
        return {};
    }
    if (array->size() != 4) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array of size 4";
        return {};
    }

    vector<uint8_t> data;
    const cppbor::Simple* payloadAsSimple = (*array)[2]->asSimple();
    if (payloadAsSimple != nullptr) {
        if (payloadAsSimple->asNull() == nullptr) {
            LOG(ERROR) << "Value for payload is not null or a bstr";
            return {};
        }
        // payload is null, so |data| should be empty (as it is)
    } else {
        const cppbor::Bstr* payloadAsBstr = (*array)[2]->asBstr();
        if (payloadAsBstr == nullptr) {
            LOG(ERROR) << "Value for payload is not null or a bstr";
            return {};
        }
        // Copy payload into |data|
        data = payloadAsBstr->value();
    }

    return data;
}

optional<int> coseSignGetAlg(const vector<uint8_t>& signatureCoseSign1) {
    auto [item, _, message] = cppbor::parse(signatureCoseSign1);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in COSE_Sign1 is not valid CBOR: " << message;
        return {};
    }
    const cppbor::Array* array = item->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array";
        return {};
    }
    if (array->size() != 4) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array of size 4";
        return {};
    }

    const cppbor::Bstr* protectedHeadersBytes = (*array)[0]->asBstr();
    if (protectedHeadersBytes == nullptr) {
        LOG(ERROR) << "Value for protectedHeaders is not a bstr";
        return {};
    }
    auto [item2, _2, message2] = cppbor::parse(protectedHeadersBytes->value());
    if (item2 == nullptr) {
        LOG(ERROR) << "Error parsing protectedHeaders: " << message2;
        return {};
    }
    const cppbor::Map* protectedHeaders = item2->asMap();
    if (protectedHeaders == nullptr) {
        LOG(ERROR) << "Decoded CBOR for protectedHeaders is not a map";
        return {};
    }

    for (size_t n = 0; n < protectedHeaders->size(); n++) {
        auto& [keyItem, valueItem] = (*protectedHeaders)[n];
        const cppbor::Int* number = keyItem->asInt();
        if (number == nullptr) {
            LOG(ERROR) << "Key item in top-level map is not a number";
            return {};
        }
        int label = number->value();
        if (label == COSE_LABEL_ALG) {
            const cppbor::Int* number = valueItem->asInt();
            if (number != nullptr) {
                return number->value();
            }
            LOG(ERROR) << "Value for COSE_LABEL_ALG label is not a number";
            return {};
        }
    }
    LOG(ERROR) << "Did not find COSE_LABEL_ALG label in protected headers";
    return {};
}

optional<vector<uint8_t>> coseSignGetX5Chain(const vector<uint8_t>& signatureCoseSign1) {
    auto [item, _, message] = cppbor::parse(signatureCoseSign1);
    if (item == nullptr) {
        LOG(ERROR) << "Passed-in COSE_Sign1 is not valid CBOR: " << message;
        return {};
    }
    const cppbor::Array* array = item->asArray();
    if (array == nullptr) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array";
        return {};
    }
    if (array->size() != 4) {
        LOG(ERROR) << "Value for COSE_Sign1 is not an array of size 4";
        return {};
    }

    const cppbor::Map* unprotectedHeaders = (*array)[1]->asMap();
    if (unprotectedHeaders == nullptr) {
        LOG(ERROR) << "Value for unprotectedHeaders is not a map";
        return {};
    }

    for (size_t n = 0; n < unprotectedHeaders->size(); n++) {
        auto& [keyItem, valueItem] = (*unprotectedHeaders)[n];
        const cppbor::Int* number = keyItem->asInt();
        if (number == nullptr) {
            LOG(ERROR) << "Key item in top-level map is not a number";
            return {};
        }
        int label = number->value();
        if (label == COSE_LABEL_X5CHAIN) {
            const cppbor::Bstr* bstr = valueItem->asBstr();
            if (bstr != nullptr) {
                return bstr->value();
            }
            const cppbor::Array* array = valueItem->asArray();
            if (array != nullptr) {
                vector<uint8_t> certs;
                for (size_t m = 0; m < array->size(); m++) {
                    const cppbor::Bstr* bstr = ((*array)[m])->asBstr();
                    if (bstr == nullptr) {
                        LOG(ERROR) << "Item in x5chain array is not a bstr";
                        return {};
                    }
                    const vector<uint8_t>& certValue = bstr->value();
                    certs.insert(certs.end(), certValue.begin(), certValue.end());
                }
                return certs;
            }
            LOG(ERROR) << "Value for x5chain label is not a bstr or array";
            return {};
        }
    }
    LOG(ERROR) << "Did not find x5chain label in unprotected headers";
    return {};
}

vector<uint8_t> coseBuildToBeMACed(const vector<uint8_t>& encodedProtectedHeaders,
                                   const vector<uint8_t>& data,
                                   const vector<uint8_t>& detachedContent) {
    cppbor::Array macStructure;
    macStructure.add("MAC0");
    macStructure.add(encodedProtectedHeaders);

    // We currently don't support Externally Supplied Data (RFC 8152 section 4.3)
    // so external_aad is the empty bstr
    vector<uint8_t> emptyExternalAad;
    macStructure.add(emptyExternalAad);

    // Next field is the payload, independently of how it's transported (RFC
    // 8152 section 4.4). Since our API specifies only one of |data| and
    // |detachedContent| can be non-empty, it's simply just the non-empty one.
    if (data.size() > 0) {
        macStructure.add(data);
    } else {
        macStructure.add(detachedContent);
    }

    return macStructure.encode();
}

optional<vector<uint8_t>> coseMac0(const vector<uint8_t>& key, const vector<uint8_t>& data,
                                   const vector<uint8_t>& detachedContent) {
    cppbor::Map unprotectedHeaders;
    cppbor::Map protectedHeaders;

    if (data.size() > 0 && detachedContent.size() > 0) {
        LOG(ERROR) << "data and detachedContent cannot both be non-empty";
        return {};
    }

    protectedHeaders.add(COSE_LABEL_ALG, COSE_ALG_HMAC_256_256);

    vector<uint8_t> encodedProtectedHeaders = coseEncodeHeaders(protectedHeaders);
    vector<uint8_t> toBeMACed = coseBuildToBeMACed(encodedProtectedHeaders, data, detachedContent);

    optional<vector<uint8_t>> mac = hmacSha256(key, toBeMACed);
    if (!mac) {
        LOG(ERROR) << "Error MACing toBeMACed data";
        return {};
    }

    cppbor::Array array;
    array.add(encodedProtectedHeaders);
    array.add(std::move(unprotectedHeaders));
    if (data.size() == 0) {
        cppbor::Null nullValue;
        array.add(std::move(nullValue));
    } else {
        array.add(data);
    }
    array.add(mac.value());
    return array.encode();
}

optional<vector<uint8_t>> coseMacWithDigest(const vector<uint8_t>& digestToBeMaced,
                                            const vector<uint8_t>& data) {
    cppbor::Map unprotectedHeaders;
    cppbor::Map protectedHeaders;

    protectedHeaders.add(COSE_LABEL_ALG, COSE_ALG_HMAC_256_256);

    vector<uint8_t> encodedProtectedHeaders = coseEncodeHeaders(protectedHeaders);

    cppbor::Array array;
    array.add(encodedProtectedHeaders);
    array.add(std::move(unprotectedHeaders));
    if (data.size() == 0) {
        cppbor::Null nullValue;
        array.add(std::move(nullValue));
    } else {
        array.add(data);
    }
    array.add(digestToBeMaced);
    return array.encode();
}

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

optional<vector<uint8_t>> calcEMacKey(const vector<uint8_t>& privateKey,
                                      const vector<uint8_t>& publicKey,
                                      const vector<uint8_t>& sessionTranscriptBytes) {
    optional<vector<uint8_t>> sharedSecret = support::ecdh(publicKey, privateKey);
    if (!sharedSecret) {
        LOG(ERROR) << "Error performing ECDH";
        return {};
    }
    vector<uint8_t> salt = support::sha256(sessionTranscriptBytes);
    vector<uint8_t> info = {'E', 'M', 'a', 'c', 'K', 'e', 'y'};
    optional<vector<uint8_t>> derivedKey = support::hkdf(sharedSecret.value(), salt, info, 32);
    if (!derivedKey) {
        LOG(ERROR) << "Error performing HKDF";
        return {};
    }
    return derivedKey.value();
}

optional<vector<uint8_t>> calcMac(const vector<uint8_t>& sessionTranscriptEncoded,
                                  const string& docType,
                                  const vector<uint8_t>& deviceNameSpacesEncoded,
                                  const vector<uint8_t>& eMacKey) {
    auto [sessionTranscriptItem, _, errMsg] = cppbor::parse(sessionTranscriptEncoded);
    if (sessionTranscriptItem == nullptr) {
        LOG(ERROR) << "Error parsing sessionTranscriptEncoded: " << errMsg;
        return {};
    }
    // The data that is MACed is ["DeviceAuthentication", sessionTranscript, docType,
    // deviceNameSpacesBytes] so build up that structure
    cppbor::Array deviceAuthentication =
            cppbor::Array()
                    .add("DeviceAuthentication")
                    .add(std::move(sessionTranscriptItem))
                    .add(docType)
                    .add(cppbor::SemanticTag(kSemanticTagEncodedCbor, deviceNameSpacesEncoded));
    vector<uint8_t> deviceAuthenticationBytes =
            cppbor::SemanticTag(kSemanticTagEncodedCbor, deviceAuthentication.encode()).encode();
    optional<vector<uint8_t>> calculatedMac =
            support::coseMac0(eMacKey, {},                 // payload
                              deviceAuthenticationBytes);  // detached content
    return calculatedMac;
}

vector<vector<uint8_t>> chunkVector(const vector<uint8_t>& content, size_t maxChunkSize) {
    vector<vector<uint8_t>> ret;

    size_t contentSize = content.size();
    if (contentSize <= maxChunkSize) {
        ret.push_back(content);
        return ret;
    }

    size_t numChunks = (contentSize + maxChunkSize - 1) / maxChunkSize;

    size_t pos = 0;
    for (size_t n = 0; n < numChunks; n++) {
        size_t size = contentSize - pos;
        if (size > maxChunkSize) {
            size = maxChunkSize;
        }
        auto begin = content.begin() + pos;
        auto end = content.begin() + pos + size;
        ret.emplace_back(vector<uint8_t>(begin, end));
        pos += maxChunkSize;
    }

    return ret;
}

vector<uint8_t> testHardwareBoundKey = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const vector<uint8_t>& getTestHardwareBoundKey() {
    return testHardwareBoundKey;
}

}  // namespace support
}  // namespace identity
}  // namespace hardware
}  // namespace android
