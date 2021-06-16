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
#define LOG_TAG "secureclock_test"
#include <android-base/logging.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/secureclock/ISecureClock.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <vector>

namespace aidl::android::hardware::security::secureclock::test {
using Status = ::ndk::ScopedAStatus;
using ::aidl::android::hardware::security::keymint::ErrorCode;
using ::std::shared_ptr;
using ::std::string;
using ::std::vector;

class SecureClockAidlTest : public ::testing::TestWithParam<string> {
  public:
    struct TimestampTokenResult {
        ErrorCode error;
        TimeStampToken token;
    };

    TimestampTokenResult getTimestampToken(int64_t in_challenge) {
        TimestampTokenResult result;
        result.error =
                GetReturnErrorCode(secureClock_->generateTimeStamp(in_challenge, &result.token));
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

    ErrorCode GetReturnErrorCode(const Status& result) {
        if (result.isOk()) return ErrorCode::OK;

        if (result.getExceptionCode() == EX_SERVICE_SPECIFIC) {
            return static_cast<ErrorCode>(result.getServiceSpecificError());
        }

        return ErrorCode::UNKNOWN_ERROR;
    }

    void InitializeSecureClock(std::shared_ptr<ISecureClock> secureClock) {
        ASSERT_NE(secureClock, nullptr);
        secureClock_ = secureClock;
    }

    ISecureClock& secureClock() { return *secureClock_; }

    static vector<string> build_params() {
        auto params = ::android::getAidlHalInstanceNames(ISecureClock::descriptor);
        return params;
    }

    void SetUp() override {
        if (AServiceManager_isDeclared(GetParam().c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
            InitializeSecureClock(ISecureClock::fromBinder(binder));
        } else {
            InitializeSecureClock(nullptr);
        }
    }

    void TearDown() override {}

  private:
    std::shared_ptr<ISecureClock> secureClock_;
};

/*
 * The precise capabilities required to generate TimeStampToken will vary depending on the specific
 * vendor implementations. The only thing we really can test is that tokens can be created by
 * secureclock services, and that the timestamps increase as expected.
 */
TEST_P(SecureClockAidlTest, TestCreation) {
    auto result1 = getTimestampToken(1 /* challenge */);
    auto result1_time = getTime();
    EXPECT_EQ(ErrorCode::OK, result1.error);
    EXPECT_EQ(1U, result1.token.challenge);
    EXPECT_GT(result1.token.timestamp.milliSeconds, 0U);
    EXPECT_EQ(32U, result1.token.mac.size());

    unsigned long time_to_sleep = 200;
    sleep_ms(time_to_sleep);

    auto result2 = getTimestampToken(2 /* challenge */);
    auto result2_time = getTime();
    EXPECT_EQ(ErrorCode::OK, result2.error);
    EXPECT_EQ(2U, result2.token.challenge);
    EXPECT_GT(result2.token.timestamp.milliSeconds, 0U);
    EXPECT_EQ(32U, result2.token.mac.size());

    auto host_time_delta = result2_time - result1_time;

    EXPECT_GE(host_time_delta, time_to_sleep)
            << "We slept for " << time_to_sleep << " ms, the clock must have advanced by that much";
    EXPECT_LE(host_time_delta, time_to_sleep + 100)
            << "The getTimestampToken call took " << (host_time_delta - time_to_sleep)
            << " ms?  That's awful!";
    EXPECT_GE(result2.token.timestamp.milliSeconds, result1.token.timestamp.milliSeconds);
    unsigned long km_time_delta =
            result2.token.timestamp.milliSeconds - result1.token.timestamp.milliSeconds;
    // 20 ms of slop just to avoid test flakiness.
    EXPECT_LE(host_time_delta, km_time_delta + 20);
    EXPECT_LE(km_time_delta, host_time_delta + 20);
    ASSERT_EQ(result1.token.mac.size(), result2.token.mac.size());
    ASSERT_NE(0,
              memcmp(result1.token.mac.data(), result2.token.mac.data(), result1.token.mac.size()));
}

/*
 * Test that the mac changes when the time stamp changes. This is does not guarantee that the time
 * stamp is included in the mac but on failure we know that it is not. Other than in the test
 * case above we call getTimestampToken with the exact same set of parameters.
 */
TEST_P(SecureClockAidlTest, MacChangesOnChangingTimestamp) {
    auto result1 = getTimestampToken(0 /* challenge */);
    auto result1_time = getTime();
    EXPECT_EQ(ErrorCode::OK, result1.error);
    EXPECT_EQ(0U, result1.token.challenge);
    EXPECT_GT(result1.token.timestamp.milliSeconds, 0U);
    EXPECT_EQ(32U, result1.token.mac.size());

    unsigned long time_to_sleep = 200;
    sleep_ms(time_to_sleep);

    auto result2 = getTimestampToken(1 /* challenge */);
    auto result2_time = getTime();
    EXPECT_EQ(ErrorCode::OK, result2.error);
    EXPECT_EQ(1U, result2.token.challenge);
    EXPECT_GT(result2.token.timestamp.milliSeconds, 0U);
    EXPECT_EQ(32U, result2.token.mac.size());

    auto host_time_delta = result2_time - result1_time;

    EXPECT_GE(host_time_delta, time_to_sleep)
            << "We slept for " << time_to_sleep << " ms, the clock must have advanced by that much";
    EXPECT_LE(host_time_delta, time_to_sleep + 100)
            << "The getTimestampToken call took " << (host_time_delta - time_to_sleep)
            << " ms?  That's awful!";

    EXPECT_GE(result2.token.timestamp.milliSeconds, result1.token.timestamp.milliSeconds);
    unsigned long km_time_delta =
            result2.token.timestamp.milliSeconds - result1.token.timestamp.milliSeconds;

    EXPECT_LE(host_time_delta, km_time_delta + 20);
    EXPECT_LE(km_time_delta, host_time_delta + 20);
    ASSERT_EQ(result1.token.mac.size(), result2.token.mac.size());
    ASSERT_NE(0,
              memcmp(result1.token.mac.data(), result2.token.mac.data(), result1.token.mac.size()));
}

INSTANTIATE_TEST_SUITE_P(PerInstance, SecureClockAidlTest,
                         testing::ValuesIn(SecureClockAidlTest::build_params()),
                         ::android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SecureClockAidlTest);

}  // namespace aidl::android::hardware::security::secureclock::test

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
