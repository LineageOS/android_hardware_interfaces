/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "KeymasterHidlTest.h"

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace test {

class VerificationTokenTest : public KeymasterHidlTest {
   protected:
    struct VerifyAuthorizationResult {
        bool callSuccessful;
        ErrorCode error;
        VerificationToken token;
    };

    VerifyAuthorizationResult verifyAuthorization(uint64_t operationHandle,
                                                  const AuthorizationSet& paramsToVerify,
                                                  const HardwareAuthToken& authToken) {
        VerifyAuthorizationResult result;
        result.callSuccessful =
            keymaster()
                .verifyAuthorization(operationHandle, paramsToVerify.hidl_data(), authToken,
                                     [&](auto error, auto token) {
                                         result.error = error;
                                         result.token = token;
                                     })
                .isOk();
        return result;
    }

    uint64_t getTime() {
        struct timespec timespec;
        EXPECT_EQ(0, clock_gettime(CLOCK_BOOTTIME, &timespec));
        return timespec.tv_sec * 1000 + timespec.tv_nsec / 1000000;
    }

    int sleep_ms(uint32_t milliseconds) {
        struct timespec sleep_time = {static_cast<time_t>(milliseconds / 1000),
                                      static_cast<long>(milliseconds % 1000) * 1000000};
        while (sleep_time.tv_sec || sleep_time.tv_nsec) {
            if (nanosleep(&sleep_time /* to wait */,
                          &sleep_time /* remaining (on interrruption) */) == 0) {
                sleep_time = {};
            } else {
                if (errno != EINTR) return errno;
            }
        }
        return 0;
    }

};  // namespace test

/*
 * VerificationTokens exist to facilitate cross-Keymaster verification of requirements.  As
 * such, the precise capabilities required will vary depending on the specific vendor
 * implementations. Essentially, VerificationTokens are a "hook" to enable vendor
 * implementations to communicate, so the precise usage is defined by those vendors.  The only
 * thing we really can test is that tokens can be created by TEE keymasters, and that the
 * timestamps increase as expected.
 */
TEST_F(VerificationTokenTest, TestCreation) {
    auto result1 = verifyAuthorization(
        1 /* operation handle */, AuthorizationSet() /* paramtersToVerify */, HardwareAuthToken());
    ASSERT_TRUE(result1.callSuccessful);
    auto result1_time = getTime();

    if (SecLevel() == SecurityLevel::STRONGBOX) {
        // StrongBox should not implement verifyAuthorization.
        EXPECT_EQ(ErrorCode::UNIMPLEMENTED, result1.error);
        return;
    }

    EXPECT_EQ(ErrorCode::OK, result1.error);
    EXPECT_EQ(1U, result1.token.challenge);
    EXPECT_EQ(SecLevel(), result1.token.securityLevel);
    EXPECT_EQ(0U, result1.token.parametersVerified.size())
        << "We didn't supply any parameters to verify";
    EXPECT_GT(result1.token.timestamp, 0U);

    constexpr uint32_t time_to_sleep = 200;
    sleep_ms(time_to_sleep);

    auto result2 = verifyAuthorization(
        2 /* operation handle */, AuthorizationSet() /* paramtersToVerify */, HardwareAuthToken());
    ASSERT_TRUE(result2.callSuccessful);
    auto result2_time = getTime();
    EXPECT_EQ(ErrorCode::OK, result2.error);
    EXPECT_EQ(2U, result2.token.challenge);
    EXPECT_EQ(SecLevel(), result2.token.securityLevel);
    EXPECT_EQ(0U, result2.token.parametersVerified.size())
        << "We didn't supply any parameters to verify";

    auto host_time_delta = result2_time - result1_time;

    EXPECT_GE(host_time_delta, time_to_sleep)
        << "We slept for " << time_to_sleep << " ms, the clock must have advanced by that much";
    EXPECT_LE(host_time_delta, time_to_sleep + 20)
        << "The verifyAuthorization call took " << (host_time_delta - time_to_sleep)
        << " ms?  That's awful!";

    auto km_time_delta = result2.token.timestamp - result1.token.timestamp;

    // If not too much else is going on on the system, the time delta should be quite close.  Allow
    // 2 ms of slop just to avoid test flakiness.
    //
    // TODO(swillden): see if we can output values so they can be gathered across many runs and
    // report if times aren't nearly always <1ms apart.
    EXPECT_LE(host_time_delta, km_time_delta + 2);
    EXPECT_LE(km_time_delta, host_time_delta + 2);
}

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
