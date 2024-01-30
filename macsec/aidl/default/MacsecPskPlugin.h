/*
 * Copyright 2023, The Android Open Source Project
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

#include <aidl/android/hardware/macsec/BnMacsecPskPlugin.h>

#include <openssl/aes.h>
#include <openssl/cmac.h>

namespace aidl::android::hardware::macsec {

struct keys {
    std::vector<uint8_t> keyId;
    AES_KEY kekEncCtx;
    AES_KEY kekDecCtx;
    CMAC_CTX* ickCtx;
    CMAC_CTX* cakCtx;
};

class MacsecPskPlugin : public BnMacsecPskPlugin {
  public:
    MacsecPskPlugin();
    ~MacsecPskPlugin();
    ndk::ScopedAStatus addTestKey(const std::vector<uint8_t>& keyId,
                                  const std::vector<uint8_t>& CAK,
                                  const std::vector<uint8_t>& CKN) override;
    ndk::ScopedAStatus calcIcv(const std::vector<uint8_t>& keyId, const std::vector<uint8_t>& data,
                               std::vector<uint8_t>* out) override;

    ndk::ScopedAStatus generateSak(const std::vector<uint8_t>& keyId,
                                   const std::vector<uint8_t>& data, const int sakLength,
                                   std::vector<uint8_t>* out);

    ndk::ScopedAStatus wrapSak(const std::vector<uint8_t>& keyId, const std::vector<uint8_t>& sak,
                               std::vector<uint8_t>* out) override;

    ndk::ScopedAStatus unwrapSak(const std::vector<uint8_t>& keyId, const std::vector<uint8_t>& sak,
                                 std::vector<uint8_t>* out) override;

  private:
    std::vector<struct keys> mKeys;
};
}  // namespace aidl::android::hardware::macsec
