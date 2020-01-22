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

using std::string;

using EarlyBootKeyTest = Keymaster4_1HidlTest;

// Because VTS tests are run on fully-booted machines, we can only run negative tests for early boot
// keys, which cannot be created or used after /data is mounted.  This is the only test we can run
// in the normal case.  The positive test will have to be done by the Android system, when it
// creates/uses early boot keys during boot.  It should fail to boot if the early boot key usage
// fails.
TEST_P(EarlyBootKeyTest, CannotCreateEarlyBootKeys) {
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::EARLY_BOOT_ENDED);

    CheckedDeleteKeyData(&aesKeyData);
    CheckedDeleteKeyData(&hmacKeyData);
    CheckedDeleteKeyData(&rsaKeyData);
    CheckedDeleteKeyData(&ecdsaKeyData);
}

// This is a more comprenhensive test, but it can only be run on a machine which is still in early
// boot stage, which no proper Android device is by the time we can run VTS.  To use this,
// un-disable it and modify vold to remove the call to earlyBootEnded().  Running the test will end
// early boot, so you'll have to reboot between runs.
TEST_P(EarlyBootKeyTest, DISABLED_FullTest) {
    // Should be able to create keys, since early boot has not ended
    auto [aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData] =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::OK);

    // TAG_EARLY_BOOT_ONLY should be in hw-enforced.
    EXPECT_TRUE(contains(aesKeyData.characteristics.hardwareEnforced, TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(contains(hmacKeyData.characteristics.hardwareEnforced, TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(contains(rsaKeyData.characteristics.hardwareEnforced, TAG_EARLY_BOOT_ONLY));
    EXPECT_TRUE(contains(ecdsaKeyData.characteristics.hardwareEnforced, TAG_EARLY_BOOT_ONLY));

    // Should be able to use keys, since early boot has not ended
    EXPECT_EQ(ErrorCode::OK, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::OK, UseEcdsaKey(ecdsaKeyData.blob));

    // End early boot
    Return<ErrorCode> earlyBootResult = keymaster().earlyBootEnded();
    EXPECT_TRUE(earlyBootResult.isOk());
    EXPECT_EQ(earlyBootResult, ErrorCode::OK);

    // Should not be able to use already-created keys.
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseAesKey(aesKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseHmacKey(hmacKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseRsaKey(rsaKeyData.blob));
    EXPECT_EQ(ErrorCode::EARLY_BOOT_ENDED, UseEcdsaKey(ecdsaKeyData.blob));

    CheckedDeleteKeyData(&aesKeyData);
    CheckedDeleteKeyData(&hmacKeyData);
    CheckedDeleteKeyData(&rsaKeyData);
    CheckedDeleteKeyData(&ecdsaKeyData);

    // Should not be able to create new keys
    std::tie(aesKeyData, hmacKeyData, rsaKeyData, ecdsaKeyData) =
            CreateTestKeys(TAG_EARLY_BOOT_ONLY, ErrorCode::EARLY_BOOT_ENDED);

    CheckedDeleteKeyData(&aesKeyData);
    CheckedDeleteKeyData(&hmacKeyData);
    CheckedDeleteKeyData(&rsaKeyData);
    CheckedDeleteKeyData(&ecdsaKeyData);
}

INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(EarlyBootKeyTest);

}  // namespace android::hardware::keymaster::V4_1::test
