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
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::hardware::hidl_vec;
using ::android::hardware::authsecret::V1_0::IAuthSecret;
using ::android::sp;

// Test environment for Boot HIDL HAL.
class AuthSecretHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static AuthSecretHidlEnvironment* Instance() {
        static AuthSecretHidlEnvironment* instance = new AuthSecretHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IAuthSecret>(); }

   private:
    AuthSecretHidlEnvironment() {}
};

/**
 * There is no expected behaviour that can be tested so these tests check the
 * HAL doesn't crash with different execution orders.
 */
struct AuthSecretHidlTest : public ::testing::VtsHalHidlTargetTestBase {
    virtual void SetUp() override {
        authsecret = ::testing::VtsHalHidlTargetTestBase::getService<IAuthSecret>(
            AuthSecretHidlEnvironment::Instance()->getServiceName<IAuthSecret>());
        ASSERT_NE(authsecret, nullptr);

        // All tests must enroll the correct secret first as this cannot be changed
        // without a factory reset and the order of tests could change.
        authsecret->primaryUserCredential(CORRECT_SECRET);
    }

    sp<IAuthSecret> authsecret;
    hidl_vec<uint8_t> CORRECT_SECRET{61, 93, 124, 240, 5, 0, 7, 201, 9, 129, 11, 12, 0, 14, 0, 16};
    hidl_vec<uint8_t> WRONG_SECRET{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
};

/* Provision the primary user with a secret. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredential) {
    // Secret provisioned by SetUp()
}

/* Provision the primary user with a secret and pass the secret again. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndPassAgain) {
    // Secret provisioned by SetUp()
    authsecret->primaryUserCredential(CORRECT_SECRET);
}

/* Provision the primary user with a secret and pass the secret again repeatedly. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndPassAgainMultipleTimes) {
    // Secret provisioned by SetUp()
    constexpr int N = 5;
    for (int i = 0; i < N; ++i) {
        authsecret->primaryUserCredential(CORRECT_SECRET);
    }
}

/* Provision the primary user with a secret and then pass the wrong secret. This
 * should never happen and is an framework bug if it does. As the secret is
 * wrong, the HAL implementation may not be able to function correctly but it
 * should fail gracefully. */
TEST_F(AuthSecretHidlTest, provisionPrimaryUserCredentialAndWrongSecret) {
    // Secret provisioned by SetUp()
    authsecret->primaryUserCredential(WRONG_SECRET);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(AuthSecretHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    AuthSecretHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
