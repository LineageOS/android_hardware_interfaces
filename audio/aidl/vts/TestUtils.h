/*
 * Copyright (C) 2022 The Android Open Source Project
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

#pragma once

#include <algorithm>
#include <initializer_list>

#include <android/binder_auto_utils.h>
#include <gtest/gtest.h>
#include <system/audio_aidl_utils.h>

namespace android::hardware::audio::common::testing {

namespace detail {

class TestExecutionTracer : public ::testing::EmptyTestEventListener {
  public:
    void OnTestStart(const ::testing::TestInfo& test_info) override;
    void OnTestEnd(const ::testing::TestInfo& test_info) override;
    void OnTestPartResult(const ::testing::TestPartResult& result) override;
  private:
    static void TraceTestState(const std::string& state, const ::testing::TestInfo& test_info);
};

inline ::testing::AssertionResult assertIsOk(const char* expr, const ::ndk::ScopedAStatus& status) {
    if (status.isOk()) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure()
           << "Expected the transaction \'" << expr << "\' to succeed\n"
           << "  but it has failed with: " << status;
}

inline ::testing::AssertionResult assertResult(const char* exp_expr, const char* act_expr,
                                               int32_t expected,
                                               const ::ndk::ScopedAStatus& status) {
    if (status.getExceptionCode() == expected) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure()
           << "Expected the transaction \'" << act_expr << "\' to fail with " << exp_expr
           << "\n  but is has completed with: " << status;
}

template <typename T>
inline ::testing::AssertionResult assertResult(const char* exp_expr, const char* act_expr,
                                               const std::initializer_list<T>& expected,
                                               const ::ndk::ScopedAStatus& status) {
    if (std::find(expected.begin(), expected.end(), status.getExceptionCode()) != expected.end()) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << "Expected the transaction \'" << act_expr
                                         << "\' to complete with one of: " << exp_expr
                                         << "\n  which is: " << ::testing::PrintToString(expected)
                                         << "\n  but is has completed with: " << status;
}

inline ::testing::AssertionResult assertIsOkOrUnknownTransaction(
        const char* expr, const ::ndk::ScopedAStatus& status) {
    if (status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        return ::testing::AssertionSuccess();
    }
    return assertIsOk(expr, status);
}

inline ::testing::AssertionResult assertResultOrUnknownTransaction(
        const char* exp_expr, const char* act_expr, int32_t expected,
        const ::ndk::ScopedAStatus& status) {
    if (status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        return ::testing::AssertionSuccess();
    }
    return assertResult(exp_expr, act_expr, expected, status);
}

}  // namespace detail

}  // namespace android::hardware::audio::common::testing

// Test that the transaction status 'isOk'
#define ASSERT_IS_OK(ret) \
    ASSERT_PRED_FORMAT1(::android::hardware::audio::common::testing::detail::assertIsOk, ret)
#define EXPECT_IS_OK(ret) \
    EXPECT_PRED_FORMAT1(::android::hardware::audio::common::testing::detail::assertIsOk, ret)

// Test that the transaction status is as expected.
#define ASSERT_STATUS(expected, ret)                                                       \
    ASSERT_PRED_FORMAT2(::android::hardware::audio::common::testing::detail::assertResult, \
                        expected, ret)
#define EXPECT_STATUS(expected, ret)                                                       \
    EXPECT_PRED_FORMAT2(::android::hardware::audio::common::testing::detail::assertResult, \
                        expected, ret)

#define SKIP_TEST_IF_DATA_UNSUPPORTED(flags)                                                     \
    ({                                                                                           \
        if ((flags).hwAcceleratorMode == Flags::HardwareAccelerator::TUNNEL || (flags).bypass) { \
            GTEST_SKIP() << "Skip data path for offload";                                        \
        }                                                                                        \
    })

// Test that the transaction status 'isOk' if it is a known transaction
#define EXPECT_IS_OK_OR_UNKNOWN_TRANSACTION(ret)                                                 \
    EXPECT_PRED_FORMAT1(                                                                         \
            ::android::hardware::audio::common::testing::detail::assertIsOkOrUnknownTransaction, \
            ret)

// Test that the transaction status is as expected if it is a known transaction
#define EXPECT_STATUS_OR_UNKNOWN_TRANSACTION(expected, ret)                                        \
    EXPECT_PRED_FORMAT2(                                                                           \
            ::android::hardware::audio::common::testing::detail::assertResultOrUnknownTransaction, \
            expected, ret)
