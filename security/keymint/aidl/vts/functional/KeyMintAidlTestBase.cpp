/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "KeyMintAidlTestBase.h"

#include <chrono>
#include <fstream>
#include <unordered_set>
#include <vector>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/content/pm/IPackageManagerNative.h>
#include <cppbor_parse.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>
#include <openssl/evp.h>
#include <openssl/mem.h>
#include <remote_prov/remote_prov_utils.h>

#include <keymaster/cppcose/cppcose.h>
#include <keymint_support/key_param_output.h>
#include <keymint_support/keymint_utils.h>
#include <keymint_support/openssl_utils.h>

namespace aidl::android::hardware::security::keymint {

using namespace cppcose;
using namespace std::literals::chrono_literals;
using std::endl;
using std::optional;
using std::unique_ptr;
using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using ::testing::ElementsAreArray;
using ::testing::MatchesRegex;
using ::testing::Not;

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << ::std::endl;
    else {
        os << "\n";
        for (auto& entry : set) os << entry << ::std::endl;
    }
    return os;
}

namespace test {

namespace {

// Invalid value for a patchlevel (which is of form YYYYMMDD).
const uint32_t kInvalidPatchlevel = 99998877;

// Overhead for PKCS#1 v1.5 signature padding of undigested messages.  Digested messages have
// additional overhead, for the digest algorithmIdentifier required by PKCS#1.
const size_t kPkcs1UndigestedSignaturePaddingOverhead = 11;

typedef KeyMintAidlTestBase::KeyData KeyData;
// Predicate for testing basic characteristics validity in generation or import.
bool KeyCharacteristicsBasicallyValid(SecurityLevel secLevel,
                                      const vector<KeyCharacteristics>& key_characteristics) {
    if (key_characteristics.empty()) return false;

    std::unordered_set<SecurityLevel> levels_seen;
    for (auto& entry : key_characteristics) {
        if (entry.authorizations.empty()) {
            GTEST_LOG_(ERROR) << "empty authorizations for " << entry.securityLevel;
            return false;
        }

        // Just ignore the SecurityLevel::KEYSTORE as the KM won't do any enforcement on this.
        if (entry.securityLevel == SecurityLevel::KEYSTORE) continue;

        if (levels_seen.find(entry.securityLevel) != levels_seen.end()) {
            GTEST_LOG_(ERROR) << "duplicate authorizations for " << entry.securityLevel;
            return false;
        }
        levels_seen.insert(entry.securityLevel);

        // Generally, we should only have one entry, at the same security level as the KM
        // instance.  There is an exception: StrongBox KM can have some authorizations that are
        // enforced by the TEE.
        bool isExpectedSecurityLevel = secLevel == entry.securityLevel ||
                                       (secLevel == SecurityLevel::STRONGBOX &&
                                        entry.securityLevel == SecurityLevel::TRUSTED_ENVIRONMENT);

        if (!isExpectedSecurityLevel) {
            GTEST_LOG_(ERROR) << "Unexpected security level " << entry.securityLevel;
            return false;
        }
    }
    return true;
}

// Extract attestation record from cert. Returned object is still part of cert; don't free it
// separately.
ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    EXPECT_TRUE(!!oid.get());
    if (!oid.get()) return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    EXPECT_NE(-1, location) << "Attestation extension not found in certificate";
    if (location == -1) return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    EXPECT_TRUE(!!attest_rec_ext)
            << "Found attestation extension but couldn't retrieve it?  Probably a BoringSSL bug.";
    if (!attest_rec_ext) return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    EXPECT_TRUE(!!attest_rec) << "Attestation extension contained no data";
    return attest_rec;
}

void check_attestation_version(uint32_t attestation_version, int32_t aidl_version) {
    // Version numbers in attestation extensions should be a multiple of 100.
    EXPECT_EQ(attestation_version % 100, 0);

    // The multiplier should never be higher than the AIDL version, but can be less
    // (for example, if the implementation is from an earlier version but the HAL service
    // uses the default libraries and so reports the current AIDL version).
    EXPECT_TRUE((attestation_version / 100) <= aidl_version);
}

bool avb_verification_enabled() {
    char value[PROPERTY_VALUE_MAX];
    return property_get("ro.boot.vbmeta.device_state", value, "") != 0;
}

char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

// Attestations don't contain everything in key authorization lists, so we need to filter the key
// lists to produce the lists that we expect to match the attestations.
auto kTagsToFilter = {
        Tag::CREATION_DATETIME,
        Tag::HARDWARE_TYPE,
        Tag::INCLUDE_UNIQUE_ID,
};

AuthorizationSet filtered_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(
            set.begin(), set.end(), std::back_inserter(filtered), [](const auto& entry) -> bool {
                return std::find(kTagsToFilter.begin(), kTagsToFilter.end(), entry.tag) !=
                       kTagsToFilter.end();
            });
    return filtered;
}

// Remove any SecurityLevel::KEYSTORE entries from a list of key characteristics.
void strip_keystore_tags(vector<KeyCharacteristics>* characteristics) {
    characteristics->erase(std::remove_if(characteristics->begin(), characteristics->end(),
                                          [](const auto& entry) {
                                              return entry.securityLevel == SecurityLevel::KEYSTORE;
                                          }),
                           characteristics->end());
}

string x509NameToStr(X509_NAME* name) {
    char* s = X509_NAME_oneline(name, nullptr, 0);
    string retval(s);
    OPENSSL_free(s);
    return retval;
}

}  // namespace

bool KeyMintAidlTestBase::arm_deleteAllKeys = false;
bool KeyMintAidlTestBase::dump_Attestations = false;

uint32_t KeyMintAidlTestBase::boot_patch_level(
        const vector<KeyCharacteristics>& key_characteristics) {
    // The boot patchlevel is not available as a property, but should be present
    // in the key characteristics of any created key.
    AuthorizationSet allAuths;
    for (auto& entry : key_characteristics) {
        allAuths.push_back(AuthorizationSet(entry.authorizations));
    }
    auto patchlevel = allAuths.GetTagValue(TAG_BOOT_PATCHLEVEL);
    if (patchlevel.has_value()) {
        return patchlevel.value();
    } else {
        // No boot patchlevel is available. Return a value that won't match anything
        // and so will trigger test failures.
        return kInvalidPatchlevel;
    }
}

uint32_t KeyMintAidlTestBase::boot_patch_level() {
    return boot_patch_level(key_characteristics_);
}

/**
 * An API to determine device IDs attestation is required or not,
 * which is mandatory for KeyMint version 2 or first_api_level 33 or greater.
 */
bool KeyMintAidlTestBase::isDeviceIdAttestationRequired() {
    return AidlVersion() >= 2 || property_get_int32("ro.vendor.api_level", 0) >= 33;
}

bool KeyMintAidlTestBase::Curve25519Supported() {
    // Strongbox never supports curve 25519.
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        return false;
    }

    // Curve 25519 was included in version 2 of the KeyMint interface.
    int32_t version = 0;
    auto status = keymint_->getInterfaceVersion(&version);
    if (!status.isOk()) {
        ADD_FAILURE() << "Failed to determine interface version";
    }
    return version >= 2;
}

ErrorCode KeyMintAidlTestBase::GetReturnErrorCode(const Status& result) {
    if (result.isOk()) return ErrorCode::OK;

    if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
        return static_cast<ErrorCode>(result.getServiceSpecificError());
    }

    return ErrorCode::UNKNOWN_ERROR;
}

void KeyMintAidlTestBase::InitializeKeyMint(std::shared_ptr<IKeyMintDevice> keyMint) {
    ASSERT_NE(keyMint, nullptr);
    keymint_ = std::move(keyMint);

    KeyMintHardwareInfo info;
    ASSERT_TRUE(keymint_->getHardwareInfo(&info).isOk());

    securityLevel_ = info.securityLevel;
    name_.assign(info.keyMintName.begin(), info.keyMintName.end());
    author_.assign(info.keyMintAuthorName.begin(), info.keyMintAuthorName.end());
    timestamp_token_required_ = info.timestampTokenRequired;

    os_version_ = getOsVersion();
    os_patch_level_ = getOsPatchlevel();
    vendor_patch_level_ = getVendorPatchlevel();
}

int32_t KeyMintAidlTestBase::AidlVersion() {
    int32_t version = 0;
    auto status = keymint_->getInterfaceVersion(&version);
    if (!status.isOk()) {
        ADD_FAILURE() << "Failed to determine interface version";
    }
    return version;
}

void KeyMintAidlTestBase::SetUp() {
    if (AServiceManager_isDeclared(GetParam().c_str())) {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        InitializeKeyMint(IKeyMintDevice::fromBinder(binder));
    } else {
        InitializeKeyMint(nullptr);
    }
}

ErrorCode KeyMintAidlTestBase::GenerateKey(const AuthorizationSet& key_desc,
                                           const optional<AttestationKey>& attest_key,
                                           vector<uint8_t>* key_blob,
                                           vector<KeyCharacteristics>* key_characteristics,
                                           vector<Certificate>* cert_chain) {
    EXPECT_NE(key_blob, nullptr) << "Key blob pointer must not be null.  Test bug";
    EXPECT_NE(key_characteristics, nullptr)
            << "Previous characteristics not deleted before generating key.  Test bug.";

    KeyCreationResult creationResult;
    Status result = keymint_->generateKey(key_desc.vector_data(), attest_key, &creationResult);
    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);
        *key_blob = std::move(creationResult.keyBlob);
        *key_characteristics = std::move(creationResult.keyCharacteristics);
        *cert_chain = std::move(creationResult.certificateChain);

        auto algorithm = key_desc.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain->size(), 1);
            if (key_desc.Contains(TAG_ATTESTATION_CHALLENGE)) {
                if (attest_key) {
                    EXPECT_EQ(cert_chain->size(), 1);
                } else {
                    EXPECT_GT(cert_chain->size(), 1);
                }
            }
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain->size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::GenerateKey(const AuthorizationSet& key_desc,
                                           const optional<AttestationKey>& attest_key) {
    return GenerateKey(key_desc, attest_key, &key_blob_, &key_characteristics_, &cert_chain_);
}

ErrorCode KeyMintAidlTestBase::GenerateKeyWithSelfSignedAttestKey(
        const AuthorizationSet& attest_key_desc, const AuthorizationSet& key_desc,
        vector<uint8_t>* key_blob, vector<KeyCharacteristics>* key_characteristics,
        vector<Certificate>* cert_chain) {
    AttestationKey attest_key;
    vector<Certificate> attest_cert_chain;
    vector<KeyCharacteristics> attest_key_characteristics;
    // Generate a key with self signed attestation.
    auto error = GenerateKey(attest_key_desc, std::nullopt, &attest_key.keyBlob,
                             &attest_key_characteristics, &attest_cert_chain);
    if (error != ErrorCode::OK) {
        return error;
    }

    attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
    // Generate a key, by passing the above self signed attestation key as attest key.
    error = GenerateKey(key_desc, attest_key, key_blob, key_characteristics, cert_chain);
    if (error == ErrorCode::OK) {
        // Append the attest_cert_chain to the attested cert_chain to yield a valid cert chain.
        cert_chain->push_back(attest_cert_chain[0]);
    }
    return error;
}

ErrorCode KeyMintAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                         const string& key_material, vector<uint8_t>* key_blob,
                                         vector<KeyCharacteristics>* key_characteristics) {
    Status result;

    cert_chain_.clear();
    key_characteristics->clear();
    key_blob->clear();

    KeyCreationResult creationResult;
    result = keymint_->importKey(key_desc.vector_data(), format,
                                 vector<uint8_t>(key_material.begin(), key_material.end()),
                                 {} /* attestationSigningKeyBlob */, &creationResult);

    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);

        *key_blob = std::move(creationResult.keyBlob);
        *key_characteristics = std::move(creationResult.keyCharacteristics);
        cert_chain_ = std::move(creationResult.certificateChain);

        auto algorithm = key_desc.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain_.size(), 1);
            if (key_desc.Contains(TAG_ATTESTATION_CHALLENGE)) EXPECT_GT(cert_chain_.size(), 1);
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain_.size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                         const string& key_material) {
    return ImportKey(key_desc, format, key_material, &key_blob_, &key_characteristics_);
}

ErrorCode KeyMintAidlTestBase::ImportWrappedKey(string wrapped_key, string wrapping_key,
                                                const AuthorizationSet& wrapping_key_desc,
                                                string masking_key,
                                                const AuthorizationSet& unwrapping_params,
                                                int64_t password_sid, int64_t biometric_sid) {
    EXPECT_EQ(ErrorCode::OK, ImportKey(wrapping_key_desc, KeyFormat::PKCS8, wrapping_key));

    key_characteristics_.clear();

    KeyCreationResult creationResult;
    Status result = keymint_->importWrappedKey(
            vector<uint8_t>(wrapped_key.begin(), wrapped_key.end()), key_blob_,
            vector<uint8_t>(masking_key.begin(), masking_key.end()),
            unwrapping_params.vector_data(), password_sid, biometric_sid, &creationResult);

    if (result.isOk()) {
        EXPECT_PRED2(KeyCharacteristicsBasicallyValid, SecLevel(),
                     creationResult.keyCharacteristics);
        EXPECT_GT(creationResult.keyBlob.size(), 0);

        key_blob_ = std::move(creationResult.keyBlob);
        key_characteristics_ = std::move(creationResult.keyCharacteristics);
        cert_chain_ = std::move(creationResult.certificateChain);

        AuthorizationSet allAuths;
        for (auto& entry : key_characteristics_) {
            allAuths.push_back(AuthorizationSet(entry.authorizations));
        }
        auto algorithm = allAuths.GetTagValue(TAG_ALGORITHM);
        EXPECT_TRUE(algorithm);
        if (algorithm &&
            (algorithm.value() == Algorithm::RSA || algorithm.value() == Algorithm::EC)) {
            EXPECT_GE(cert_chain_.size(), 1);
        } else {
            // For symmetric keys there should be no certificates.
            EXPECT_EQ(cert_chain_.size(), 0);
        }
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::GetCharacteristics(const vector<uint8_t>& key_blob,
                                                  const vector<uint8_t>& app_id,
                                                  const vector<uint8_t>& app_data,
                                                  vector<KeyCharacteristics>* key_characteristics) {
    Status result =
            keymint_->getKeyCharacteristics(key_blob, app_id, app_data, key_characteristics);
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::GetCharacteristics(const vector<uint8_t>& key_blob,
                                                  vector<KeyCharacteristics>* key_characteristics) {
    vector<uint8_t> empty_app_id, empty_app_data;
    return GetCharacteristics(key_blob, empty_app_id, empty_app_data, key_characteristics);
}

void KeyMintAidlTestBase::CheckCharacteristics(
        const vector<uint8_t>& key_blob,
        const vector<KeyCharacteristics>& generate_characteristics) {
    // Any key characteristics that were in SecurityLevel::KEYSTORE when returned from
    // generateKey() should be excluded, as KeyMint will have no record of them.
    // This applies to CREATION_DATETIME in particular.
    vector<KeyCharacteristics> expected_characteristics(generate_characteristics);
    strip_keystore_tags(&expected_characteristics);

    vector<KeyCharacteristics> retrieved;
    ASSERT_EQ(ErrorCode::OK, GetCharacteristics(key_blob, &retrieved));
    EXPECT_EQ(expected_characteristics, retrieved);
}

void KeyMintAidlTestBase::CheckAppIdCharacteristics(
        const vector<uint8_t>& key_blob, std::string_view app_id_string,
        std::string_view app_data_string,
        const vector<KeyCharacteristics>& generate_characteristics) {
    // Exclude any SecurityLevel::KEYSTORE characteristics for comparisons.
    vector<KeyCharacteristics> expected_characteristics(generate_characteristics);
    strip_keystore_tags(&expected_characteristics);

    vector<uint8_t> app_id(app_id_string.begin(), app_id_string.end());
    vector<uint8_t> app_data(app_data_string.begin(), app_data_string.end());
    vector<KeyCharacteristics> retrieved;
    ASSERT_EQ(ErrorCode::OK, GetCharacteristics(key_blob, app_id, app_data, &retrieved));
    EXPECT_EQ(expected_characteristics, retrieved);

    // Check that key characteristics can't be retrieved if the app ID or app data is missing.
    vector<uint8_t> empty;
    vector<KeyCharacteristics> not_retrieved;
    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              GetCharacteristics(key_blob, empty, app_data, &not_retrieved));
    EXPECT_EQ(not_retrieved.size(), 0);

    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              GetCharacteristics(key_blob, app_id, empty, &not_retrieved));
    EXPECT_EQ(not_retrieved.size(), 0);

    EXPECT_EQ(ErrorCode::INVALID_KEY_BLOB,
              GetCharacteristics(key_blob, empty, empty, &not_retrieved));
    EXPECT_EQ(not_retrieved.size(), 0);
}

ErrorCode KeyMintAidlTestBase::DeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    Status result = keymint_->deleteKey(*key_blob);
    if (!keep_key_blob) {
        *key_blob = vector<uint8_t>();
    }

    EXPECT_TRUE(result.isOk()) << result.getServiceSpecificError() << endl;
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::DeleteKey(bool keep_key_blob) {
    return DeleteKey(&key_blob_, keep_key_blob);
}

ErrorCode KeyMintAidlTestBase::DeleteAllKeys() {
    Status result = keymint_->deleteAllKeys();
    EXPECT_TRUE(result.isOk()) << result.getServiceSpecificError() << endl;
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::DestroyAttestationIds() {
    Status result = keymint_->destroyAttestationIds();
    return GetReturnErrorCode(result);
}

void KeyMintAidlTestBase::CheckedDeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    ErrorCode result = DeleteKey(key_blob, keep_key_blob);
    EXPECT_TRUE(result == ErrorCode::OK || result == ErrorCode::UNIMPLEMENTED) << result << endl;
}

void KeyMintAidlTestBase::CheckedDeleteKey() {
    CheckedDeleteKey(&key_blob_);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                     const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params,
                                     std::shared_ptr<IKeyMintOperation>& op) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;
    result = keymint_->begin(purpose, key_blob, in_params.vector_data(), std::nullopt, &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op = out.operation;
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                     const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;

    result = keymint_->begin(purpose, key_blob, in_params.vector_data(), std::nullopt, &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op_ = out.operation;
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                                     AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    EXPECT_EQ(nullptr, op_);
    return Begin(purpose, key_blob_, in_params, out_params);
}

ErrorCode KeyMintAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params) {
    SCOPED_TRACE("Begin");
    AuthorizationSet out_params;
    ErrorCode result = Begin(purpose, in_params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return result;
}

ErrorCode KeyMintAidlTestBase::UpdateAad(const string& input) {
    return GetReturnErrorCode(op_->updateAad(vector<uint8_t>(input.begin(), input.end()),
                                             {} /* hardwareAuthToken */,
                                             {} /* verificationToken */));
}

ErrorCode KeyMintAidlTestBase::Update(const string& input, string* output) {
    SCOPED_TRACE("Update");

    Status result;
    if (!output) return ErrorCode::UNEXPECTED_NULL_POINTER;

    EXPECT_NE(op_, nullptr);
    if (!op_) return ErrorCode::UNEXPECTED_NULL_POINTER;

    std::vector<uint8_t> o_put;
    result = op_->update(vector<uint8_t>(input.begin(), input.end()), {}, {}, &o_put);

    if (result.isOk()) {
        output->append(o_put.begin(), o_put.end());
    } else {
        // Failure always terminates the operation.
        op_ = {};
    }

    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Finish(const string& input, const string& signature,
                                      string* output) {
    SCOPED_TRACE("Finish");
    Status result;

    EXPECT_NE(op_, nullptr);
    if (!op_) return ErrorCode::UNEXPECTED_NULL_POINTER;

    vector<uint8_t> oPut;
    result = op_->finish(vector<uint8_t>(input.begin(), input.end()),
                         vector<uint8_t>(signature.begin(), signature.end()), {} /* authToken */,
                         {} /* timestampToken */, {} /* confirmationToken */, &oPut);

    if (result.isOk()) output->append(oPut.begin(), oPut.end());

    op_ = {};
    return GetReturnErrorCode(result);
}

ErrorCode KeyMintAidlTestBase::Abort(const std::shared_ptr<IKeyMintOperation>& op) {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op, nullptr);
    if (!op) return ErrorCode::UNEXPECTED_NULL_POINTER;

    Status retval = op->abort();
    EXPECT_TRUE(retval.isOk());
    return static_cast<ErrorCode>(retval.getServiceSpecificError());
}

ErrorCode KeyMintAidlTestBase::Abort() {
    SCOPED_TRACE("Abort");

    EXPECT_NE(op_, nullptr);
    if (!op_) return ErrorCode::UNEXPECTED_NULL_POINTER;

    Status retval = op_->abort();
    return static_cast<ErrorCode>(retval.getServiceSpecificError());
}

void KeyMintAidlTestBase::AbortIfNeeded() {
    SCOPED_TRACE("AbortIfNeeded");
    if (op_) {
        EXPECT_EQ(ErrorCode::OK, Abort());
        op_.reset();
    }
}

auto KeyMintAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                         const string& message, const AuthorizationSet& in_params)
        -> std::tuple<ErrorCode, string> {
    AuthorizationSet begin_out_params;
    ErrorCode result = Begin(operation, key_blob, in_params, &begin_out_params);
    if (result != ErrorCode::OK) return {result, {}};

    string output;
    return {Finish(message, &output), output};
}

string KeyMintAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                           const string& message, const AuthorizationSet& in_params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("ProcessMessage");
    AuthorizationSet begin_out_params;
    ErrorCode result = Begin(operation, key_blob, in_params, out_params);
    EXPECT_EQ(ErrorCode::OK, result);
    if (result != ErrorCode::OK) {
        return "";
    }

    string output;
    EXPECT_EQ(ErrorCode::OK, Finish(message, &output));
    return output;
}

string KeyMintAidlTestBase::SignMessage(const vector<uint8_t>& key_blob, const string& message,
                                        const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet out_params;
    string signature = ProcessMessage(key_blob, KeyPurpose::SIGN, message, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return signature;
}

string KeyMintAidlTestBase::SignMessage(const string& message, const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    return SignMessage(key_blob_, message, params);
}

string KeyMintAidlTestBase::MacMessage(const string& message, Digest digest, size_t mac_length) {
    SCOPED_TRACE("MacMessage");
    return SignMessage(
            key_blob_, message,
            AuthorizationSetBuilder().Digest(digest).Authorization(TAG_MAC_LENGTH, mac_length));
}

void KeyMintAidlTestBase::CheckAesIncrementalEncryptOperation(BlockMode block_mode,
                                                              int message_size) {
    auto builder = AuthorizationSetBuilder()
                           .Authorization(TAG_NO_AUTH_REQUIRED)
                           .AesEncryptionKey(128)
                           .BlockMode(block_mode)
                           .Padding(PaddingMode::NONE);
    if (block_mode == BlockMode::GCM) {
        builder.Authorization(TAG_MIN_MAC_LENGTH, 128);
    }
    ASSERT_EQ(ErrorCode::OK, GenerateKey(builder));

    for (int increment = 1; increment <= message_size; ++increment) {
        string message(message_size, 'a');
        auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(PaddingMode::NONE);
        if (block_mode == BlockMode::GCM) {
            params.Authorization(TAG_MAC_LENGTH, 128) /* for GCM */;
        }

        AuthorizationSet output_params;
        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, params, &output_params));

        string ciphertext;
        string to_send;
        for (size_t i = 0; i < message.size(); i += increment) {
            EXPECT_EQ(ErrorCode::OK, Update(message.substr(i, increment), &ciphertext));
        }
        EXPECT_EQ(ErrorCode::OK, Finish(to_send, &ciphertext))
                << "Error sending " << to_send << " with block mode " << block_mode;

        switch (block_mode) {
            case BlockMode::GCM:
                EXPECT_EQ(message.size() + 16, ciphertext.size());
                break;
            case BlockMode::CTR:
                EXPECT_EQ(message.size(), ciphertext.size());
                break;
            case BlockMode::CBC:
            case BlockMode::ECB:
                EXPECT_EQ(message.size() + message.size() % 16, ciphertext.size());
                break;
        }

        auto iv = output_params.GetTagValue(TAG_NONCE);
        switch (block_mode) {
            case BlockMode::CBC:
            case BlockMode::GCM:
            case BlockMode::CTR:
                ASSERT_TRUE(iv) << "No IV for block mode " << block_mode;
                EXPECT_EQ(block_mode == BlockMode::GCM ? 12U : 16U, iv->get().size());
                params.push_back(TAG_NONCE, iv->get());
                break;

            case BlockMode::ECB:
                EXPECT_FALSE(iv) << "ECB mode should not generate IV";
                break;
        }

        EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params))
                << "Decrypt begin() failed for block mode " << block_mode;

        string plaintext;
        for (size_t i = 0; i < ciphertext.size(); i += increment) {
            EXPECT_EQ(ErrorCode::OK, Update(ciphertext.substr(i, increment), &plaintext));
        }
        ErrorCode error = Finish(to_send, &plaintext);
        ASSERT_EQ(ErrorCode::OK, error) << "Decryption failed for block mode " << block_mode
                                        << " and increment " << increment;
        if (error == ErrorCode::OK) {
            ASSERT_EQ(message, plaintext) << "Decryption didn't match for block mode " << block_mode
                                          << " and increment " << increment;
        }
    }
}

void KeyMintAidlTestBase::CheckHmacTestVector(const string& key, const string& message,
                                              Digest digest, const string& expected_mac) {
    SCOPED_TRACE("CheckHmacTestVector");
    ASSERT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .HmacKey(key.size() * 8)
                                .Authorization(TAG_MIN_MAC_LENGTH, expected_mac.size() * 8)
                                .Digest(digest),
                        KeyFormat::RAW, key));
    string signature = MacMessage(message, digest, expected_mac.size() * 8);
    EXPECT_EQ(expected_mac, signature)
            << "Test vector didn't match for key of size " << key.size() << " message of size "
            << message.size() << " and digest " << digest;
    CheckedDeleteKey();
}

void KeyMintAidlTestBase::CheckAesCtrTestVector(const string& key, const string& nonce,
                                                const string& message,
                                                const string& expected_ciphertext) {
    SCOPED_TRACE("CheckAesCtrTestVector");
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .AesEncryptionKey(key.size() * 8)
                                               .BlockMode(BlockMode::CTR)
                                               .Authorization(TAG_CALLER_NONCE)
                                               .Padding(PaddingMode::NONE),
                                       KeyFormat::RAW, key));

    auto params = AuthorizationSetBuilder()
                          .Authorization(TAG_NONCE, nonce.data(), nonce.size())
                          .BlockMode(BlockMode::CTR)
                          .Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(key_blob_, message, params, &out_params);
    EXPECT_EQ(expected_ciphertext, ciphertext);
}

void KeyMintAidlTestBase::CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                                   PaddingMode padding_mode, const string& key,
                                                   const string& iv, const string& input,
                                                   const string& expected_output) {
    auto authset = AuthorizationSetBuilder()
                           .TripleDesEncryptionKey(key.size() * 7)
                           .BlockMode(block_mode)
                           .Authorization(TAG_NO_AUTH_REQUIRED)
                           .Padding(padding_mode);
    if (iv.size()) authset.Authorization(TAG_CALLER_NONCE);
    ASSERT_EQ(ErrorCode::OK, ImportKey(authset, KeyFormat::RAW, key));
    ASSERT_GT(key_blob_.size(), 0U);

    auto begin_params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding_mode);
    if (iv.size()) begin_params.Authorization(TAG_NONCE, iv.data(), iv.size());
    AuthorizationSet output_params;
    string output = ProcessMessage(key_blob_, purpose, input, begin_params, &output_params);
    EXPECT_EQ(expected_output, output);
}

void KeyMintAidlTestBase::VerifyMessage(const vector<uint8_t>& key_blob, const string& message,
                                        const string& signature, const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::VERIFY, key_blob, params, &begin_out_params));

    string output;
    EXPECT_EQ(ErrorCode::OK, Finish(message, signature, &output));
    EXPECT_TRUE(output.empty());
    op_ = {};
}

void KeyMintAidlTestBase::VerifyMessage(const string& message, const string& signature,
                                        const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    VerifyMessage(key_blob_, message, signature, params);
}

void KeyMintAidlTestBase::LocalVerifyMessage(const string& message, const string& signature,
                                             const AuthorizationSet& params) {
    SCOPED_TRACE("LocalVerifyMessage");

    // Retrieve the public key from the leaf certificate.
    ASSERT_GT(cert_chain_.size(), 0);
    X509_Ptr key_cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
    ASSERT_TRUE(key_cert.get());
    EVP_PKEY_Ptr pub_key(X509_get_pubkey(key_cert.get()));
    ASSERT_TRUE(pub_key.get());

    Digest digest = params.GetTagValue(TAG_DIGEST).value();
    PaddingMode padding = PaddingMode::NONE;
    auto tag = params.GetTagValue(TAG_PADDING);
    if (tag.has_value()) {
        padding = tag.value();
    }

    if (digest == Digest::NONE) {
        switch (EVP_PKEY_id(pub_key.get())) {
            case EVP_PKEY_ED25519: {
                ASSERT_EQ(64, signature.size());
                uint8_t pub_keydata[32];
                size_t pub_len = sizeof(pub_keydata);
                ASSERT_EQ(1, EVP_PKEY_get_raw_public_key(pub_key.get(), pub_keydata, &pub_len));
                ASSERT_EQ(sizeof(pub_keydata), pub_len);
                ASSERT_EQ(1, ED25519_verify(reinterpret_cast<const uint8_t*>(message.data()),
                                            message.size(),
                                            reinterpret_cast<const uint8_t*>(signature.data()),
                                            pub_keydata));
                break;
            }

            case EVP_PKEY_EC: {
                vector<uint8_t> data((EVP_PKEY_bits(pub_key.get()) + 7) / 8);
                size_t data_size = std::min(data.size(), message.size());
                memcpy(data.data(), message.data(), data_size);
                EC_KEY_Ptr ecdsa(EVP_PKEY_get1_EC_KEY(pub_key.get()));
                ASSERT_TRUE(ecdsa.get());
                ASSERT_EQ(1,
                          ECDSA_verify(0, reinterpret_cast<const uint8_t*>(data.data()), data_size,
                                       reinterpret_cast<const uint8_t*>(signature.data()),
                                       signature.size(), ecdsa.get()));
                break;
            }
            case EVP_PKEY_RSA: {
                vector<uint8_t> data(EVP_PKEY_size(pub_key.get()));
                size_t data_size = std::min(data.size(), message.size());
                memcpy(data.data(), message.data(), data_size);

                RSA_Ptr rsa(EVP_PKEY_get1_RSA(const_cast<EVP_PKEY*>(pub_key.get())));
                ASSERT_TRUE(rsa.get());

                size_t key_len = RSA_size(rsa.get());
                int openssl_padding = RSA_NO_PADDING;
                switch (padding) {
                    case PaddingMode::NONE:
                        ASSERT_TRUE(data_size <= key_len);
                        ASSERT_EQ(key_len, signature.size());
                        openssl_padding = RSA_NO_PADDING;
                        break;
                    case PaddingMode::RSA_PKCS1_1_5_SIGN:
                        ASSERT_TRUE(data_size + kPkcs1UndigestedSignaturePaddingOverhead <=
                                    key_len);
                        openssl_padding = RSA_PKCS1_PADDING;
                        break;
                    default:
                        ADD_FAILURE() << "Unsupported RSA padding mode " << padding;
                }

                vector<uint8_t> decrypted_data(key_len);
                int bytes_decrypted = RSA_public_decrypt(
                        signature.size(), reinterpret_cast<const uint8_t*>(signature.data()),
                        decrypted_data.data(), rsa.get(), openssl_padding);
                ASSERT_GE(bytes_decrypted, 0);

                const uint8_t* compare_pos = decrypted_data.data();
                size_t bytes_to_compare = bytes_decrypted;
                uint8_t zero_check_result = 0;
                if (padding == PaddingMode::NONE && data_size < bytes_to_compare) {
                    // If the data is short, for "unpadded" signing we zero-pad to the left.  So
                    // during verification we should have zeros on the left of the decrypted data.
                    // Do a constant-time check.
                    const uint8_t* zero_end = compare_pos + bytes_to_compare - data_size;
                    while (compare_pos < zero_end) zero_check_result |= *compare_pos++;
                    ASSERT_EQ(0, zero_check_result);
                    bytes_to_compare = data_size;
                }
                ASSERT_EQ(0, memcmp(compare_pos, data.data(), bytes_to_compare));
                break;
            }
            default:
                ADD_FAILURE() << "Unknown public key type";
        }
    } else {
        EVP_MD_CTX digest_ctx;
        EVP_MD_CTX_init(&digest_ctx);
        EVP_PKEY_CTX* pkey_ctx;
        const EVP_MD* md = openssl_digest(digest);
        ASSERT_NE(md, nullptr);
        ASSERT_EQ(1, EVP_DigestVerifyInit(&digest_ctx, &pkey_ctx, md, nullptr, pub_key.get()));

        if (padding == PaddingMode::RSA_PSS) {
            EXPECT_GT(EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING), 0);
            EXPECT_GT(EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, EVP_MD_size(md)), 0);
            EXPECT_GT(EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, md), 0);
        }

        ASSERT_EQ(1, EVP_DigestVerifyUpdate(&digest_ctx,
                                            reinterpret_cast<const uint8_t*>(message.data()),
                                            message.size()));
        ASSERT_EQ(1, EVP_DigestVerifyFinal(&digest_ctx,
                                           reinterpret_cast<const uint8_t*>(signature.data()),
                                           signature.size()));
        EVP_MD_CTX_cleanup(&digest_ctx);
    }
}

string KeyMintAidlTestBase::LocalRsaEncryptMessage(const string& message,
                                                   const AuthorizationSet& params) {
    SCOPED_TRACE("LocalRsaEncryptMessage");

    // Retrieve the public key from the leaf certificate.
    if (cert_chain_.empty()) {
        ADD_FAILURE() << "No public key available";
        return "Failure";
    }
    X509_Ptr key_cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
    EVP_PKEY_Ptr pub_key(X509_get_pubkey(key_cert.get()));
    RSA_Ptr rsa(EVP_PKEY_get1_RSA(const_cast<EVP_PKEY*>(pub_key.get())));

    // Retrieve relevant tags.
    Digest digest = Digest::NONE;
    Digest mgf_digest = Digest::NONE;
    PaddingMode padding = PaddingMode::NONE;

    auto digest_tag = params.GetTagValue(TAG_DIGEST);
    if (digest_tag.has_value()) digest = digest_tag.value();
    auto pad_tag = params.GetTagValue(TAG_PADDING);
    if (pad_tag.has_value()) padding = pad_tag.value();
    auto mgf_tag = params.GetTagValue(TAG_RSA_OAEP_MGF_DIGEST);
    if (mgf_tag.has_value()) mgf_digest = mgf_tag.value();

    const EVP_MD* md = openssl_digest(digest);
    const EVP_MD* mgf_md = openssl_digest(mgf_digest);

    // Set up encryption context.
    EVP_PKEY_CTX_Ptr ctx(EVP_PKEY_CTX_new(pub_key.get(), /* engine= */ nullptr));
    if (EVP_PKEY_encrypt_init(ctx.get()) <= 0) {
        ADD_FAILURE() << "Encryption init failed: " << ERR_peek_last_error();
        return "Failure";
    }

    int rc = -1;
    switch (padding) {
        case PaddingMode::NONE:
            rc = EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_NO_PADDING);
            break;
        case PaddingMode::RSA_PKCS1_1_5_ENCRYPT:
            rc = EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING);
            break;
        case PaddingMode::RSA_OAEP:
            rc = EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING);
            break;
        default:
            break;
    }
    if (rc <= 0) {
        ADD_FAILURE() << "Set padding failed: " << ERR_peek_last_error();
        return "Failure";
    }
    if (padding == PaddingMode::RSA_OAEP) {
        if (!EVP_PKEY_CTX_set_rsa_oaep_md(ctx.get(), md)) {
            ADD_FAILURE() << "Set digest failed: " << ERR_peek_last_error();
            return "Failure";
        }
        if (!EVP_PKEY_CTX_set_rsa_mgf1_md(ctx.get(), mgf_md)) {
            ADD_FAILURE() << "Set MGF digest failed: " << ERR_peek_last_error();
            return "Failure";
        }
    }

    // Determine output size.
    size_t outlen;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr /* out */, &outlen,
                         reinterpret_cast<const uint8_t*>(message.data()), message.size()) <= 0) {
        ADD_FAILURE() << "Determine output size failed: " << ERR_peek_last_error();
        return "Failure";
    }

    // Left-zero-pad the input if necessary.
    const uint8_t* to_encrypt = reinterpret_cast<const uint8_t*>(message.data());
    size_t to_encrypt_len = message.size();

    std::unique_ptr<string> zero_padded_message;
    if (padding == PaddingMode::NONE && to_encrypt_len < outlen) {
        zero_padded_message.reset(new string(outlen, '\0'));
        memcpy(zero_padded_message->data() + (outlen - to_encrypt_len), message.data(),
               message.size());
        to_encrypt = reinterpret_cast<const uint8_t*>(zero_padded_message->data());
        to_encrypt_len = outlen;
    }

    // Do the encryption.
    string output(outlen, '\0');
    if (EVP_PKEY_encrypt(ctx.get(), reinterpret_cast<uint8_t*>(output.data()), &outlen, to_encrypt,
                         to_encrypt_len) <= 0) {
        ADD_FAILURE() << "Encryption failed: " << ERR_peek_last_error();
        return "Failure";
    }
    return output;
}

string KeyMintAidlTestBase::EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                                           const AuthorizationSet& in_params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params);
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, const AuthorizationSet& params,
                                           AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return EncryptMessage(key_blob_, message, params, out_params);
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, const AuthorizationSet& params) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, vector<uint8_t>* iv_out) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(1U, out_params.size());
    auto ivVal = out_params.GetTagValue(TAG_NONCE);
    EXPECT_TRUE(ivVal);
    if (ivVal) *iv_out = *ivVal;
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, uint8_t mac_length_bits,
                                           const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_MAC_LENGTH, mac_length_bits)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeyMintAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                           PaddingMode padding, uint8_t mac_length_bits) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_MAC_LENGTH, mac_length_bits);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeyMintAidlTestBase::DecryptMessage(const vector<uint8_t>& key_blob,
                                           const string& ciphertext,
                                           const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet out_params;
    string plaintext =
            ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return plaintext;
}

string KeyMintAidlTestBase::DecryptMessage(const string& ciphertext,
                                           const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    return DecryptMessage(key_blob_, ciphertext, params);
}

string KeyMintAidlTestBase::DecryptMessage(const string& ciphertext, BlockMode block_mode,
                                           PaddingMode padding_mode, const vector<uint8_t>& iv) {
    SCOPED_TRACE("DecryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding_mode)
                          .Authorization(TAG_NONCE, iv);
    return DecryptMessage(key_blob_, ciphertext, params);
}

std::pair<ErrorCode, vector<uint8_t>> KeyMintAidlTestBase::UpgradeKey(
        const vector<uint8_t>& key_blob) {
    std::pair<ErrorCode, vector<uint8_t>> retval;
    vector<uint8_t> outKeyBlob;
    Status result = keymint_->upgradeKey(key_blob, vector<KeyParameter>(), &outKeyBlob);
    ErrorCode errorcode = GetReturnErrorCode(result);
    retval = std::tie(errorcode, outKeyBlob);

    return retval;
}
vector<uint32_t> KeyMintAidlTestBase::ValidKeySizes(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::RSA:
            switch (SecLevel()) {
                case SecurityLevel::SOFTWARE:
                case SecurityLevel::TRUSTED_ENVIRONMENT:
                    return {2048, 3072, 4096};
                case SecurityLevel::STRONGBOX:
                    return {2048};
                default:
                    ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
                    break;
            }
            break;
        case Algorithm::EC:
            ADD_FAILURE() << "EC keys must be specified by curve not size";
            break;
        case Algorithm::AES:
            return {128, 256};
        case Algorithm::TRIPLE_DES:
            return {168};
        case Algorithm::HMAC: {
            vector<uint32_t> retval((512 - 64) / 8 + 1);
            uint32_t size = 64 - 8;
            std::generate(retval.begin(), retval.end(), [&]() { return (size += 8); });
            return retval;
        }
        default:
            ADD_FAILURE() << "Invalid Algorithm: " << algorithm;
            return {};
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

vector<uint32_t> KeyMintAidlTestBase::InvalidKeySizes(Algorithm algorithm) {
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        switch (algorithm) {
            case Algorithm::RSA:
                return {3072, 4096};
            case Algorithm::EC:
                return {224, 384, 521};
            case Algorithm::AES:
                return {192};
            case Algorithm::TRIPLE_DES:
                return {56};
            default:
                return {};
        }
    } else {
        switch (algorithm) {
            case Algorithm::AES:
                return {64, 96, 131, 512};
            case Algorithm::TRIPLE_DES:
                return {56};
            default:
                return {};
        }
    }
    return {};
}

vector<BlockMode> KeyMintAidlTestBase::ValidBlockModes(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::AES:
            return {
                    BlockMode::CBC,
                    BlockMode::CTR,
                    BlockMode::ECB,
                    BlockMode::GCM,
            };
        case Algorithm::TRIPLE_DES:
            return {
                    BlockMode::CBC,
                    BlockMode::ECB,
            };
        default:
            return {};
    }
}

vector<PaddingMode> KeyMintAidlTestBase::ValidPaddingModes(Algorithm algorithm,
                                                           BlockMode blockMode) {
    switch (algorithm) {
        case Algorithm::AES:
            switch (blockMode) {
                case BlockMode::CBC:
                case BlockMode::ECB:
                    return {PaddingMode::NONE, PaddingMode::PKCS7};
                case BlockMode::CTR:
                case BlockMode::GCM:
                    return {PaddingMode::NONE};
                default:
                    return {};
            };
        case Algorithm::TRIPLE_DES:
            switch (blockMode) {
                case BlockMode::CBC:
                case BlockMode::ECB:
                    return {PaddingMode::NONE, PaddingMode::PKCS7};
                default:
                    return {};
            };
        default:
            return {};
    }
}

vector<PaddingMode> KeyMintAidlTestBase::InvalidPaddingModes(Algorithm algorithm,
                                                             BlockMode blockMode) {
    switch (algorithm) {
        case Algorithm::AES:
            switch (blockMode) {
                case BlockMode::CTR:
                case BlockMode::GCM:
                    return {PaddingMode::PKCS7};
                default:
                    return {};
            };
        default:
            return {};
    }
}

vector<EcCurve> KeyMintAidlTestBase::ValidCurves() {
    if (securityLevel_ == SecurityLevel::STRONGBOX) {
        return {EcCurve::P_256};
    } else if (Curve25519Supported()) {
        return {EcCurve::P_224, EcCurve::P_256, EcCurve::P_384, EcCurve::P_521,
                EcCurve::CURVE_25519};
    } else {
        return {
                EcCurve::P_224,
                EcCurve::P_256,
                EcCurve::P_384,
                EcCurve::P_521,
        };
    }
}

vector<EcCurve> KeyMintAidlTestBase::InvalidCurves() {
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        // Curve 25519 is not supported, either because:
        // - KeyMint v1: it's an unknown enum value
        // - KeyMint v2+: it's not supported by StrongBox.
        return {EcCurve::P_224, EcCurve::P_384, EcCurve::P_521, EcCurve::CURVE_25519};
    } else {
        if (Curve25519Supported()) {
            return {};
        } else {
            return {EcCurve::CURVE_25519};
        }
    }
}

vector<uint64_t> KeyMintAidlTestBase::ValidExponents() {
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        return {65537};
    } else {
        return {3, 65537};
    }
}

vector<Digest> KeyMintAidlTestBase::ValidDigests(bool withNone, bool withMD5) {
    switch (SecLevel()) {
        case SecurityLevel::SOFTWARE:
        case SecurityLevel::TRUSTED_ENVIRONMENT:
            if (withNone) {
                if (withMD5)
                    return {Digest::NONE,      Digest::MD5,       Digest::SHA1,
                            Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
                else
                    return {Digest::NONE,      Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
            } else {
                if (withMD5)
                    return {Digest::MD5,       Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
                else
                    return {Digest::SHA1, Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
            }
            break;
        case SecurityLevel::STRONGBOX:
            if (withNone)
                return {Digest::NONE, Digest::SHA_2_256};
            else
                return {Digest::SHA_2_256};
            break;
        default:
            ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
            break;
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

static const vector<KeyParameter> kEmptyAuthList{};

const vector<KeyParameter>& KeyMintAidlTestBase::SecLevelAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics) {
    auto found = std::find_if(key_characteristics.begin(), key_characteristics.end(),
                              [this](auto& entry) { return entry.securityLevel == SecLevel(); });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

const vector<KeyParameter>& KeyMintAidlTestBase::SecLevelAuthorizations(
        const vector<KeyCharacteristics>& key_characteristics, SecurityLevel securityLevel) {
    auto found = std::find_if(
            key_characteristics.begin(), key_characteristics.end(),
            [securityLevel](auto& entry) { return entry.securityLevel == securityLevel; });
    return (found == key_characteristics.end()) ? kEmptyAuthList : found->authorizations;
}

ErrorCode KeyMintAidlTestBase::UseAesKey(const vector<uint8_t>& aesKeyBlob) {
    auto [result, ciphertext] = ProcessMessage(
            aesKeyBlob, KeyPurpose::ENCRYPT, "1234567890123456",
            AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::NONE));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseHmacKey(const vector<uint8_t>& hmacKeyBlob) {
    auto [result, mac] = ProcessMessage(
            hmacKeyBlob, KeyPurpose::SIGN, "1234567890123456",
            AuthorizationSetBuilder().Authorization(TAG_MAC_LENGTH, 128).Digest(Digest::SHA_2_256));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseRsaKey(const vector<uint8_t>& rsaKeyBlob) {
    std::string message(2048 / 8, 'a');
    auto [result, signature] = ProcessMessage(
            rsaKeyBlob, KeyPurpose::SIGN, message,
            AuthorizationSetBuilder().Digest(Digest::NONE).Padding(PaddingMode::NONE));
    return result;
}

ErrorCode KeyMintAidlTestBase::UseEcdsaKey(const vector<uint8_t>& ecdsaKeyBlob) {
    auto [result, signature] = ProcessMessage(ecdsaKeyBlob, KeyPurpose::SIGN, "a",
                                              AuthorizationSetBuilder().Digest(Digest::SHA_2_256));
    return result;
}

void verify_serial(X509* cert, const uint64_t expected_serial) {
    BIGNUM_Ptr ser(BN_new());
    EXPECT_TRUE(ASN1_INTEGER_to_BN(X509_get_serialNumber(cert), ser.get()));

    uint64_t serial;
    EXPECT_TRUE(BN_get_u64(ser.get(), &serial));
    EXPECT_EQ(serial, expected_serial);
}

// Please set self_signed to true for fake certificates or self signed
// certificates
void verify_subject(const X509* cert,       //
                    const string& subject,  //
                    bool self_signed) {
    char* cert_issuer =  //
            X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);

    char* cert_subj = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);

    string expected_subject("/CN=");
    if (subject.empty()) {
        expected_subject.append("Android Keystore Key");
    } else {
        expected_subject.append(subject);
    }

    EXPECT_STREQ(expected_subject.c_str(), cert_subj) << "Cert has wrong subject." << cert_subj;

    if (self_signed) {
        EXPECT_STREQ(cert_issuer, cert_subj)
                << "Cert issuer and subject mismatch for self signed certificate.";
    }

    OPENSSL_free(cert_subj);
    OPENSSL_free(cert_issuer);
}

int get_vsr_api_level() {
    int vendor_api_level = ::android::base::GetIntProperty("ro.vendor.api_level", -1);
    if (vendor_api_level != -1) {
        return vendor_api_level;
    }

    // Android S and older devices do not define ro.vendor.api_level
    vendor_api_level = ::android::base::GetIntProperty("ro.board.api_level", -1);
    if (vendor_api_level == -1) {
        vendor_api_level = ::android::base::GetIntProperty("ro.board.first_api_level", -1);
    }

    int product_api_level = ::android::base::GetIntProperty("ro.product.first_api_level", -1);
    if (product_api_level == -1) {
        product_api_level = ::android::base::GetIntProperty("ro.build.version.sdk", -1);
        EXPECT_NE(product_api_level, -1) << "Could not find ro.build.version.sdk";
    }

    // VSR API level is the minimum of vendor_api_level and product_api_level.
    if (vendor_api_level == -1 || vendor_api_level > product_api_level) {
        return product_api_level;
    }
    return vendor_api_level;
}

bool is_gsi_image() {
    std::ifstream ifs("/system/system_ext/etc/init/init.gsi.rc");
    return ifs.good();
}

vector<uint8_t> build_serial_blob(const uint64_t serial_int) {
    BIGNUM_Ptr serial(BN_new());
    EXPECT_TRUE(BN_set_u64(serial.get(), serial_int));

    int len = BN_num_bytes(serial.get());
    vector<uint8_t> serial_blob(len);
    if (BN_bn2bin(serial.get(), serial_blob.data()) != len) {
        return {};
    }

    if (serial_blob.empty() || serial_blob[0] & 0x80) {
        // An empty blob is OpenSSL's encoding of the zero value; we need single zero byte.
        // Top bit being set indicates a negative number in two's complement, but our input
        // was positive.
        // In either case, prepend a zero byte.
        serial_blob.insert(serial_blob.begin(), 0x00);
    }

    return serial_blob;
}

void verify_subject_and_serial(const Certificate& certificate,  //
                               const uint64_t expected_serial,  //
                               const string& subject, bool self_signed) {
    X509_Ptr cert(parse_cert_blob(certificate.encodedCertificate));
    ASSERT_TRUE(!!cert.get());

    verify_serial(cert.get(), expected_serial);
    verify_subject(cert.get(), subject, self_signed);
}

void verify_root_of_trust(const vector<uint8_t>& verified_boot_key, bool device_locked,
                          VerifiedBoot verified_boot_state,
                          const vector<uint8_t>& verified_boot_hash) {
    char property_value[PROPERTY_VALUE_MAX] = {};

    if (avb_verification_enabled()) {
        EXPECT_NE(property_get("ro.boot.vbmeta.digest", property_value, ""), 0);
        string prop_string(property_value);
        EXPECT_EQ(prop_string.size(), 64);
        EXPECT_EQ(prop_string, bin2hex(verified_boot_hash));

        EXPECT_NE(property_get("ro.boot.vbmeta.device_state", property_value, ""), 0);
        if (!strcmp(property_value, "unlocked")) {
            EXPECT_FALSE(device_locked);
        } else {
            EXPECT_TRUE(device_locked);
        }

        // Check that the device is locked if not debuggable, e.g., user build
        // images in CTS. For VTS, debuggable images are used to allow adb root
        // and the device is unlocked.
        if (!property_get_bool("ro.debuggable", false)) {
            EXPECT_TRUE(device_locked);
        } else {
            EXPECT_FALSE(device_locked);
        }
    }

    // Verified boot key should be all 0's if the boot state is not verified or self signed
    std::string empty_boot_key(32, '\0');
    std::string verified_boot_key_str((const char*)verified_boot_key.data(),
                                      verified_boot_key.size());
    EXPECT_NE(property_get("ro.boot.verifiedbootstate", property_value, ""), 0);
    if (!strcmp(property_value, "green")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::VERIFIED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "yellow")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::SELF_SIGNED);
        EXPECT_NE(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "orange")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::UNVERIFIED);
        EXPECT_EQ(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    } else if (!strcmp(property_value, "red")) {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::FAILED);
    } else {
        EXPECT_EQ(verified_boot_state, VerifiedBoot::UNVERIFIED);
        EXPECT_EQ(0, memcmp(verified_boot_key.data(), empty_boot_key.data(),
                            verified_boot_key.size()));
    }
}

bool verify_attestation_record(int32_t aidl_version,                   //
                               const string& challenge,                //
                               const string& app_id,                   //
                               AuthorizationSet expected_sw_enforced,  //
                               AuthorizationSet expected_hw_enforced,  //
                               SecurityLevel security_level,
                               const vector<uint8_t>& attestation_cert,
                               vector<uint8_t>* unique_id) {
    X509_Ptr cert(parse_cert_blob(attestation_cert));
    EXPECT_TRUE(!!cert.get());
    if (!cert.get()) return false;

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    EXPECT_TRUE(!!attest_rec);
    if (!attest_rec) return false;

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_hw_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymint_version;
    SecurityLevel att_attestation_security_level;
    SecurityLevel att_keymint_security_level;
    vector<uint8_t> att_challenge;
    vector<uint8_t> att_unique_id;
    vector<uint8_t> att_app_id;

    auto error = parse_attestation_record(attest_rec->data,                 //
                                          attest_rec->length,               //
                                          &att_attestation_version,         //
                                          &att_attestation_security_level,  //
                                          &att_keymint_version,             //
                                          &att_keymint_security_level,      //
                                          &att_challenge,                   //
                                          &att_sw_enforced,                 //
                                          &att_hw_enforced,                 //
                                          &att_unique_id);
    EXPECT_EQ(ErrorCode::OK, error);
    if (error != ErrorCode::OK) return false;

    check_attestation_version(att_attestation_version, aidl_version);
    vector<uint8_t> appId(app_id.begin(), app_id.end());

    // check challenge and app id only if we expects a non-fake certificate
    if (challenge.length() > 0) {
        EXPECT_EQ(challenge.length(), att_challenge.size());
        EXPECT_EQ(0, memcmp(challenge.data(), att_challenge.data(), challenge.length()));

        expected_sw_enforced.push_back(TAG_ATTESTATION_APPLICATION_ID, appId);
    }

    check_attestation_version(att_keymint_version, aidl_version);
    EXPECT_EQ(security_level, att_keymint_security_level);
    EXPECT_EQ(security_level, att_attestation_security_level);

    // TODO(b/136282179): When running under VTS-on-GSI the TEE-backed
    // keymint implementation will report YYYYMM dates instead of YYYYMMDD
    // for the BOOT_PATCH_LEVEL.
    if (avb_verification_enabled()) {
        for (int i = 0; i < att_hw_enforced.size(); i++) {
            if (att_hw_enforced[i].tag == TAG_BOOT_PATCHLEVEL ||
                att_hw_enforced[i].tag == TAG_VENDOR_PATCHLEVEL) {
                std::string date =
                        std::to_string(att_hw_enforced[i].value.get<KeyParameterValue::integer>());

                // strptime seems to require delimiters, but the tag value will
                // be YYYYMMDD
                if (date.size() != 8) {
                    ADD_FAILURE() << "Tag " << att_hw_enforced[i].tag
                                  << " with invalid format (not YYYYMMDD): " << date;
                    return false;
                }
                date.insert(6, "-");
                date.insert(4, "-");
                struct tm time;
                strptime(date.c_str(), "%Y-%m-%d", &time);

                // Day of the month (0-31)
                EXPECT_GE(time.tm_mday, 0);
                EXPECT_LT(time.tm_mday, 32);
                // Months since Jan (0-11)
                EXPECT_GE(time.tm_mon, 0);
                EXPECT_LT(time.tm_mon, 12);
                // Years since 1900
                EXPECT_GT(time.tm_year, 110);
                EXPECT_LT(time.tm_year, 200);
            }
        }
    }

    // Check to make sure boolean values are properly encoded. Presence of a boolean tag
    // indicates true. A provided boolean tag that can be pulled back out of the certificate
    // indicates correct encoding. No need to check if it's in both lists, since the
    // AuthorizationSet compare below will handle mismatches of tags.
    if (security_level == SecurityLevel::SOFTWARE) {
        EXPECT_TRUE(expected_sw_enforced.Contains(TAG_NO_AUTH_REQUIRED));
    } else {
        EXPECT_TRUE(expected_hw_enforced.Contains(TAG_NO_AUTH_REQUIRED));
    }

    if (att_hw_enforced.Contains(TAG_ALGORITHM, Algorithm::EC)) {
        // For ECDSA keys, either an EC_CURVE or a KEY_SIZE can be specified, but one must be.
        EXPECT_TRUE(att_hw_enforced.Contains(TAG_EC_CURVE) ||
                    att_hw_enforced.Contains(TAG_KEY_SIZE));
    }

    // Test root of trust elements
    vector<uint8_t> verified_boot_key;
    VerifiedBoot verified_boot_state;
    bool device_locked;
    vector<uint8_t> verified_boot_hash;
    error = parse_root_of_trust(attest_rec->data, attest_rec->length, &verified_boot_key,
                                &verified_boot_state, &device_locked, &verified_boot_hash);
    EXPECT_EQ(ErrorCode::OK, error);
    verify_root_of_trust(verified_boot_key, device_locked, verified_boot_state, verified_boot_hash);

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    EXPECT_EQ(filtered_tags(expected_sw_enforced), filtered_tags(att_sw_enforced));

    att_hw_enforced.Sort();
    expected_hw_enforced.Sort();
    EXPECT_EQ(filtered_tags(expected_hw_enforced), filtered_tags(att_hw_enforced));

    if (unique_id != nullptr) {
        *unique_id = att_unique_id;
    }

    return true;
}

string bin2hex(const vector<uint8_t>& data) {
    string retval;
    retval.reserve(data.size() * 2 + 1);
    for (uint8_t byte : data) {
        retval.push_back(nibble2hex[0x0F & (byte >> 4)]);
        retval.push_back(nibble2hex[0x0F & byte]);
    }
    return retval;
}

AuthorizationSet HwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics) {
    AuthorizationSet authList;
    for (auto& entry : key_characteristics) {
        if (entry.securityLevel == SecurityLevel::STRONGBOX ||
            entry.securityLevel == SecurityLevel::TRUSTED_ENVIRONMENT) {
            authList.push_back(AuthorizationSet(entry.authorizations));
        }
    }
    return authList;
}

AuthorizationSet SwEnforcedAuthorizations(const vector<KeyCharacteristics>& key_characteristics) {
    AuthorizationSet authList;
    for (auto& entry : key_characteristics) {
        if (entry.securityLevel == SecurityLevel::SOFTWARE ||
            entry.securityLevel == SecurityLevel::KEYSTORE) {
            authList.push_back(AuthorizationSet(entry.authorizations));
        }
    }
    return authList;
}

AssertionResult ChainSignaturesAreValid(const vector<Certificate>& chain,
                                        bool strict_issuer_check) {
    std::stringstream cert_data;

    for (size_t i = 0; i < chain.size(); ++i) {
        cert_data << bin2hex(chain[i].encodedCertificate) << std::endl;

        X509_Ptr key_cert(parse_cert_blob(chain[i].encodedCertificate));
        X509_Ptr signing_cert;
        if (i < chain.size() - 1) {
            signing_cert = parse_cert_blob(chain[i + 1].encodedCertificate);
        } else {
            signing_cert = parse_cert_blob(chain[i].encodedCertificate);
        }
        if (!key_cert.get() || !signing_cert.get()) return AssertionFailure() << cert_data.str();

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        if (!signing_pubkey.get()) return AssertionFailure() << cert_data.str();

        if (!X509_verify(key_cert.get(), signing_pubkey.get())) {
            return AssertionFailure()
                   << "Verification of certificate " << i << " failed "
                   << "OpenSSL error string: " << ERR_error_string(ERR_get_error(), NULL) << '\n'
                   << cert_data.str();
        }

        string cert_issuer = x509NameToStr(X509_get_issuer_name(key_cert.get()));
        string signer_subj = x509NameToStr(X509_get_subject_name(signing_cert.get()));
        if (cert_issuer != signer_subj && strict_issuer_check) {
            return AssertionFailure() << "Cert " << i << " has wrong issuer.\n"
                                      << " Signer subject is " << signer_subj
                                      << " Issuer subject is " << cert_issuer << endl
                                      << cert_data.str();
        }
    }

    if (KeyMintAidlTestBase::dump_Attestations) std::cout << cert_data.str();
    return AssertionSuccess();
}

X509_Ptr parse_cert_blob(const vector<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return X509_Ptr(d2i_X509(nullptr /* allocate new */, &p, blob.size()));
}

vector<uint8_t> make_name_from_str(const string& name) {
    X509_NAME_Ptr x509_name(X509_NAME_new());
    EXPECT_TRUE(x509_name.get() != nullptr);
    if (!x509_name) return {};

    EXPECT_EQ(1, X509_NAME_add_entry_by_txt(x509_name.get(),  //
                                            "CN",             //
                                            MBSTRING_ASC,
                                            reinterpret_cast<const uint8_t*>(name.c_str()),
                                            -1,  // len
                                            -1,  // loc
                                            0 /* set */));

    int len = i2d_X509_NAME(x509_name.get(), nullptr /* only return length */);
    EXPECT_GT(len, 0);

    vector<uint8_t> retval(len);
    uint8_t* p = retval.data();
    i2d_X509_NAME(x509_name.get(), &p);

    return retval;
}

namespace {

void check_cose_key(const vector<uint8_t>& data, bool testMode) {
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(data);
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;

    // The following check assumes that canonical CBOR encoding is used for the COSE_Key.
    if (testMode) {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "  -70000 : null,\n"                              // test marker
                                 "}"));
    } else {
        EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                    MatchesRegex("{\n"
                                 "  1 : 2,\n"   // kty: EC2
                                 "  3 : -7,\n"  // alg: ES256
                                 "  -1 : 1,\n"  // EC id: P256
                                 // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a
                                 // sequence of 32 hexadecimal bytes, enclosed in braces and
                                 // separated by commas. In this case, some Ed25519 public key.
                                 "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_x: data
                                 "  -3 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"  // pub_y: data
                                 "}"));
    }
}

}  // namespace

void check_maced_pubkey(const MacedPublicKey& macedPubKey, bool testMode,
                        vector<uint8_t>* payload_value) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);
    ASSERT_TRUE(coseMac0) << "COSE Mac0 parse failed " << mac0ParseErr;

    ASSERT_NE(coseMac0->asArray(), nullptr);
    ASSERT_EQ(coseMac0->asArray()->size(), kCoseMac0EntryCount);

    auto protParms = coseMac0->asArray()->get(kCoseMac0ProtectedParams)->asBstr();
    ASSERT_NE(protParms, nullptr);

    // Header label:value of 'alg': HMAC-256
    ASSERT_EQ(cppbor::prettyPrint(protParms->value()), "{\n  1 : 5,\n}");

    auto unprotParms = coseMac0->asArray()->get(kCoseMac0UnprotectedParams)->asMap();
    ASSERT_NE(unprotParms, nullptr);
    ASSERT_EQ(unprotParms->size(), 0);

    // The payload is a bstr holding an encoded COSE_Key
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    ASSERT_NE(payload, nullptr);
    check_cose_key(payload->value(), testMode);

    auto coseMac0Tag = coseMac0->asArray()->get(kCoseMac0Tag)->asBstr();
    ASSERT_TRUE(coseMac0Tag);
    auto extractedTag = coseMac0Tag->value();
    EXPECT_EQ(extractedTag.size(), 32U);

    // Compare with tag generated with kTestMacKey.  Should only match in test mode
    auto macFunction = [](const cppcose::bytevec& input) {
        return cppcose::generateHmacSha256(remote_prov::kTestMacKey, input);
    };
    auto testTag =
            cppcose::generateCoseMac0Mac(macFunction, {} /* external_aad */, payload->value());
    ASSERT_TRUE(testTag) << "Tag calculation failed: " << testTag.message();

    if (testMode) {
        EXPECT_THAT(*testTag, ElementsAreArray(extractedTag));
    } else {
        EXPECT_THAT(*testTag, Not(ElementsAreArray(extractedTag)));
    }
    if (payload_value != nullptr) {
        *payload_value = payload->value();
    }
}

void p256_pub_key(const vector<uint8_t>& coseKeyData, EVP_PKEY_Ptr* signingKey) {
    // Extract x and y affine coordinates from the encoded Cose_Key.
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(coseKeyData);
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;
    auto coseKey = parsedPayload->asMap();
    const std::unique_ptr<cppbor::Item>& xItem = coseKey->get(cppcose::CoseKey::PUBKEY_X);
    ASSERT_NE(xItem->asBstr(), nullptr);
    vector<uint8_t> x = xItem->asBstr()->value();
    const std::unique_ptr<cppbor::Item>& yItem = coseKey->get(cppcose::CoseKey::PUBKEY_Y);
    ASSERT_NE(yItem->asBstr(), nullptr);
    vector<uint8_t> y = yItem->asBstr()->value();

    // Concatenate: 0x04 (uncompressed form marker) | x | y
    vector<uint8_t> pubKeyData{0x04};
    pubKeyData.insert(pubKeyData.end(), x.begin(), x.end());
    pubKeyData.insert(pubKeyData.end(), y.begin(), y.end());

    EC_KEY_Ptr ecKey = EC_KEY_Ptr(EC_KEY_new());
    ASSERT_NE(ecKey, nullptr);
    EC_GROUP_Ptr group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    ASSERT_NE(group, nullptr);
    ASSERT_EQ(EC_KEY_set_group(ecKey.get(), group.get()), 1);
    EC_POINT_Ptr point = EC_POINT_Ptr(EC_POINT_new(group.get()));
    ASSERT_NE(point, nullptr);
    ASSERT_EQ(EC_POINT_oct2point(group.get(), point.get(), pubKeyData.data(), pubKeyData.size(),
                                 nullptr),
              1);
    ASSERT_EQ(EC_KEY_set_public_key(ecKey.get(), point.get()), 1);

    EVP_PKEY_Ptr pubKey = EVP_PKEY_Ptr(EVP_PKEY_new());
    ASSERT_NE(pubKey, nullptr);
    EVP_PKEY_assign_EC_KEY(pubKey.get(), ecKey.release());
    *signingKey = std::move(pubKey);
}

// Check whether the given named feature is available.
bool check_feature(const std::string& name) {
    ::android::sp<::android::IServiceManager> sm(::android::defaultServiceManager());
    ::android::sp<::android::IBinder> binder(sm->getService(::android::String16("package_native")));
    if (binder == nullptr) {
        GTEST_LOG_(ERROR) << "getService package_native failed";
        return false;
    }
    ::android::sp<::android::content::pm::IPackageManagerNative> packageMgr =
            ::android::interface_cast<::android::content::pm::IPackageManagerNative>(binder);
    if (packageMgr == nullptr) {
        GTEST_LOG_(ERROR) << "Cannot find package manager";
        return false;
    }
    bool hasFeature = false;
    auto status = packageMgr->hasSystemFeature(::android::String16(name.c_str()), 0, &hasFeature);
    if (!status.isOk()) {
        GTEST_LOG_(ERROR) << "hasSystemFeature('" << name << "') failed: " << status;
        return false;
    }
    return hasFeature;
}

}  // namespace test

}  // namespace aidl::android::hardware::security::keymint
