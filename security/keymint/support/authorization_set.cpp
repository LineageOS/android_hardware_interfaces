/*
 * Copyright 2020 The Android Open Source Project
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

#include <keymint_support/authorization_set.h>

#include <assert.h>
#include <sstream>

#include <android-base/logging.h>

#include <android/hardware/security/keymint/Algorithm.h>
#include <android/hardware/security/keymint/BlockMode.h>
#include <android/hardware/security/keymint/Digest.h>
#include <android/hardware/security/keymint/KeyParameter.h>
#include <android/hardware/security/keymint/KeyPurpose.h>
#include <android/hardware/security/keymint/TagType.h>

namespace android::hardware::security::keymint {

void AuthorizationSet::Sort() {
    std::sort(data_.begin(), data_.end());
}

void AuthorizationSet::Deduplicate() {
    if (data_.empty()) return;

    Sort();
    std::vector<KeyParameter> result;

    auto curr = data_.begin();
    auto prev = curr++;
    for (; curr != data_.end(); ++prev, ++curr) {
        if (prev->tag == Tag::INVALID) continue;

        if (*prev != *curr) {
            result.push_back(std::move(*prev));
        }
    }
    result.push_back(std::move(*prev));

    std::swap(data_, result);
}

void AuthorizationSet::Union(const AuthorizationSet& other) {
    data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    Deduplicate();
}

void AuthorizationSet::Subtract(const AuthorizationSet& other) {
    Deduplicate();

    auto i = other.begin();
    while (i != other.end()) {
        int pos = -1;
        do {
            pos = find(i->tag, pos);
            if (pos != -1 && (*i == data_[pos])) {
                data_.erase(data_.begin() + pos);
                break;
            }
        } while (pos != -1);
        ++i;
    }
}

KeyParameter& AuthorizationSet::operator[](int at) {
    return data_[at];
}

const KeyParameter& AuthorizationSet::operator[](int at) const {
    return data_[at];
}

void AuthorizationSet::Clear() {
    data_.clear();
}

size_t AuthorizationSet::GetTagCount(Tag tag) const {
    size_t count = 0;
    for (int pos = -1; (pos = find(tag, pos)) != -1;) ++count;
    return count;
}

int AuthorizationSet::find(Tag tag, int begin) const {
    auto iter = data_.begin() + (1 + begin);

    while (iter != data_.end() && iter->tag != tag) ++iter;

    if (iter != data_.end()) return iter - data_.begin();
    return -1;
}

bool AuthorizationSet::erase(int index) {
    auto pos = data_.begin() + index;
    if (pos != data_.end()) {
        data_.erase(pos);
        return true;
    }
    return false;
}

NullOr<const KeyParameter&> AuthorizationSet::GetEntry(Tag tag) const {
    int pos = find(tag);
    if (pos == -1) return {};
    return data_[pos];
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaKey(uint32_t key_size,
                                                         uint64_t public_exponent) {
    Authorization(TAG_ALGORITHM, Algorithm::RSA);
    Authorization(TAG_KEY_SIZE, key_size);
    Authorization(TAG_RSA_PUBLIC_EXPONENT, public_exponent);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::EC);
    Authorization(TAG_KEY_SIZE, key_size);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaKey(EcCurve curve) {
    Authorization(TAG_ALGORITHM, Algorithm::EC);
    Authorization(TAG_EC_CURVE, curve);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::AesKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::AES);
    return Authorization(TAG_KEY_SIZE, key_size);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::TripleDesKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::TRIPLE_DES);
    return Authorization(TAG_KEY_SIZE, key_size);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::HmacKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::HMAC);
    Authorization(TAG_KEY_SIZE, key_size);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaSigningKey(uint32_t key_size,
                                                                uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaEncryptionKey(uint32_t key_size,
                                                                   uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaSigningKey(uint32_t key_size) {
    EcdsaKey(key_size);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaSigningKey(EcCurve curve) {
    EcdsaKey(curve);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::AesEncryptionKey(uint32_t key_size) {
    AesKey(key_size);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::TripleDesEncryptionKey(uint32_t key_size) {
    TripleDesKey(key_size);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::SigningKey() {
    Authorization(TAG_PURPOSE, KeyPurpose::SIGN);
    return Authorization(TAG_PURPOSE, KeyPurpose::VERIFY);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EncryptionKey() {
    Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT);
    return Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::NoDigestOrPadding() {
    Authorization(TAG_DIGEST, Digest::NONE);
    return Authorization(TAG_PADDING, PaddingMode::NONE);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcbMode() {
    return Authorization(TAG_BLOCK_MODE, BlockMode::ECB);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::GcmModeMinMacLen(uint32_t minMacLength) {
    return BlockMode(BlockMode::GCM)
            .Padding(PaddingMode::NONE)
            .Authorization(TAG_MIN_MAC_LENGTH, minMacLength);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::GcmModeMacLen(uint32_t macLength) {
    return BlockMode(BlockMode::GCM)
            .Padding(PaddingMode::NONE)
            .Authorization(TAG_MAC_LENGTH, macLength);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::BlockMode(
        std::initializer_list<android::hardware::security::keymint::BlockMode> blockModes) {
    for (auto mode : blockModes) {
        push_back(TAG_BLOCK_MODE, mode);
    }
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::Digest(std::vector<keymint::Digest> digests) {
    for (auto digest : digests) {
        push_back(TAG_DIGEST, digest);
    }
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::Padding(
        std::initializer_list<PaddingMode> paddingModes) {
    for (auto paddingMode : paddingModes) {
        push_back(TAG_PADDING, paddingMode);
    }
    return *this;
}

}  // namespace android::hardware::security::keymint
