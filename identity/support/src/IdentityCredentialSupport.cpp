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

#include <cppbor.h>
#include <cppbor_parse.h>

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
// CBOR utilities.
// ---------------------------------------------------------------------------

static bool cborAreAllElementsNonCompound(const cppbor::CompoundItem* compoundItem) {
    if (compoundItem->type() == cppbor::ARRAY) {
        const cppbor::Array* array = compoundItem->asArray();
        for (size_t n = 0; n < array->size(); n++) {
            const cppbor::Item* entry = (*array)[n].get();
            switch (entry->type()) {
                case cppbor::ARRAY:
                case cppbor::MAP:
                    return false;
                default:
                    break;
            }
        }
    } else {
        const cppbor::Map* map = compoundItem->asMap();
        for (size_t n = 0; n < map->size(); n++) {
            auto [keyEntry, valueEntry] = (*map)[n];
            switch (keyEntry->type()) {
                case cppbor::ARRAY:
                case cppbor::MAP:
                    return false;
                default:
                    break;
            }
            switch (valueEntry->type()) {
                case cppbor::ARRAY:
                case cppbor::MAP:
                    return false;
                default:
                    break;
            }
        }
    }
    return true;
}

static bool cborPrettyPrintInternal(const cppbor::Item* item, string& out, size_t indent,
                                    size_t maxBStrSize, const vector<string>& mapKeysToNotPrint) {
    char buf[80];

    string indentString(indent, ' ');

    switch (item->type()) {
        case cppbor::UINT:
            snprintf(buf, sizeof(buf), "%" PRIu64, item->asUint()->unsignedValue());
            out.append(buf);
            break;

        case cppbor::NINT:
            snprintf(buf, sizeof(buf), "%" PRId64, item->asNint()->value());
            out.append(buf);
            break;

        case cppbor::BSTR: {
            const cppbor::Bstr* bstr = item->asBstr();
            const vector<uint8_t>& value = bstr->value();
            if (value.size() > maxBStrSize) {
                unsigned char digest[SHA_DIGEST_LENGTH];
                SHA_CTX ctx;
                SHA1_Init(&ctx);
                SHA1_Update(&ctx, value.data(), value.size());
                SHA1_Final(digest, &ctx);
                char buf2[SHA_DIGEST_LENGTH * 2 + 1];
                for (size_t n = 0; n < SHA_DIGEST_LENGTH; n++) {
                    snprintf(buf2 + n * 2, 3, "%02x", digest[n]);
                }
                snprintf(buf, sizeof(buf), "<bstr size=%zd sha1=%s>", value.size(), buf2);
                out.append(buf);
            } else {
                out.append("{");
                for (size_t n = 0; n < value.size(); n++) {
                    if (n > 0) {
                        out.append(", ");
                    }
                    snprintf(buf, sizeof(buf), "0x%02x", value[n]);
                    out.append(buf);
                }
                out.append("}");
            }
        } break;

        case cppbor::TSTR:
            out.append("'");
            {
                // TODO: escape "'" characters
                out.append(item->asTstr()->value().c_str());
            }
            out.append("'");
            break;

        case cppbor::ARRAY: {
            const cppbor::Array* array = item->asArray();
            if (array->size() == 0) {
                out.append("[]");
            } else if (cborAreAllElementsNonCompound(array)) {
                out.append("[");
                for (size_t n = 0; n < array->size(); n++) {
                    if (!cborPrettyPrintInternal((*array)[n].get(), out, indent + 2, maxBStrSize,
                                                 mapKeysToNotPrint)) {
                        return false;
                    }
                    out.append(", ");
                }
                out.append("]");
            } else {
                out.append("[\n" + indentString);
                for (size_t n = 0; n < array->size(); n++) {
                    out.append("  ");
                    if (!cborPrettyPrintInternal((*array)[n].get(), out, indent + 2, maxBStrSize,
                                                 mapKeysToNotPrint)) {
                        return false;
                    }
                    out.append(",\n" + indentString);
                }
                out.append("]");
            }
        } break;

        case cppbor::MAP: {
            const cppbor::Map* map = item->asMap();

            if (map->size() == 0) {
                out.append("{}");
            } else {
                out.append("{\n" + indentString);
                for (size_t n = 0; n < map->size(); n++) {
                    out.append("  ");

                    auto [map_key, map_value] = (*map)[n];

                    if (!cborPrettyPrintInternal(map_key.get(), out, indent + 2, maxBStrSize,
                                                 mapKeysToNotPrint)) {
                        return false;
                    }
                    out.append(" : ");
                    if (map_key->type() == cppbor::TSTR &&
                        std::find(mapKeysToNotPrint.begin(), mapKeysToNotPrint.end(),
                                  map_key->asTstr()->value()) != mapKeysToNotPrint.end()) {
                        out.append("<not printed>");
                    } else {
                        if (!cborPrettyPrintInternal(map_value.get(), out, indent + 2, maxBStrSize,
                                                     mapKeysToNotPrint)) {
                            return false;
                        }
                    }
                    out.append(",\n" + indentString);
                }
                out.append("}");
            }
        } break;

        case cppbor::SEMANTIC: {
            const cppbor::Semantic* semantic = item->asSemantic();
            snprintf(buf, sizeof(buf), "tag %" PRIu64 " ", semantic->value());
            out.append(buf);
            cborPrettyPrintInternal(semantic->child().get(), out, indent, maxBStrSize,
                                    mapKeysToNotPrint);
        } break;

        case cppbor::SIMPLE:
            const cppbor::Bool* asBool = item->asSimple()->asBool();
            const cppbor::Null* asNull = item->asSimple()->asNull();
            if (asBool != nullptr) {
                out.append(asBool->value() ? "true" : "false");
            } else if (asNull != nullptr) {
                out.append("null");
            } else {
                LOG(ERROR) << "Only boolean/null is implemented for SIMPLE";
                return false;
            }
            break;
    }

    return true;
}

string cborPrettyPrint(const vector<uint8_t>& encodedCbor, size_t maxBStrSize,
                       const vector<string>& mapKeysToNotPrint) {
    auto [item, _, message] = cppbor::parse(encodedCbor);
    if (item == nullptr) {
        LOG(ERROR) << "Data to pretty print is not valid CBOR: " << message;
        return "";
    }

    string out;
    cborPrettyPrintInternal(item.get(), out, 0, maxBStrSize, mapKeysToNotPrint);
    return out;
}

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

struct EVP_CIPHER_CTX_Deleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx != nullptr) {
            EVP_CIPHER_CTX_free(ctx);
        }
    }
};

using EvpCipherCtxPtr = unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter>;

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

struct EC_KEY_Deleter {
    void operator()(EC_KEY* key) const {
        if (key != nullptr) {
            EC_KEY_free(key);
        }
    }
};
using EC_KEY_Ptr = unique_ptr<EC_KEY, EC_KEY_Deleter>;

struct EVP_PKEY_Deleter {
    void operator()(EVP_PKEY* key) const {
        if (key != nullptr) {
            EVP_PKEY_free(key);
        }
    }
};
using EVP_PKEY_Ptr = unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;

struct EVP_PKEY_CTX_Deleter {
    void operator()(EVP_PKEY_CTX* ctx) const {
        if (ctx != nullptr) {
            EVP_PKEY_CTX_free(ctx);
        }
    }
};
using EVP_PKEY_CTX_Ptr = unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_Deleter>;

struct EC_GROUP_Deleter {
    void operator()(EC_GROUP* group) const {
        if (group != nullptr) {
            EC_GROUP_free(group);
        }
    }
};
using EC_GROUP_Ptr = unique_ptr<EC_GROUP, EC_GROUP_Deleter>;

struct EC_POINT_Deleter {
    void operator()(EC_POINT* point) const {
        if (point != nullptr) {
            EC_POINT_free(point);
        }
    }
};

using EC_POINT_Ptr = unique_ptr<EC_POINT, EC_POINT_Deleter>;

struct ECDSA_SIG_Deleter {
    void operator()(ECDSA_SIG* sig) const {
        if (sig != nullptr) {
            ECDSA_SIG_free(sig);
        }
    }
};
using ECDSA_SIG_Ptr = unique_ptr<ECDSA_SIG, ECDSA_SIG_Deleter>;

struct X509_Deleter {
    void operator()(X509* x509) const {
        if (x509 != nullptr) {
            X509_free(x509);
        }
    }
};
using X509_Ptr = unique_ptr<X509, X509_Deleter>;

struct PKCS12_Deleter {
    void operator()(PKCS12* pkcs12) const {
        if (pkcs12 != nullptr) {
            PKCS12_free(pkcs12);
        }
    }
};
using PKCS12_Ptr = unique_ptr<PKCS12, PKCS12_Deleter>;

struct BIGNUM_Deleter {
    void operator()(BIGNUM* bignum) const {
        if (bignum != nullptr) {
            BN_free(bignum);
        }
    }
};
using BIGNUM_Ptr = unique_ptr<BIGNUM, BIGNUM_Deleter>;

struct ASN1_INTEGER_Deleter {
    void operator()(ASN1_INTEGER* value) const {
        if (value != nullptr) {
            ASN1_INTEGER_free(value);
        }
    }
};
using ASN1_INTEGER_Ptr = unique_ptr<ASN1_INTEGER, ASN1_INTEGER_Deleter>;

struct ASN1_TIME_Deleter {
    void operator()(ASN1_TIME* value) const {
        if (value != nullptr) {
            ASN1_TIME_free(value);
        }
    }
};
using ASN1_TIME_Ptr = unique_ptr<ASN1_TIME, ASN1_TIME_Deleter>;

struct X509_NAME_Deleter {
    void operator()(X509_NAME* value) const {
        if (value != nullptr) {
            X509_NAME_free(value);
        }
    }
};
using X509_NAME_Ptr = unique_ptr<X509_NAME, X509_NAME_Deleter>;

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

optional<vector<uint8_t>> signEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data) {
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

    auto digest = sha256(data);
    ECDSA_SIG* sig = ECDSA_do_sign(digest.data(), digest.size(), ec_key.get());
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

optional<vector<uint8_t>> createEcKeyPair() {
    auto ec_key = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
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
    privateKey.resize(BN_num_bytes(bignum));
    BN_bn2bin(bignum, privateKey.data());
    return privateKey;
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
        time_t validityNotBefore, time_t validityNotAfter) {
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

    int algoId = OBJ_obj2nid(certs[0]->cert_info->key->algor->algorithm);
    if (algoId != NID_X9_62_id_ecPublicKey) {
        LOG(ERROR) << "Expected NID_X9_62_id_ecPublicKey, got " << OBJ_nid2ln(algoId);
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
    if (BN_bn2binpad(sig->r, ecdsaCoseSignature.data(), 32) != 32) {
        LOG(ERROR) << "Error encoding r";
        return false;
    }
    if (BN_bn2binpad(sig->s, ecdsaCoseSignature.data() + 32, 32) != 32) {
        LOG(ERROR) << "Error encoding s";
        return false;
    }
    return true;
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
        auto [keyItem, valueItem] = (*unprotectedHeaders)[n];
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

// ---------------------------------------------------------------------------
// Platform abstraction.
// ---------------------------------------------------------------------------

// This is not a very random HBK but that's OK because this is the SW
// implementation where it can't be kept secret.
vector<uint8_t> hardwareBoundKey = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const vector<uint8_t>& getHardwareBoundKey() {
    return hardwareBoundKey;
}

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

Result okResult{ResultCode::OK, ""};

const Result& resultOK() {
    return okResult;
}

Result result(ResultCode code, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    string str;
    android::base::StringAppendV(&str, format, ap);
    va_end(ap);
    return Result{code, str};
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

vector<uint8_t> secureAccessControlProfileEncodeCbor(const SecureAccessControlProfile& profile) {
    cppbor::Map map;
    map.add("id", profile.id);

    if (profile.readerCertificate.size() > 0) {
        map.add("readerCertificate", cppbor::Bstr(profile.readerCertificate));
    }

    if (profile.userAuthenticationRequired) {
        map.add("userAuthenticationRequired", profile.userAuthenticationRequired);
        map.add("timeoutMillis", profile.timeoutMillis);
        map.add("secureUserId", profile.secureUserId);
    }

    return map.encode();
}

optional<vector<uint8_t>> secureAccessControlProfileCalcMac(
        const SecureAccessControlProfile& profile, const vector<uint8_t>& storageKey) {
    vector<uint8_t> cborData = secureAccessControlProfileEncodeCbor(profile);

    optional<vector<uint8_t>> nonce = getRandom(12);
    if (!nonce) {
        return {};
    }
    optional<vector<uint8_t>> macO = encryptAes128Gcm(storageKey, nonce.value(), {}, cborData);
    if (!macO) {
        return {};
    }
    return macO.value();
}

bool secureAccessControlProfileCheckMac(const SecureAccessControlProfile& profile,
                                        const vector<uint8_t>& storageKey) {
    vector<uint8_t> cborData = secureAccessControlProfileEncodeCbor(profile);

    if (profile.mac.size() < kAesGcmIvSize) {
        return false;
    }
    vector<uint8_t> nonce =
            vector<uint8_t>(profile.mac.begin(), profile.mac.begin() + kAesGcmIvSize);
    optional<vector<uint8_t>> mac = encryptAes128Gcm(storageKey, nonce, {}, cborData);
    if (!mac) {
        return false;
    }
    if (mac.value() != vector<uint8_t>(profile.mac)) {
        return false;
    }
    return true;
}

vector<uint8_t> testHardwareBoundKey = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const vector<uint8_t>& getTestHardwareBoundKey() {
    return testHardwareBoundKey;
}

vector<uint8_t> entryCreateAdditionalData(const string& nameSpace, const string& name,
                                          const vector<uint16_t> accessControlProfileIds) {
    cppbor::Map map;
    map.add("Namespace", nameSpace);
    map.add("Name", name);

    cppbor::Array acpIds;
    for (auto id : accessControlProfileIds) {
        acpIds.add(id);
    }
    map.add("AccessControlProfileIds", std::move(acpIds));
    return map.encode();
}

}  // namespace support
}  // namespace identity
}  // namespace hardware
}  // namespace android
