/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "PlatformSpecifics.h"

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <time.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace V1_0 {
namespace implementation {

MonotonicClockTimeStamper::TimeStamp MonotonicClockTimeStamper::now() {
    timespec ts;
    if (!clock_gettime(CLOCK_BOOTTIME, &ts)) {
        return TimeStamp(ts.tv_sec * UINT64_C(1000) + ts.tv_nsec / UINT64_C(1000000));
    } else {
        return {};
    }
}

support::NullOr<support::hmac_t> HMacImplementation::hmac256(
    const support::auth_token_key_t& key, std::initializer_list<support::ByteBufferProxy> buffers) {
    HMAC_CTX hmacCtx;
    HMAC_CTX_init(&hmacCtx);
    if (!HMAC_Init_ex(&hmacCtx, key.data(), key.size(), EVP_sha256(), nullptr)) {
        return {};
    }
    for (auto& buffer : buffers) {
        if (!HMAC_Update(&hmacCtx, buffer.data(), buffer.size())) {
            return {};
        }
    }
    support::hmac_t result;
    if (!HMAC_Final(&hmacCtx, result.data(), nullptr)) {
        return {};
    }
    return result;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android
