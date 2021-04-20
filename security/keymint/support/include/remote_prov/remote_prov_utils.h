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

#pragma once

#include <vector>

#include <keymaster/cppcose/cppcose.h>

namespace aidl::android::hardware::security::keymint::remote_prov {

using bytevec = std::vector<uint8_t>;
using namespace cppcose;

extern bytevec kTestMacKey;

/**
 * Generates random bytes.
 */
bytevec randomBytes(size_t numBytes);

struct EekChain {
    bytevec chain;
    bytevec last_pubkey;
    bytevec last_privkey;
};

/**
 * Generates an X25518 EEK with the specified eekId and an Ed25519 chain of the
 * specified length. All keys are generated randomly.
 */
ErrMsgOr<EekChain> generateEekChain(size_t length, const bytevec& eekId);

struct BccEntryData {
    bytevec pubKey;
};

/**
 * Validates the provided CBOR-encoded BCC, returning a vector of BccEntryData
 * structs containing the BCC entry contents.  If an entry contains no firmware
 * digest, the corresponding BccEntryData.firmwareDigest will have length zero
 * (there's no way to distinguish between an empty and missing firmware digest,
 * which seems fine).
 */
ErrMsgOr<std::vector<BccEntryData>> validateBcc(const cppbor::Array* bcc);

}  // namespace aidl::android::hardware::security::keymint::remote_prov
