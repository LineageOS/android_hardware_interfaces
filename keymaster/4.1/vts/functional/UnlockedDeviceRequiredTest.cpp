/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "Keymaster4_1HidlTest.h"

#include <keymasterV4_1/authorization_set.h>

namespace android::hardware::keymaster::V4_1::test {

using UnlockedDeviceRequiredTest = Keymaster4_1HidlTest;

// This may be a problematic test.  It can't be run repeatedly without unlocking the device in
// between runs... and on most test devices there are no enrolled credentials so it can't be
// unlocked at all, meaning the only way to get the test to pass again on a properly-functioning
// device is to reboot it.  For that reason, this is disabled by default.  It can be used as part of
// a manual test process, which includes unlocking between runs, which is why it's included here.
// Well, that and the fact that it's the only test we can do without also making calls into the
// Gatekeeper HAL.  We haven't written any cross-HAL tests, and don't know what all of the
// implications might be, so that may or may not be a solution.
//
// TODO(swillden): Use the Gatekeeper HAL to enroll some test credentials which we can verify to get
// an unlock auth token.  If that works, enable the improved test.
TEST_P(UnlockedDeviceRequiredTest, DISABLED_KeysBecomeUnusable) {
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_UNLOCKED_DEVICE_REQUIRED, ErrorCode::OK);

    EXPECT_EQ(ErrorCode::OK, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseEcdsaKey(ecdsaKeyData.blob));

    Return<ErrorCode> rc =
            keymaster().deviceLocked(false /* passwordOnly */, {} /* verificationToken */);
    ASSERT_TRUE(rc.isOk());
    ASSERT_EQ(ErrorCode::OK, static_cast<ErrorCode>(rc));

    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::DEVICE_LOCKED, UseEcdsaKey(ecdsaKeyData.blob));

    CheckedDeleteKeyData(&aesKeyData);
    CheckedDeleteKeyData(&hmacKeyData);
    CheckedDeleteKeyData(&rsaKeyData);
    CheckedDeleteKeyData(&ecdsaKeyData);
}

INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(UnlockedDeviceRequiredTest);

}  // namespace android::hardware::keymaster::V4_1::test
