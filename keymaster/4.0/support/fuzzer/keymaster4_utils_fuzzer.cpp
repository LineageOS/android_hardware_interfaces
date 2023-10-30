/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <hardware/hw_auth_token.h>
#include <keymasterV4_0/keymaster_utils.h>
#include "keymaster4_common.h"

namespace android::hardware::keymaster::V4_0::fuzzer {

using android::hardware::keymaster::V4_0::SecurityLevel;
using android::hardware::keymaster::V4_0::VerificationToken;
using android::hardware::keymaster::V4_0::support::deserializeVerificationToken;
using android::hardware::keymaster::V4_0::support::serializeVerificationToken;

constexpr SecurityLevel kSecurityLevel[]{
        SecurityLevel::SOFTWARE,
        SecurityLevel::TRUSTED_ENVIRONMENT,
        SecurityLevel::STRONGBOX,
};
constexpr size_t kMaxVectorSize = 100;
constexpr size_t kMaxCharacters = 100;

class KeyMaster4UtilsFuzzer {
  public:
    void process(const uint8_t* data, size_t size);

  private:
    void invokeKeyMasterUtils();
    std::unique_ptr<FuzzedDataProvider> mFdp = nullptr;
};

void KeyMaster4UtilsFuzzer::invokeKeyMasterUtils() {
    support::getOsVersion();
    support::getOsPatchlevel();

    while (mFdp->remaining_bytes() > 0) {
        auto keymaster_function = mFdp->PickValueInArray<const std::function<void()>>({
                [&]() {
                    VerificationToken token;
                    token.challenge = mFdp->ConsumeIntegral<uint64_t>();
                    token.timestamp = mFdp->ConsumeIntegral<uint64_t>();
                    token.securityLevel = mFdp->PickValueInArray(kSecurityLevel);
                    size_t vectorSize = mFdp->ConsumeIntegralInRange<size_t>(0, kMaxVectorSize);
                    token.mac.resize(vectorSize);
                    for (size_t n = 0; n < vectorSize; ++n) {
                        token.mac[n] = mFdp->ConsumeIntegral<uint8_t>();
                    }
                    std::optional<std::vector<uint8_t>> serialized =
                            serializeVerificationToken(token);
                    if (serialized.has_value()) {
                        std::optional<VerificationToken> deserialized =
                                deserializeVerificationToken(serialized.value());
                    }
                },
                [&]() {
                    std::vector<uint8_t> dataVector;
                    size_t size = mFdp->ConsumeIntegralInRange<size_t>(0, sizeof(hw_auth_token_t));
                    dataVector = mFdp->ConsumeBytes<uint8_t>(size);
                    support::blob2hidlVec(dataVector.data(), dataVector.size());
                    support::blob2hidlVec(dataVector);
                    HardwareAuthToken authToken = support::hidlVec2AuthToken(dataVector);
                    hidl_vec<uint8_t> volatile hidlVector = support::authToken2HidlVec(authToken);
                },
                [&]() {
                    std::string str = mFdp->ConsumeRandomLengthString(kMaxCharacters);
                    support::blob2hidlVec(str);
                },
        });
        keymaster_function();
    }
    return;
}

void KeyMaster4UtilsFuzzer::process(const uint8_t* data, size_t size) {
    mFdp = std::make_unique<FuzzedDataProvider>(data, size);
    invokeKeyMasterUtils();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMaster4UtilsFuzzer kmUtilsFuzzer;
    kmUtilsFuzzer.process(data, size);
    return 0;
}

}  // namespace android::hardware::keymaster::V4_0::fuzzer
