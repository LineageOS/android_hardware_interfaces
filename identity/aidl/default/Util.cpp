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

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <string.h>

#include <android-base/logging.h>

#include <cppbor.h>
#include <cppbor_parse.h>

namespace aidl::android::hardware::identity {

using namespace ::android::hardware::identity;

// This is not a very random HBK but that's OK because this is the SW
// implementation where it can't be kept secret.
vector<uint8_t> hardwareBoundKey = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

const vector<uint8_t>& getHardwareBoundKey() {
    return hardwareBoundKey;
}

vector<uint8_t> byteStringToUnsigned(const vector<int8_t>& value) {
    return vector<uint8_t>(value.begin(), value.end());
}

vector<int8_t> byteStringToSigned(const vector<uint8_t>& value) {
    return vector<int8_t>(value.begin(), value.end());
}

vector<uint8_t> secureAccessControlProfileEncodeCbor(const SecureAccessControlProfile& profile) {
    cppbor::Map map;
    map.add("id", profile.id);

    if (profile.readerCertificate.encodedCertificate.size() > 0) {
        map.add("readerCertificate",
                cppbor::Bstr(byteStringToUnsigned(profile.readerCertificate.encodedCertificate)));
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

    optional<vector<uint8_t>> nonce = support::getRandom(12);
    if (!nonce) {
        return {};
    }
    optional<vector<uint8_t>> macO =
            support::encryptAes128Gcm(storageKey, nonce.value(), {}, cborData);
    if (!macO) {
        return {};
    }
    return macO.value();
}

bool secureAccessControlProfileCheckMac(const SecureAccessControlProfile& profile,
                                        const vector<uint8_t>& storageKey) {
    vector<uint8_t> cborData = secureAccessControlProfileEncodeCbor(profile);

    if (profile.mac.size() < support::kAesGcmIvSize) {
        return false;
    }
    vector<uint8_t> nonce =
            vector<uint8_t>(profile.mac.begin(), profile.mac.begin() + support::kAesGcmIvSize);
    optional<vector<uint8_t>> mac = support::encryptAes128Gcm(storageKey, nonce, {}, cborData);
    if (!mac) {
        return false;
    }
    if (mac.value() != byteStringToUnsigned(profile.mac)) {
        return false;
    }
    return true;
}

vector<uint8_t> entryCreateAdditionalData(const string& nameSpace, const string& name,
                                          const vector<int32_t> accessControlProfileIds) {
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

}  // namespace aidl::android::hardware::identity
