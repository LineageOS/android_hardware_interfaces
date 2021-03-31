/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "RemotelyProvisionedComponent.h"

#include <assert.h>
#include <variant>

#include <cppbor.h>
#include <cppbor_parse.h>

#include <KeyMintUtils.h>
#include <cppcose/cppcose.h>
#include <keymaster/keymaster_configuration.h>
#include <remote_prov/remote_prov_utils.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

namespace aidl::android::hardware::security::keymint {

using ::std::string;
using ::std::tuple;
using ::std::unique_ptr;
using ::std::variant;
using ::std::vector;
using bytevec = ::std::vector<uint8_t>;

using namespace cppcose;
using namespace keymaster;

namespace {

// Hard-coded set of acceptable public keys that can act as roots of EEK chains.
inline const vector<bytevec> kAuthorizedEekRoots = {
        // TODO(drysdale): replace this random value with real root pubkey(s).
        {0x5c, 0xea, 0x4b, 0xd2, 0x31, 0x27, 0x15, 0x5e, 0x62, 0x94, 0x70,
         0x53, 0x94, 0x43, 0x0f, 0x9a, 0x89, 0xd5, 0xc5, 0x0f, 0x82, 0x9b,
         0xcd, 0x10, 0xe0, 0x79, 0xef, 0xf3, 0xfa, 0x40, 0xeb, 0x0a},
};

constexpr auto STATUS_FAILED = RemotelyProvisionedComponent::STATUS_FAILED;
constexpr auto STATUS_INVALID_EEK = RemotelyProvisionedComponent::STATUS_INVALID_EEK;
constexpr auto STATUS_INVALID_MAC = RemotelyProvisionedComponent::STATUS_INVALID_MAC;
constexpr uint32_t kAffinePointLength = 32;
struct AStatusDeleter {
    void operator()(AStatus* p) { AStatus_delete(p); }
};

// TODO(swillden): Remove the dependency on AStatus stuff.  The COSE lib should use something like
// StatusOr, but it shouldn't depend on AStatus.
class Status {
  public:
    Status() {}
    Status(int32_t errCode, const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(errCode, errMsg.c_str())) {}
    explicit Status(const std::string& errMsg)
        : status_(AStatus_fromServiceSpecificErrorWithMessage(STATUS_FAILED, errMsg.c_str())) {}
    Status(AStatus* status) : status_(status) {}
    Status(Status&&) = default;
    Status(const Status&) = delete;

    operator ::ndk::ScopedAStatus() && { return ndk::ScopedAStatus(status_.release()); }

    bool isOk() { return !status_; }

    // Don't call getMessage() unless isOk() returns false;
    const char* getMessage() const { return AStatus_getMessage(status_.get()); }

  private:
    std::unique_ptr<AStatus, AStatusDeleter> status_;
};

template <typename T>
class StatusOr {
  public:
    StatusOr(AStatus* status) : status_(status) {}
    StatusOr(Status status) : status_(std::move(status)) {}
    StatusOr(T val) : value_(std::move(val)) {}

    bool isOk() { return status_.isOk(); }

    T* operator->() & {
        assert(isOk());
        return &value_.value();
    }
    T& operator*() & {
        assert(isOk());
        return value_.value();
    }
    T&& operator*() && {
        assert(isOk());
        return std::move(value_).value();
    }

    const char* getMessage() const {
        assert(!isOk());
        return status_.getMessage();
    }

    Status moveError() {
        assert(!isOk());
        return std::move(status_);
    }

    T moveValue() { return std::move(value_).value(); }

  private:
    Status status_;
    std::optional<T> value_;
};

StatusOr<std::pair<bytevec /* EEK pub */, bytevec /* EEK ID */>> validateAndExtractEekPubAndId(
        bool testMode, const bytevec& endpointEncryptionCertChain) {
    auto [item, newPos, errMsg] = cppbor::parse(endpointEncryptionCertChain);

    if (!item || !item->asArray()) {
        return Status("Error parsing EEK chain" + errMsg);
    }

    const cppbor::Array* certArr = item->asArray();
    bytevec lastPubKey;
    for (int i = 0; i < certArr->size(); ++i) {
        auto cosePubKey = verifyAndParseCoseSign1(testMode, certArr->get(i)->asArray(),
                                                  std::move(lastPubKey), bytevec{} /* AAD */);
        if (!cosePubKey) {
            return Status(STATUS_INVALID_EEK,
                          "Failed to validate EEK chain: " + cosePubKey.moveMessage());
        }
        lastPubKey = *std::move(cosePubKey);

        // In prod mode the first pubkey should match a well-known Google public key.
        if (!testMode && i == 0 &&
            std::find(kAuthorizedEekRoots.begin(), kAuthorizedEekRoots.end(), lastPubKey) ==
                    kAuthorizedEekRoots.end()) {
            return Status(STATUS_INVALID_EEK, "Unrecognized root of EEK chain");
        }
    }

    auto eek = CoseKey::parseX25519(lastPubKey, true /* requireKid */);
    if (!eek) return Status(STATUS_INVALID_EEK, "Failed to get EEK: " + eek.moveMessage());

    return std::make_pair(eek->getBstrValue(CoseKey::PUBKEY_X).value(),
                          eek->getBstrValue(CoseKey::KEY_ID).value());
}

StatusOr<bytevec /* pubkeys */> validateAndExtractPubkeys(bool testMode,
                                                          const vector<MacedPublicKey>& keysToSign,
                                                          const bytevec& macKey) {
    auto pubKeysToMac = cppbor::Array();
    for (auto& keyToSign : keysToSign) {
        auto [macedKeyItem, _, coseMacErrMsg] = cppbor::parse(keyToSign.macedKey);
        if (!macedKeyItem || !macedKeyItem->asArray() ||
            macedKeyItem->asArray()->size() != kCoseMac0EntryCount) {
            return Status("Invalid COSE_Mac0 structure");
        }

        auto protectedParms = macedKeyItem->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
        auto unprotectedParms = macedKeyItem->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
        auto payload = macedKeyItem->asArray()->get(kCoseMac0Payload)->asBstr();
        auto tag = macedKeyItem->asArray()->get(kCoseMac0Tag)->asBstr();
        if (!protectedParms || !unprotectedParms || !payload || !tag) {
            return Status("Invalid COSE_Mac0 contents");
        }

        auto [protectedMap, __, errMsg] = cppbor::parse(protectedParms);
        if (!protectedMap || !protectedMap->asMap()) {
            return Status("Invalid Mac0 protected: " + errMsg);
        }
        auto& algo = protectedMap->asMap()->get(ALGORITHM);
        if (!algo || !algo->asInt() || algo->asInt()->value() != HMAC_256) {
            return Status("Unsupported Mac0 algorithm");
        }

        auto pubKey = CoseKey::parse(payload->value(), EC2, ES256, P256);
        if (!pubKey) return Status(pubKey.moveMessage());

        bool testKey = static_cast<bool>(pubKey->getMap().get(CoseKey::TEST_KEY));
        if (testMode && !testKey) {
            return Status(BnRemotelyProvisionedComponent::STATUS_PRODUCTION_KEY_IN_TEST_REQUEST,
                          "Production key in test request");
        } else if (!testMode && testKey) {
            return Status(BnRemotelyProvisionedComponent::STATUS_TEST_KEY_IN_PRODUCTION_REQUEST,
                          "Test key in production request");
        }

        auto macTag = generateCoseMac0Mac(macKey, {} /* external_aad */, payload->value());
        if (!macTag) return Status(STATUS_INVALID_MAC, macTag.moveMessage());
        if (macTag->size() != tag->value().size() ||
            CRYPTO_memcmp(macTag->data(), tag->value().data(), macTag->size()) != 0) {
            return Status(STATUS_INVALID_MAC, "MAC tag mismatch");
        }

        pubKeysToMac.add(pubKey->moveMap());
    }

    return pubKeysToMac.encode();
}

StatusOr<std::pair<bytevec, bytevec>> buildCosePublicKeyFromKmCert(
        const keymaster_blob_t* km_cert) {
    if (km_cert == nullptr) {
        return Status(STATUS_FAILED, "km_cert is a nullptr");
    }
    const uint8_t* temp = km_cert->data;
    X509* cert = d2i_X509(NULL, &temp, km_cert->data_length);
    if (cert == nullptr) {
        return Status(STATUS_FAILED, "d2i_X509 returned null when attempting to get the cert.");
    }
    EVP_PKEY* pubKey = X509_get_pubkey(cert);
    if (pubKey == nullptr) {
        return Status(STATUS_FAILED, "Boringssl failed to get the public key from the cert");
    }
    EC_KEY* ecKey = EVP_PKEY_get0_EC_KEY(pubKey);
    if (ecKey == nullptr) {
        return Status(STATUS_FAILED,
                      "The key in the certificate returned from GenerateKey is not "
                      "an EC key.");
    }
    const EC_POINT* jacobian_coords = EC_KEY_get0_public_key(ecKey);
    BIGNUM x;
    BIGNUM y;
    BN_CTX* ctx = BN_CTX_new();
    if (ctx == nullptr) {
        return Status(STATUS_FAILED, "Memory allocation failure for BN_CTX");
    }
    if (!EC_POINT_get_affine_coordinates_GFp(EC_KEY_get0_group(ecKey), jacobian_coords, &x, &y,
                                             ctx)) {
        return Status(STATUS_FAILED, "Failed to get affine coordinates");
    }
    bytevec x_bytestring(kAffinePointLength);
    bytevec y_bytestring(kAffinePointLength);
    if (BN_bn2binpad(&x, x_bytestring.data(), kAffinePointLength) != kAffinePointLength) {
        return Status(STATUS_FAILED, "Wrote incorrect number of bytes for x coordinate");
    }
    if (BN_bn2binpad(&y, y_bytestring.data(), kAffinePointLength) != kAffinePointLength) {
        return Status(STATUS_FAILED, "Wrote incorrect number of bytes for y coordinate");
    }
    BN_CTX_free(ctx);
    return std::make_pair(x_bytestring, y_bytestring);
}

cppbor::Array buildCertReqRecipients(const bytevec& pubkey, const bytevec& kid) {
    return cppbor::Array()                   // Array of recipients
            .add(cppbor::Array()             // Recipient
                         .add(cppbor::Map()  // Protected
                                      .add(ALGORITHM, ECDH_ES_HKDF_256)
                                      .canonicalize()
                                      .encode())
                         .add(cppbor::Map()  // Unprotected
                                      .add(COSE_KEY, cppbor::Map()
                                                             .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                                                             .add(CoseKey::CURVE, cppcose::X25519)
                                                             .add(CoseKey::PUBKEY_X, pubkey)
                                                             .canonicalize())
                                      .add(KEY_ID, kid)
                                      .canonicalize())
                         .add(cppbor::Null()));  // No ciphertext
}

static keymaster_key_param_t kKeyMintEcdsaP256Params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_ATTEST_KEY),
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_EC), Authorization(TAG_KEY_SIZE, 256),
        Authorization(TAG_DIGEST, KM_DIGEST_SHA_2_256),
        Authorization(TAG_EC_CURVE, KM_EC_CURVE_P_256), Authorization(TAG_NO_AUTH_REQUIRED),
        // The certificate generated by KM will be discarded, these values don't matter.
        Authorization(TAG_CERTIFICATE_NOT_BEFORE, 0), Authorization(TAG_CERTIFICATE_NOT_AFTER, 0)};

}  // namespace

RemotelyProvisionedComponent::RemotelyProvisionedComponent(
        std::shared_ptr<keymint::AndroidKeyMintDevice> keymint) {
    std::tie(devicePrivKey_, bcc_) = generateBcc();
    impl_ = keymint->getKeymasterImpl();
}

RemotelyProvisionedComponent::~RemotelyProvisionedComponent() {}

ScopedAStatus RemotelyProvisionedComponent::generateEcdsaP256KeyPair(bool testMode,
                                                                     MacedPublicKey* macedPublicKey,
                                                                     bytevec* privateKeyHandle) {
    // TODO(jbires): The following should move from ->GenerateKey to ->GenerateRKPKey and everything
    //              after the GenerateKey call should basically be moved into that new function call
    //              as well once the issue with libcppbor in system/keymaster is sorted out
    GenerateKeyRequest request(impl_->message_version());
    request.key_description.Reinitialize(kKeyMintEcdsaP256Params,
                                         array_length(kKeyMintEcdsaP256Params));
    GenerateKeyResponse response(impl_->message_version());
    impl_->GenerateKey(request, &response);
    if (response.error != KM_ERROR_OK) {
        return km_utils::kmError2ScopedAStatus(response.error);
    }

    if (response.certificate_chain.entry_count != 1) {
        // Error: Need the single non-signed certificate with the public key in it.
        return Status(STATUS_FAILED,
                      "Expected to receive a single certificate from GenerateKey. Instead got: " +
                              std::to_string(response.certificate_chain.entry_count));
    }
    auto affineCoords = buildCosePublicKeyFromKmCert(response.certificate_chain.begin());
    if (!affineCoords.isOk()) return affineCoords.moveError();
    cppbor::Map cosePublicKeyMap = cppbor::Map()
                                           .add(CoseKey::KEY_TYPE, EC2)
                                           .add(CoseKey::ALGORITHM, ES256)
                                           .add(CoseKey::CURVE, cppcose::P256)
                                           .add(CoseKey::PUBKEY_X, affineCoords->first)
                                           .add(CoseKey::PUBKEY_Y, affineCoords->second);
    if (testMode) {
        cosePublicKeyMap.add(CoseKey::TEST_KEY, cppbor::Null());
    }

    bytevec cosePublicKey = cosePublicKeyMap.canonicalize().encode();

    auto macedKey = constructCoseMac0(testMode ? remote_prov::kTestMacKey : macKey_,
                                      {} /* externalAad */, cosePublicKey);
    if (!macedKey) return Status(macedKey.moveMessage());

    macedPublicKey->macedKey = macedKey->encode();
    *privateKeyHandle = km_utils::kmBlob2vector(response.key_blob);
    return ScopedAStatus::ok();
}

ScopedAStatus RemotelyProvisionedComponent::generateCertificateRequest(
        bool testMode, const vector<MacedPublicKey>& keysToSign,
        const bytevec& endpointEncCertChain, const bytevec& challenge, DeviceInfo* deviceInfo,
        ProtectedData* protectedData, bytevec* keysToSignMac) {
    auto pubKeysToSign = validateAndExtractPubkeys(testMode, keysToSign,
                                                   testMode ? remote_prov::kTestMacKey : macKey_);
    if (!pubKeysToSign.isOk()) return pubKeysToSign.moveError();

    bytevec ephemeralMacKey = remote_prov::randomBytes(SHA256_DIGEST_LENGTH);

    auto pubKeysToSignMac = generateCoseMac0Mac(ephemeralMacKey, bytevec{}, *pubKeysToSign);
    if (!pubKeysToSignMac) return Status(pubKeysToSignMac.moveMessage());
    *keysToSignMac = *std::move(pubKeysToSignMac);

    bytevec devicePrivKey;
    cppbor::Array bcc;
    if (testMode) {
        std::tie(devicePrivKey, bcc) = generateBcc();
    } else {
        devicePrivKey = devicePrivKey_;
        bcc = bcc_.clone();
    }

    deviceInfo->deviceInfo = createDeviceInfo();
    auto signedMac = constructCoseSign1(devicePrivKey /* Signing key */,  //
                                        ephemeralMacKey /* Payload */,
                                        cppbor::Array() /* AAD */
                                                .add(challenge)
                                                .add(deviceInfo->deviceInfo)
                                                .encode());
    if (!signedMac) return Status(signedMac.moveMessage());

    bytevec ephemeralPrivKey(X25519_PRIVATE_KEY_LEN);
    bytevec ephemeralPubKey(X25519_PUBLIC_VALUE_LEN);
    X25519_keypair(ephemeralPubKey.data(), ephemeralPrivKey.data());

    auto eek = validateAndExtractEekPubAndId(testMode, endpointEncCertChain);
    if (!eek.isOk()) return eek.moveError();

    auto sessionKey = x25519_HKDF_DeriveKey(ephemeralPubKey, ephemeralPrivKey, eek->first,
                                            true /* senderIsA */);
    if (!sessionKey) return Status(sessionKey.moveMessage());

    auto coseEncrypted =
            constructCoseEncrypt(*sessionKey, remote_prov::randomBytes(kAesGcmNonceLength),
                                 cppbor::Array()  // payload
                                         .add(signedMac.moveValue())
                                         .add(std::move(bcc))
                                         .encode(),
                                 {},  // aad
                                 buildCertReqRecipients(ephemeralPubKey, eek->second));

    if (!coseEncrypted) return Status(coseEncrypted.moveMessage());
    protectedData->protectedData = coseEncrypted->encode();

    return ScopedAStatus::ok();
}

bytevec RemotelyProvisionedComponent::deriveBytesFromHbk(const string& context,
                                                         size_t numBytes) const {
    bytevec fakeHbk(32, 0);
    bytevec result(numBytes);

    // TODO(swillden): Figure out if HKDF can fail.  It doesn't seem like it should be able to,
    // but the function does return an error code.
    HKDF(result.data(), numBytes,               //
         EVP_sha256(),                          //
         fakeHbk.data(), fakeHbk.size(),        //
         nullptr /* salt */, 0 /* salt len */,  //
         reinterpret_cast<const uint8_t*>(context.data()), context.size());

    return result;
}

bytevec RemotelyProvisionedComponent::createDeviceInfo() const {
    return cppbor::Map().encode();
}

std::pair<bytevec /* privKey */, cppbor::Array /* BCC */>
RemotelyProvisionedComponent::generateBcc() {
    bytevec privKey(ED25519_PRIVATE_KEY_LEN);
    bytevec pubKey(ED25519_PUBLIC_KEY_LEN);

    ED25519_keypair(pubKey.data(), privKey.data());

    auto coseKey = cppbor::Map()
                           .add(CoseKey::KEY_TYPE, OCTET_KEY_PAIR)
                           .add(CoseKey::ALGORITHM, EDDSA)
                           .add(CoseKey::CURVE, ED25519)
                           .add(CoseKey::KEY_OPS, VERIFY)
                           .add(CoseKey::PUBKEY_X, pubKey)
                           .canonicalize()
                           .encode();
    auto sign1Payload = cppbor::Map()
                                .add(1 /* Issuer */, "Issuer")
                                .add(2 /* Subject */, "Subject")
                                .add(-4670552 /* Subject Pub Key */, coseKey)
                                .add(-4670553 /* Key Usage (little-endian order) */,
                                     std::vector<uint8_t>{0x20} /* keyCertSign = 1<<5 */)
                                .canonicalize()
                                .encode();
    auto coseSign1 = constructCoseSign1(privKey,       /* signing key */
                                        cppbor::Map(), /* extra protected */
                                        sign1Payload, {} /* AAD */);
    assert(coseSign1);

    return {privKey, cppbor::Array().add(coseKey).add(coseSign1.moveValue())};
}

}  // namespace aidl::android::hardware::security::keymint
