/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <hardware/hw_auth_token.h>
#include <keymasterV4_0/keymaster_utils.h>

namespace android {
namespace hardware {

inline static bool operator<(const hidl_vec<uint8_t>& a, const hidl_vec<uint8_t>& b) {
    return memcmp(a.data(), b.data(), std::min(a.size(), b.size())) == -1;
}

template <size_t SIZE>
inline static bool operator<(const hidl_array<uint8_t, SIZE>& a,
                             const hidl_array<uint8_t, SIZE>& b) {
    return memcmp(a.data(), b.data(), SIZE) == -1;
}

namespace keymaster {
namespace V4_0 {

bool operator<(const HmacSharingParameters& a, const HmacSharingParameters& b) {
    return std::tie(a.seed, a.nonce) < std::tie(b.seed, b.nonce);
}

namespace support {

template <typename T, typename InIter>
inline static InIter copy_bytes_from_iterator(T* value, InIter src) {
    uint8_t* value_ptr = reinterpret_cast<uint8_t*>(value);
    std::copy(src, src + sizeof(T), value_ptr);
    return src + sizeof(T);
}

template <typename T, typename OutIter>
inline static OutIter copy_bytes_to_iterator(const T& value, OutIter dest) {
    const uint8_t* value_ptr = reinterpret_cast<const uint8_t*>(&value);
    return std::copy(value_ptr, value_ptr + sizeof(value), dest);
}

constexpr size_t kHmacSize = 32;

hidl_vec<uint8_t> authToken2HidlVec(const HardwareAuthToken& token) {
    static_assert(1 /* version size */ + sizeof(token.challenge) + sizeof(token.userId) +
                          sizeof(token.authenticatorId) + sizeof(token.authenticatorType) +
                          sizeof(token.timestamp) + kHmacSize ==
                      sizeof(hw_auth_token_t),
                  "HardwareAuthToken content size does not match hw_auth_token_t size");

    hidl_vec<uint8_t> result;
    result.resize(sizeof(hw_auth_token_t));
    auto pos = result.begin();
    *pos++ = 0;  // Version byte
    pos = copy_bytes_to_iterator(token.challenge, pos);
    pos = copy_bytes_to_iterator(token.userId, pos);
    pos = copy_bytes_to_iterator(token.authenticatorId, pos);
    auto auth_type = htonl(static_cast<uint32_t>(token.authenticatorType));
    pos = copy_bytes_to_iterator(auth_type, pos);
    auto timestamp = htonq(token.timestamp);
    pos = copy_bytes_to_iterator(timestamp, pos);
    if (token.mac.size() != kHmacSize) {
        std::fill(pos, pos + kHmacSize, 0);
    } else {
        std::copy(token.mac.begin(), token.mac.end(), pos);
    }

    return result;
}

HardwareAuthToken hidlVec2AuthToken(const hidl_vec<uint8_t>& buffer) {
    HardwareAuthToken token;
    static_assert(1 /* version size */ + sizeof(token.challenge) + sizeof(token.userId) +
                          sizeof(token.authenticatorId) + sizeof(token.authenticatorType) +
                          sizeof(token.timestamp) + kHmacSize ==
                      sizeof(hw_auth_token_t),
                  "HardwareAuthToken content size does not match hw_auth_token_t size");

    if (buffer.size() != sizeof(hw_auth_token_t)) return {};

    auto pos = buffer.begin();
    ++pos;  // skip first byte
    pos = copy_bytes_from_iterator(&token.challenge, pos);
    pos = copy_bytes_from_iterator(&token.userId, pos);
    pos = copy_bytes_from_iterator(&token.authenticatorId, pos);
    pos = copy_bytes_from_iterator(&token.authenticatorType, pos);
    token.authenticatorType = static_cast<HardwareAuthenticatorType>(
        ntohl(static_cast<uint32_t>(token.authenticatorType)));
    pos = copy_bytes_from_iterator(&token.timestamp, pos);
    token.timestamp = ntohq(token.timestamp);
    token.mac.resize(kHmacSize);
    std::copy(pos, pos + kHmacSize, token.mac.data());

    return token;
}

}  // namespace support
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
