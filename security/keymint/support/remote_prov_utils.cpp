/*
 * Copyright (c) 2019, The Android Open Source Project
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

#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include "aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h"

#include <aidl/android/hardware/security/keymint/RpcHardwareInfo.h>
#include <android-base/properties.h>
#include <cppbor.h>
#include <json/json.h>
#include <keymaster/km_openssl/ec_key.h>
#include <keymaster/km_openssl/ecdsa_operation.h>
#include <keymaster/km_openssl/openssl_err.h>
#include <keymaster/km_openssl/openssl_utils.h>
#include <openssl/base64.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <remote_prov/remote_prov_utils.h>

namespace aidl::android::hardware::security::keymint::remote_prov {

constexpr uint32_t kBccPayloadIssuer = 1;
constexpr uint32_t kBccPayloadSubject = 2;
constexpr int32_t kBccPayloadSubjPubKey = -4670552;
constexpr int32_t kBccPayloadKeyUsage = -4670553;
constexpr int kP256AffinePointSize = 32;

using EC_KEY_Ptr = bssl::UniquePtr<EC_KEY>;
using EVP_PKEY_Ptr = bssl::UniquePtr<EVP_PKEY>;
using EVP_PKEY_CTX_Ptr = bssl::UniquePtr<EVP_PKEY_CTX>;

ErrMsgOr<bytevec> ecKeyGetPrivateKey(const EC_KEY* ecKey) {
    // Extract private key.
    const BIGNUM* bignum = EC_KEY_get0_private_key(ecKey);
    if (bignum == nullptr) {
        return "Error getting bignum from private key";
    }
    // Pad with zeros in case the length is lesser than 32.
    bytevec privKey(32, 0);
    BN_bn2binpad(bignum, privKey.data(), privKey.size());
    return privKey;
}

ErrMsgOr<bytevec> ecKeyGetPublicKey(const EC_KEY* ecKey) {
    // Extract public key.
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }
    const EC_POINT* point = EC_KEY_get0_public_key(ecKey);
    if (point == nullptr) return "Error getting ecpoint from public key";

    int size =
        EC_POINT_point2oct(group.get(), point, POINT_CONVERSION_UNCOMPRESSED, nullptr, 0, nullptr);
    if (size == 0) {
        return "Error generating public key encoding";
    }

    bytevec publicKey;
    publicKey.resize(size);
    EC_POINT_point2oct(group.get(), point, POINT_CONVERSION_UNCOMPRESSED, publicKey.data(),
                       publicKey.size(), nullptr);
    return publicKey;
}

ErrMsgOr<std::tuple<bytevec, bytevec>> getAffineCoordinates(const bytevec& pubKey) {
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }
    auto point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    if (EC_POINT_oct2point(group.get(), point.get(), pubKey.data(), pubKey.size(), nullptr) != 1) {
        return "Error decoding publicKey";
    }
    BIGNUM_Ptr x(BN_new());
    BIGNUM_Ptr y(BN_new());
    BN_CTX_Ptr ctx(BN_CTX_new());
    if (!ctx.get()) return "Failed to create BN_CTX instance";

    if (!EC_POINT_get_affine_coordinates_GFp(group.get(), point.get(), x.get(), y.get(),
                                             ctx.get())) {
        return "Failed to get affine coordinates from ECPoint";
    }
    bytevec pubX(kP256AffinePointSize);
    bytevec pubY(kP256AffinePointSize);
    if (BN_bn2binpad(x.get(), pubX.data(), kP256AffinePointSize) != kP256AffinePointSize) {
        return "Error in converting absolute value of x coordinate to big-endian";
    }
    if (BN_bn2binpad(y.get(), pubY.data(), kP256AffinePointSize) != kP256AffinePointSize) {
        return "Error in converting absolute value of y coordinate to big-endian";
    }
    return std::make_tuple(std::move(pubX), std::move(pubY));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateEc256KeyPair() {
    auto ec_key = EC_KEY_Ptr(EC_KEY_new());
    if (ec_key.get() == nullptr) {
        return "Failed to allocate ec key";
    }

    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        return "Error creating EC group by curve name";
    }

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        return "Error generating key";
    }

    auto privKey = ecKeyGetPrivateKey(ec_key.get());
    if (!privKey) return privKey.moveMessage();

    auto pubKey = ecKeyGetPublicKey(ec_key.get());
    if (!pubKey) return pubKey.moveMessage();

    return std::make_tuple(pubKey.moveValue(), privKey.moveValue());
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateX25519KeyPair() {
    /* Generate X25519 key pair */
    bytevec pubKey(X25519_PUBLIC_VALUE_LEN);
    bytevec privKey(X25519_PRIVATE_KEY_LEN);
    X25519_keypair(pubKey.data(), privKey.data());
    return std::make_tuple(std::move(pubKey), std::move(privKey));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateED25519KeyPair() {
    /* Generate ED25519 key pair */
    bytevec pubKey(ED25519_PUBLIC_KEY_LEN);
    bytevec privKey(ED25519_PRIVATE_KEY_LEN);
    ED25519_keypair(pubKey.data(), privKey.data());
    return std::make_tuple(std::move(pubKey), std::move(privKey));
}

ErrMsgOr<std::tuple<bytevec, bytevec>> generateKeyPair(int32_t supportedEekCurve, bool isEek) {
    switch (supportedEekCurve) {
    case RpcHardwareInfo::CURVE_25519:
        if (isEek) {
            return generateX25519KeyPair();
        }
        return generateED25519KeyPair();
    case RpcHardwareInfo::CURVE_P256:
        return generateEc256KeyPair();
    default:
        return "Unknown EEK Curve.";
    }
}

ErrMsgOr<bytevec> constructCoseKey(int32_t supportedEekCurve, const bytevec& eekId,
                                   const bytevec& pubKey) {
    CoseKeyType keyType;
    CoseKeyAlgorithm algorithm;
    CoseKeyCurve curve;
    bytevec pubX;
    bytevec pubY;
    switch (supportedEekCurve) {
    case RpcHardwareInfo::CURVE_25519:
        keyType = OCTET_KEY_PAIR;
        algorithm = (eekId.empty()) ? EDDSA : ECDH_ES_HKDF_256;
        curve = (eekId.empty()) ? ED25519 : cppcose::X25519;
        pubX = pubKey;
        break;
    case RpcHardwareInfo::CURVE_P256: {
        keyType = EC2;
        algorithm = (eekId.empty()) ? ES256 : ECDH_ES_HKDF_256;
        curve = P256;
        auto affineCoordinates = getAffineCoordinates(pubKey);
        if (!affineCoordinates) return affineCoordinates.moveMessage();
        std::tie(pubX, pubY) = affineCoordinates.moveValue();
    } break;
    default:
        return "Unknown EEK Curve.";
    }
    cppbor::Map coseKey = cppbor::Map()
                              .add(CoseKey::KEY_TYPE, keyType)
                              .add(CoseKey::ALGORITHM, algorithm)
                              .add(CoseKey::CURVE, curve)
                              .add(CoseKey::PUBKEY_X, pubX);

    if (!pubY.empty()) coseKey.add(CoseKey::PUBKEY_Y, pubY);
    if (!eekId.empty()) coseKey.add(CoseKey::KEY_ID, eekId);

    return coseKey.canonicalize().encode();
}

bytevec kTestMacKey(32 /* count */, 0 /* byte value */);

bytevec randomBytes(size_t numBytes) {
    bytevec retval(numBytes);
    RAND_bytes(retval.data(), numBytes);
    return retval;
}

ErrMsgOr<cppbor::Array> constructCoseSign1(int32_t supportedEekCurve, const bytevec& key,
                                           const bytevec& payload, const bytevec& aad) {
    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        return constructECDSACoseSign1(key, {} /* protectedParams */, payload, aad);
    } else {
        return cppcose::constructCoseSign1(key, payload, aad);
    }
}

ErrMsgOr<EekChain> generateEekChain(int32_t supportedEekCurve, size_t length,
                                    const bytevec& eekId) {
    if (length < 2) {
        return "EEK chain must contain at least 2 certs.";
    }

    auto eekChain = cppbor::Array();

    bytevec prev_priv_key;
    for (size_t i = 0; i < length - 1; ++i) {
        auto keyPair = generateKeyPair(supportedEekCurve, false);
        if (!keyPair) return keyPair.moveMessage();
        auto [pub_key, priv_key] = keyPair.moveValue();

        // The first signing key is self-signed.
        if (prev_priv_key.empty()) prev_priv_key = priv_key;

        auto coseKey = constructCoseKey(supportedEekCurve, {}, pub_key);
        if (!coseKey) return coseKey.moveMessage();

        auto coseSign1 =
            constructCoseSign1(supportedEekCurve, prev_priv_key, coseKey.moveValue(), {} /* AAD */);
        if (!coseSign1) return coseSign1.moveMessage();
        eekChain.add(coseSign1.moveValue());

        prev_priv_key = priv_key;
    }
    auto keyPair = generateKeyPair(supportedEekCurve, true);
    if (!keyPair) return keyPair.moveMessage();
    auto [pub_key, priv_key] = keyPair.moveValue();

    auto coseKey = constructCoseKey(supportedEekCurve, eekId, pub_key);
    if (!coseKey) return coseKey.moveMessage();

    auto coseSign1 =
        constructCoseSign1(supportedEekCurve, prev_priv_key, coseKey.moveValue(), {} /* AAD */);
    if (!coseSign1) return coseSign1.moveMessage();
    eekChain.add(coseSign1.moveValue());

    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        // convert ec public key to x and y co-ordinates.
        auto affineCoordinates = getAffineCoordinates(pub_key);
        if (!affineCoordinates) return affineCoordinates.moveMessage();
        auto [pubX, pubY] = affineCoordinates.moveValue();
        pub_key.clear();
        pub_key.insert(pub_key.begin(), pubX.begin(), pubX.end());
        pub_key.insert(pub_key.end(), pubY.begin(), pubY.end());
    }

    return EekChain{eekChain.encode(), pub_key, priv_key};
}

bytevec getProdEekChain(int32_t supportedEekCurve) {
    cppbor::Array chain;
    if (supportedEekCurve == RpcHardwareInfo::CURVE_P256) {
        chain.add(cppbor::EncodedItem(bytevec(std::begin(kCoseEncodedEcdsa256RootCert),
                                              std::end(kCoseEncodedEcdsa256RootCert))));
        chain.add(cppbor::EncodedItem(bytevec(std::begin(kCoseEncodedEcdsa256GeekCert),
                                              std::end(kCoseEncodedEcdsa256GeekCert))));
    } else {
        chain.add(cppbor::EncodedItem(
            bytevec(std::begin(kCoseEncodedRootCert), std::end(kCoseEncodedRootCert))));
        chain.add(cppbor::EncodedItem(
            bytevec(std::begin(kCoseEncodedGeekCert), std::end(kCoseEncodedGeekCert))));
    }
    return chain.encode();
}

ErrMsgOr<bytevec> validatePayloadAndFetchPubKey(const cppbor::Map* payload) {
    const auto& issuer = payload->get(kBccPayloadIssuer);
    if (!issuer || !issuer->asTstr()) return "Issuer is not present or not a tstr.";
    const auto& subject = payload->get(kBccPayloadSubject);
    if (!subject || !subject->asTstr()) return "Subject is not present or not a tstr.";
    const auto& keyUsage = payload->get(kBccPayloadKeyUsage);
    if (!keyUsage || !keyUsage->asBstr()) return "Key usage is not present or not a bstr.";
    const auto& serializedKey = payload->get(kBccPayloadSubjPubKey);
    if (!serializedKey || !serializedKey->asBstr()) return "Key is not present or not a bstr.";
    return serializedKey->asBstr()->value();
}

ErrMsgOr<bytevec> verifyAndParseCoseSign1Cwt(const cppbor::Array* coseSign1,
                                             const bytevec& signingCoseKey, const bytevec& aad) {
    if (!coseSign1 || coseSign1->size() != kCoseSign1EntryCount) {
        return "Invalid COSE_Sign1";
    }

    const cppbor::Bstr* protectedParams = coseSign1->get(kCoseSign1ProtectedParams)->asBstr();
    const cppbor::Map* unprotectedParams = coseSign1->get(kCoseSign1UnprotectedParams)->asMap();
    const cppbor::Bstr* payload = coseSign1->get(kCoseSign1Payload)->asBstr();
    const cppbor::Bstr* signature = coseSign1->get(kCoseSign1Signature)->asBstr();

    if (!protectedParams || !unprotectedParams || !payload || !signature) {
        return "Invalid COSE_Sign1";
    }

    auto [parsedProtParams, _, errMsg] = cppbor::parse(protectedParams);
    if (!parsedProtParams) {
        return errMsg + " when parsing protected params.";
    }
    if (!parsedProtParams->asMap()) {
        return "Protected params must be a map";
    }

    auto& algorithm = parsedProtParams->asMap()->get(ALGORITHM);
    if (!algorithm || !algorithm->asInt() ||
        (algorithm->asInt()->value() != EDDSA && algorithm->asInt()->value() != ES256)) {
        return "Unsupported signature algorithm";
    }

    auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(payload);
    if (!parsedPayload) return payloadErrMsg + " when parsing key";
    if (!parsedPayload->asMap()) return "CWT must be a map";
    auto serializedKey = validatePayloadAndFetchPubKey(parsedPayload->asMap());
    if (!serializedKey) {
        return "CWT validation failed: " + serializedKey.moveMessage();
    }

    bool selfSigned = signingCoseKey.empty();
    bytevec signatureInput =
        cppbor::Array().add("Signature1").add(*protectedParams).add(aad).add(*payload).encode();

    if (algorithm->asInt()->value() == EDDSA) {
        auto key = CoseKey::parseEd25519(selfSigned ? *serializedKey : signingCoseKey);

        if (!key) return "Bad signing key: " + key.moveMessage();

        if (!ED25519_verify(signatureInput.data(), signatureInput.size(), signature->value().data(),
                            key->getBstrValue(CoseKey::PUBKEY_X)->data())) {
            return "Signature verification failed";
        }
    } else {  // P256
        auto key = CoseKey::parseP256(selfSigned ? *serializedKey : signingCoseKey);
        if (!key || key->getBstrValue(CoseKey::PUBKEY_X)->empty() ||
            key->getBstrValue(CoseKey::PUBKEY_Y)->empty()) {
            return "Bad signing key: " + key.moveMessage();
        }
        auto publicKey = key->getEcPublicKey();
        if (!publicKey) return publicKey.moveMessage();

        auto ecdsaDerSignature = ecdsaCoseSignatureToDer(signature->value());
        if (!ecdsaDerSignature) return ecdsaDerSignature.moveMessage();

        // convert public key to uncompressed form.
        publicKey->insert(publicKey->begin(), 0x04);

        if (!verifyEcdsaDigest(publicKey.moveValue(), sha256(signatureInput), *ecdsaDerSignature)) {
            return "Signature verification failed";
        }
    }

    return serializedKey.moveValue();
}

ErrMsgOr<std::vector<BccEntryData>> validateBcc(const cppbor::Array* bcc) {
    if (!bcc || bcc->size() == 0) return "Invalid BCC";

    std::vector<BccEntryData> result;

    const auto& devicePubKey = bcc->get(0);
    if (!devicePubKey->asMap()) return "Invalid device public key at the 1st entry in the BCC";

    bytevec prevKey;

    for (size_t i = 1; i < bcc->size(); ++i) {
        const cppbor::Array* entry = bcc->get(i)->asArray();
        if (!entry || entry->size() != kCoseSign1EntryCount) {
            return "Invalid BCC entry " + std::to_string(i) + ": " + prettyPrint(entry);
        }
        auto payload = verifyAndParseCoseSign1Cwt(entry, std::move(prevKey), bytevec{} /* AAD */);
        if (!payload) {
            return "Failed to verify entry " + std::to_string(i) + ": " + payload.moveMessage();
        }

        auto& certProtParms = entry->get(kCoseSign1ProtectedParams);
        if (!certProtParms || !certProtParms->asBstr()) return "Invalid prot params";
        auto [parsedProtParms, _, errMsg] = cppbor::parse(certProtParms->asBstr()->value());
        if (!parsedProtParms || !parsedProtParms->asMap()) return "Invalid prot params";

        result.push_back(BccEntryData{*payload});

        // This entry's public key is the signing key for the next entry.
        prevKey = payload.moveValue();
        if (i == 1) {
            auto [parsedRootKey, _, errMsg] = cppbor::parse(prevKey);
            if (!parsedRootKey || !parsedRootKey->asMap()) return "Invalid payload entry in BCC.";
            if (*parsedRootKey != *devicePubKey) {
                return "Device public key doesn't match BCC root.";
            }
        }
    }

    return result;
}

JsonOutput jsonEncodeCsrWithBuild(const std::string instance_name, const cppbor::Array& csr) {
    const std::string kFingerprintProp = "ro.build.fingerprint";

    if (!::android::base::WaitForPropertyCreation(kFingerprintProp)) {
        return JsonOutput::Error("Unable to read build fingerprint");
    }

    bytevec csrCbor = csr.encode();
    size_t base64Length;
    int rc = EVP_EncodedLength(&base64Length, csrCbor.size());
    if (!rc) {
        return JsonOutput::Error("Error getting base64 length. Size overflow?");
    }

    std::vector<char> base64(base64Length);
    rc = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(base64.data()), csrCbor.data(), csrCbor.size());
    ++rc;  // Account for NUL, which BoringSSL does not for some reason.
    if (rc != base64Length) {
        return JsonOutput::Error("Error writing base64. Expected " + std::to_string(base64Length) +
                                 " bytes to be written, but " + std::to_string(rc) +
                                 " bytes were actually written.");
    }

    Json::Value json(Json::objectValue);
    json["name"] = instance_name;
    json["build_fingerprint"] = ::android::base::GetProperty(kFingerprintProp, /*default=*/"");
    json["csr"] = base64.data();  // Boring writes a NUL-terminated c-string

    Json::StreamWriterBuilder factory;
    factory["indentation"] = "";  // disable pretty formatting
    return JsonOutput::Ok(Json::writeString(factory, json));
}

std::string checkMapEntry(bool isFactory, const cppbor::Map& devInfo, cppbor::MajorType majorType,
                          const std::string& entryName) {
    const std::unique_ptr<cppbor::Item>& val = devInfo.get(entryName);
    if (!val) {
        return entryName + " is missing.\n";
    }
    if (val->type() != majorType) {
        return entryName + " has the wrong type.\n";
    }
    if (isFactory) {
        return "";
    }
    switch (majorType) {
        case cppbor::TSTR:
            if (val->asTstr()->value().size() <= 0) {
                return entryName + " is present but the value is empty.\n";
            }
            break;
        case cppbor::BSTR:
            if (val->asBstr()->value().size() <= 0) {
                return entryName + " is present but the value is empty.\n";
            }
            break;
        default:
            break;
    }
    return "";
}

std::string checkMapEntry(bool isFactory, const cppbor::Map& devInfo, cppbor::MajorType majorType,
                          const std::string& entryName, const cppbor::Array& allowList) {
    std::string error = checkMapEntry(isFactory, devInfo, majorType, entryName);
    if (!error.empty()) {
        return error;
    }

    if (isFactory) {
        return "";
    }

    const std::unique_ptr<cppbor::Item>& val = devInfo.get(entryName);
    for (auto i = allowList.begin(); i != allowList.end(); ++i) {
        if (**i == *val) {
            return "";
        }
    }
    return entryName + " has an invalid value.\n";
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable,
        bool isFactory) {
    const cppbor::Array kValidVbStates = {"green", "yellow", "orange"};
    const cppbor::Array kValidBootloaderStates = {"locked", "unlocked"};
    const cppbor::Array kValidSecurityLevels = {"tee", "strongbox"};
    const cppbor::Array kValidAttIdStates = {"locked", "open"};
    const cppbor::Array kValidFused = {0, 1};

    struct AttestationIdEntry {
        const char* id;
        bool alwaysValidate;
    };
    constexpr AttestationIdEntry kAttestationIdEntrySet[] = {{"brand", false},
                                                             {"manufacturer", true},
                                                             {"product", false},
                                                             {"model", false},
                                                             {"device", false}};

    auto [parsedVerifiedDeviceInfo, ignore1, errMsg] = cppbor::parse(deviceInfoBytes);
    if (!parsedVerifiedDeviceInfo) {
        return errMsg;
    }

    std::unique_ptr<cppbor::Map> parsed(parsedVerifiedDeviceInfo->asMap());
    if (!parsed) {
        return "DeviceInfo must be a CBOR map.";
    }
    parsedVerifiedDeviceInfo.release();

    if (parsed->clone()->asMap()->canonicalize().encode() != deviceInfoBytes) {
        return "DeviceInfo ordering is non-canonical.";
    }
    const std::unique_ptr<cppbor::Item>& version = parsed->get("version");
    if (!version) {
        return "Device info is missing version";
    }
    if (!version->asUint()) {
        return "version must be an unsigned integer";
    }
    RpcHardwareInfo info;
    provisionable->getHardwareInfo(&info);
    if (version->asUint()->value() != info.versionNumber) {
        return "DeviceInfo version (" + std::to_string(version->asUint()->value()) +
               ") does not match the remotely provisioned component version (" +
               std::to_string(info.versionNumber) + ").";
    }
    std::string error;
    switch (version->asUint()->value()) {
        case 2:
            for (const auto& entry : kAttestationIdEntrySet) {
                error += checkMapEntry(isFactory && !entry.alwaysValidate, *parsed, cppbor::TSTR,
                                       entry.id);
            }
            if (!error.empty()) {
                return error +
                       "Attestation IDs are missing or malprovisioned. If this test is being\n"
                       "run against an early proto or EVT build, this error is probably WAI\n"
                       "and indicates that Device IDs were not provisioned in the factory. If\n"
                       "this error is returned on a DVT or later build revision, then\n"
                       "something is likely wrong with the factory provisioning process.";
            }
            // TODO: Refactor the KeyMint code that validates these fields and include it here.
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "vb_state", kValidVbStates);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "bootloader_state",
                                   kValidBootloaderStates);
            error += checkMapEntry(isFactory, *parsed, cppbor::BSTR, "vbmeta_digest");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "system_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "boot_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "vendor_patch_level");
            error += checkMapEntry(isFactory, *parsed, cppbor::UINT, "fused", kValidFused);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "security_level",
                                   kValidSecurityLevels);
            if (parsed->get("security_level") && parsed->get("security_level")->asTstr() &&
                parsed->get("security_level")->asTstr()->value() == "tee") {
                error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "os_version");
            }
            break;
        case 1:
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "security_level",
                                   kValidSecurityLevels);
            error += checkMapEntry(isFactory, *parsed, cppbor::TSTR, "att_id_state",
                                   kValidAttIdStates);
            break;
        default:
            return "Unrecognized version: " + std::to_string(version->asUint()->value());
    }

    if (!error.empty()) {
        return error;
    }

    return std::move(parsed);
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateFactoryDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable) {
    return parseAndValidateDeviceInfo(deviceInfoBytes, provisionable, /*isFactory=*/true);
}

ErrMsgOr<std::unique_ptr<cppbor::Map>> parseAndValidateProductionDeviceInfo(
        const std::vector<uint8_t>& deviceInfoBytes, IRemotelyProvisionedComponent* provisionable) {
    return parseAndValidateDeviceInfo(deviceInfoBytes, provisionable, /*isFactory=*/false);
}

ErrMsgOr<bytevec> getSessionKey(ErrMsgOr<std::pair<bytevec, bytevec>>& senderPubkey,
                                const EekChain& eekChain, int32_t supportedEekCurve) {
    if (supportedEekCurve == RpcHardwareInfo::CURVE_25519 ||
        supportedEekCurve == RpcHardwareInfo::CURVE_NONE) {
        return x25519_HKDF_DeriveKey(eekChain.last_pubkey, eekChain.last_privkey,
                                     senderPubkey->first, false /* senderIsA */);
    } else {
        return ECDH_HKDF_DeriveKey(eekChain.last_pubkey, eekChain.last_privkey, senderPubkey->first,
                                   false /* senderIsA */);
    }
}

ErrMsgOr<std::vector<BccEntryData>> verifyProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::vector<uint8_t>& challenge,
        bool isFactory) {
    auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData.protectedData);
    if (!parsedProtectedData) {
        return protDataErrMsg;
    }
    if (!parsedProtectedData->asArray()) {
        return "Protected data is not a CBOR array.";
    }
    if (parsedProtectedData->asArray()->size() != kCoseEncryptEntryCount) {
        return "The protected data COSE_encrypt structure must have " +
               std::to_string(kCoseEncryptEntryCount) + " entries, but it only has " +
               std::to_string(parsedProtectedData->asArray()->size());
    }

    auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
    if (!senderPubkey) {
        return senderPubkey.message();
    }
    if (senderPubkey->second != eekId) {
        return "The COSE_encrypt recipient does not match the expected EEK identifier";
    }

    auto sessionKey = getSessionKey(senderPubkey, eekChain, supportedEekCurve);
    if (!sessionKey) {
        return sessionKey.message();
    }

    auto protectedDataPayload =
            decryptCoseEncrypt(*sessionKey, parsedProtectedData.get(), bytevec{} /* aad */);
    if (!protectedDataPayload) {
        return protectedDataPayload.message();
    }

    auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(*protectedDataPayload);
    if (!parsedPayload) {
        return "Failed to parse payload: " + payloadErrMsg;
    }
    if (!parsedPayload->asArray()) {
        return "The protected data payload must be an Array.";
    }
    if (parsedPayload->asArray()->size() != 3U && parsedPayload->asArray()->size() != 2U) {
        return "The protected data payload must contain SignedMAC and BCC. It may optionally "
               "contain AdditionalDKSignatures. However, the parsed payload has " +
               std::to_string(parsedPayload->asArray()->size()) + " entries.";
    }

    auto& signedMac = parsedPayload->asArray()->get(0);
    auto& bcc = parsedPayload->asArray()->get(1);
    if (!signedMac->asArray()) {
        return "The SignedMAC in the protected data payload is not an Array.";
    }
    if (!bcc->asArray()) {
        return "The BCC in the protected data payload is not an Array.";
    }

    // BCC is [ pubkey, + BccEntry]
    auto bccContents = validateBcc(bcc->asArray());
    if (!bccContents) {
        return bccContents.message() + "\n" + prettyPrint(bcc.get());
    }
    if (bccContents->size() == 0U) {
        return "The BCC is empty. It must contain at least one entry.";
    }

    auto deviceInfoResult =
            parseAndValidateDeviceInfo(deviceInfo.deviceInfo, provisionable, isFactory);
    if (!deviceInfoResult) {
        return deviceInfoResult.message();
    }
    std::unique_ptr<cppbor::Map> deviceInfoMap = deviceInfoResult.moveValue();
    auto& signingKey = bccContents->back().pubKey;
    auto macKey = verifyAndParseCoseSign1(signedMac->asArray(), signingKey,
                                          cppbor::Array()  // SignedMacAad
                                                  .add(challenge)
                                                  .add(std::move(deviceInfoMap))
                                                  .add(keysToSignMac)
                                                  .encode());
    if (!macKey) {
        return macKey.message();
    }

    auto coseMac0 = cppbor::Array()
                            .add(cppbor::Map()  // protected
                                         .add(ALGORITHM, HMAC_256)
                                         .canonicalize()
                                         .encode())
                            .add(cppbor::Map())        // unprotected
                            .add(keysToSign.encode())  // payload (keysToSign)
                            .add(keysToSignMac);       // tag

    auto macPayload = verifyAndParseCoseMac0(&coseMac0, *macKey);
    if (!macPayload) {
        return macPayload.message();
    }

    return *bccContents;
}

ErrMsgOr<std::vector<BccEntryData>> verifyFactoryProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::vector<uint8_t>& challenge) {
    return verifyProtectedData(deviceInfo, keysToSign, keysToSignMac, protectedData, eekChain,
                               eekId, supportedEekCurve, provisionable, challenge,
                               /*isFactory=*/true);
}

ErrMsgOr<std::vector<BccEntryData>> verifyProductionProtectedData(
        const DeviceInfo& deviceInfo, const cppbor::Array& keysToSign,
        const std::vector<uint8_t>& keysToSignMac, const ProtectedData& protectedData,
        const EekChain& eekChain, const std::vector<uint8_t>& eekId, int32_t supportedEekCurve,
        IRemotelyProvisionedComponent* provisionable, const std::vector<uint8_t>& challenge) {
    return verifyProtectedData(deviceInfo, keysToSign, keysToSignMac, protectedData, eekChain,
                               eekId, supportedEekCurve, provisionable, challenge,
                               /*isFactory=*/false);
}

}  // namespace aidl::android::hardware::security::keymint::remote_prov