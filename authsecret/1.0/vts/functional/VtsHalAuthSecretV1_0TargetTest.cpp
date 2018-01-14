/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <android/hardware/authsecret/1.0/IAuthSecret.h>

#include <VtsHalHidlTargetTestBase.h>

using ::android::hardware::hidl_vec;
using ::android::hardware::authsecret::V1_0::IAuthSecret;
using ::android::sp;

/**
 * There is no expected behaviour that can be tested so these tests check the
 * HAL doesn't crash with different execution orders.
 */
struct AuthSecretHidlTest : public ::testing::VtsHalHidlTargetTestBase {
    virtual void SetUp() override {
        authsecret = ::testing::VtsHalHidlTargetTestBase::getService<IAuthSecret>();
        ASSERT_NE(authsecret, nullptr);
        authsecret->factoryReset();
    }

    sp<IAuthSecret> authsecret;
};

/* Provision the primary user with a secret. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredential) {
    hidl_vec<uint8_t> secret{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    authsecret->primaryUserCredential(secret);
}

/* Provision the primary user with a large secret. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialWithLargeSecret) {
    hidl_vec<uint8_t> secret{89,  233, 52,  29,  130, 210, 229, 170, 124, 102, 56,  238, 198,
                             199, 246, 152, 185, 123, 155, 215, 29,  252, 30,  70,  118, 29,
                             149, 36,  222, 203, 163, 7,   72,  56,  247, 19,  198, 76,  71,
                             37,  120, 201, 220, 70,  150, 18,  23,  22,  236, 57,  184, 86,
                             190, 122, 210, 207, 74,  51,  222, 157, 74,  196, 86,  208};
    authsecret->primaryUserCredential(secret);
}

/* Provision the primary user with a secret and pass the secret again. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndPassAgain) {
    hidl_vec<uint8_t> secret{64, 2, 3, 0, 5, 6, 7, 172, 9, 10, 11, 255, 13, 14, 15, 83};
    authsecret->primaryUserCredential(secret);
    authsecret->primaryUserCredential(secret);
}

/* Provision the primary user with a secret and pass the secret again repeatedly. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndPassAgainMultipleTimes) {
    hidl_vec<uint8_t> secret{1, 2, 34, 4, 5, 6, 7, 8, 9, 105, 11, 12, 13, 184, 15, 16};
    authsecret->primaryUserCredential(secret);
    constexpr int N = 5;
    for (int i = 0; i < N; ++i) {
        authsecret->primaryUserCredential(secret);
    }
}

/* Factory reset before provisioning the primary user with a secret. */
TEST_F(AuthSecretHidlTest, factoryResetWithoutProvisioningPrimaryUserCredential) {
    authsecret->factoryReset();
}

/* Provision the primary user with a secret then factory reset. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndFactoryReset) {
    hidl_vec<uint8_t> secret{1, 24, 124, 240, 5, 6, 7, 8, 9, 13, 11, 12, 189, 14, 195, 16};
    authsecret->primaryUserCredential(secret);
    authsecret->factoryReset();
}

/* Provision the primary differently after factory reset. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialDifferentlyAfterFactoryReset) {
    {
        hidl_vec<uint8_t> secret1{19, 0, 65, 20, 65, 12, 7, 8, 9, 13, 29, 12, 189, 32, 195, 16};
        authsecret->primaryUserCredential(secret1);
    }

    authsecret->factoryReset();

    {
        hidl_vec<uint8_t> secret2{61, 93, 124, 240, 5, 0, 7, 201, 9, 129, 11, 12, 0, 14, 0, 16};
        authsecret->primaryUserCredential(secret2);
    }
}
