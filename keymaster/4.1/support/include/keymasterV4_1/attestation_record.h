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

#pragma once

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <keymasterV4_0/attestation_record.h>
#include <keymasterV4_0/openssl_utils.h>
#include <keymasterV4_1/authorization_set.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_1 {

using V4_0::kAttestionRecordOid;
using V4_0::keymaster_verified_boot_t;

struct RootOfTrust {
    SecurityLevel security_level;
    hidl_vec<uint8_t> verified_boot_key;
    hidl_vec<uint8_t> verified_boot_hash;
    keymaster_verified_boot_t verified_boot_state;
    bool device_locked;
};

struct AttestationRecord {
    RootOfTrust root_of_trust;
    uint32_t attestation_version;
    SecurityLevel attestation_security_level;
    uint32_t keymaster_version;
    SecurityLevel keymaster_security_level;
    hidl_vec<uint8_t> attestation_challenge;
    AuthorizationSet software_enforced;
    AuthorizationSet hardware_enforced;
    hidl_vec<uint8_t> unique_id;
};

std::tuple<ErrorCode, AttestationRecord> parse_attestation_record(const hidl_vec<uint8_t>& cert);

}  // namespace V4_1
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
