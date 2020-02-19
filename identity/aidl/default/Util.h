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

#ifndef ANDROID_HARDWARE_IDENTITY_UTIL_H
#define ANDROID_HARDWARE_IDENTITY_UTIL_H

#include <aidl/android/hardware/identity/BnIdentityCredential.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <cppbor/cppbor.h>

namespace aidl::android::hardware::identity {

using ::std::optional;
using ::std::string;
using ::std::vector;

// Returns the hardware-bound AES-128 key.
const vector<uint8_t>& getHardwareBoundKey();

// Calculates the MAC for |profile| using |storageKey|.
optional<vector<uint8_t>> secureAccessControlProfileCalcMac(
        const SecureAccessControlProfile& profile, const vector<uint8_t>& storageKey);

// Checks authenticity of the MAC in |profile| using |storageKey|.
bool secureAccessControlProfileCheckMac(const SecureAccessControlProfile& profile,
                                        const vector<uint8_t>& storageKey);

// Creates the AdditionalData CBOR used in the addEntryValue() HIDL method.
vector<uint8_t> entryCreateAdditionalData(const string& nameSpace, const string& name,
                                          const vector<int32_t> accessControlProfileIds);

vector<uint8_t> byteStringToUnsigned(const vector<int8_t>& value);

vector<int8_t> byteStringToSigned(const vector<uint8_t>& value);

}  // namespace aidl::android::hardware::identity

#endif  // ANDROID_HARDWARE_IDENTITY_UTIL_H
