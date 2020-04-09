/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "drm_hal_test@1.3"

#include "android/hardware/drm/1.3/vts/drm_hal_test.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_3 {
namespace vts {

TEST_P(DrmHalTestV1_3, SchemeSupported) {
    EXPECT_TRUE(drmFactory_->isCryptoSchemeSupported(GetParam().scheme_));
}

TEST_P(DrmHalTestV1_3, SignRsaNotAllowed) {
    hidl_array<uint8_t, 16> kWidevineUUID ({
        0xED,0xEF,0x8B,0xA9,0x79,0xD6,0x4A,0xCE,
        0xA3,0xC8,0x27,0xDC,0xD5,0x1D,0x21,0xED
    });

    if (!drmFactory_->isCryptoSchemeSupported(kWidevineUUID)) {
        GTEST_SKIP() << "Widevine only test";
    }

    // signRSA
    const hidl_vec<uint8_t>& sessionId{0};
    const hidl_string& algorithm{"RSASSA-PSS-SHA1"};
    const hidl_vec<uint8_t>& message{0};
    const hidl_vec<uint8_t>& wrappedKey{0};
    auto res = drmPlugin_->signRSA(
        sessionId, algorithm, message, wrappedKey,
        [&](StatusV1_0 status, const hidl_vec<uint8_t>& signature) {
            EXPECT_EQ(status, StatusV1_0::ERROR_DRM_UNKNOWN);
            EXPECT_EQ(signature.size(), 0);
        }
    );
    EXPECT_TRUE(res.isOk());
}

}  // namespace vts
}  // namespace V1_3
}  // namespace drm
}  // namespace hardware
}  // namespace android
