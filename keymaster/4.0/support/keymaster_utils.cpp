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

#include <regex.h>

#include <android-base/properties.h>
#include <hardware/hw_auth_token.h>
#include <keymasterV4_0/keymaster_utils.h>

namespace android::hardware {

inline static bool operator<(const hidl_vec<uint8_t>& a, const hidl_vec<uint8_t>& b) {
    auto result = memcmp(a.data(), b.data(), std::min(a.size(), b.size()));
    if (!result) return a.size() < b.size();
    return result < 0;
}

template <size_t SIZE>
inline static bool operator<(const hidl_array<uint8_t, SIZE>& a,
                             const hidl_array<uint8_t, SIZE>& b) {
    return memcmp(a.data(), b.data(), SIZE) == -1;
}

namespace keymaster::V4_0 {

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

namespace {

constexpr char kPlatformVersionProp[] = "ro.build.version.release";
constexpr char kPlatformVersionRegex[] = "^([0-9]{1,2})(\\.([0-9]{1,2}))?(\\.([0-9]{1,2}))?";
constexpr size_t kMajorVersionMatch = 1;
constexpr size_t kMinorVersionMatch = 3;
constexpr size_t kSubminorVersionMatch = 5;
constexpr size_t kPlatformVersionMatchCount = kSubminorVersionMatch + 1;

constexpr char kPlatformPatchlevelProp[] = "ro.build.version.security_patch";
constexpr char kPlatformPatchlevelRegex[] = "^([0-9]{4})-([0-9]{2})-[0-9]{2}$";
constexpr size_t kYearMatch = 1;
constexpr size_t kMonthMatch = 2;
constexpr size_t kPlatformPatchlevelMatchCount = kMonthMatch + 1;

uint32_t match_to_uint32(const char* expression, const regmatch_t& match) {
    if (match.rm_so == -1) return 0;

    size_t len = match.rm_eo - match.rm_so;
    std::string s(expression + match.rm_so, len);
    return std::stoul(s);
}

std::string wait_and_get_property(const char* prop) {
    std::string prop_value;
    while (!android::base::WaitForPropertyCreation(prop))
        ;
    prop_value = android::base::GetProperty(prop, "" /* default */);
    return prop_value;
}

}  // anonymous namespace

uint32_t getOsVersion(const char* version_str) {
    regex_t regex;
    if (regcomp(&regex, kPlatformVersionRegex, REG_EXTENDED)) {
        return 0;
    }

    regmatch_t matches[kPlatformVersionMatchCount];
    int not_match =
            regexec(&regex, version_str, kPlatformVersionMatchCount, matches, 0 /* flags */);
    regfree(&regex);
    if (not_match) {
        return 0;
    }

    uint32_t major = match_to_uint32(version_str, matches[kMajorVersionMatch]);
    uint32_t minor = match_to_uint32(version_str, matches[kMinorVersionMatch]);
    uint32_t subminor = match_to_uint32(version_str, matches[kSubminorVersionMatch]);

    return (major * 100 + minor) * 100 + subminor;
}

uint32_t getOsVersion() {
    std::string version = wait_and_get_property(kPlatformVersionProp);
    return getOsVersion(version.c_str());
}

uint32_t getOsPatchlevel(const char* patchlevel_str) {
    regex_t regex;
    if (regcomp(&regex, kPlatformPatchlevelRegex, REG_EXTENDED) != 0) {
        return 0;
    }

    regmatch_t matches[kPlatformPatchlevelMatchCount];
    int not_match =
            regexec(&regex, patchlevel_str, kPlatformPatchlevelMatchCount, matches, 0 /* flags */);
    regfree(&regex);
    if (not_match) {
        return 0;
    }

    uint32_t year = match_to_uint32(patchlevel_str, matches[kYearMatch]);
    uint32_t month = match_to_uint32(patchlevel_str, matches[kMonthMatch]);

    if (month < 1 || month > 12) {
        return 0;
    }
    return year * 100 + month;
}

uint32_t getOsPatchlevel() {
    std::string patchlevel = wait_and_get_property(kPlatformPatchlevelProp);
    return getOsPatchlevel(patchlevel.c_str());
}

}  // namespace support
}  // namespace keymaster::V4_0
}  // namespace android::hardware
