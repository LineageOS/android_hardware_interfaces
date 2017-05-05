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

#include <algorithm>
#include <vector>

#include <hidl/Status.h>

namespace detail {

// This is a detail namespace, thus it is OK to import a class as nobody else is
// allowed to use it
using ::android::hardware::Return;
using ::android::hardware::audio::V2_0::Result;

inline void assertResult(Result expected, Result result) {
    ASSERT_EQ(expected, result);
}

inline void assertResult(Result expected, const Return<Result>& ret) {
    ASSERT_TRUE(ret.isOk());
    Result result = ret;
    assertResult(expected, result);
}

inline void assertResult(const std::vector<Result>& expected, Result result) {
    if (std::find(expected.begin(), expected.end(), result) != expected.end()) {
        return;  // result is in expected
    }
    FAIL() << "Expected result " << ::testing::PrintToString(result)
           << " to be one of " << ::testing::PrintToString(expected);
}

inline void assertResult(const std::vector<Result>& expected,
                         const Return<Result>& ret) {
    ASSERT_TRUE(ret.isOk());
    Result result = ret;
    assertResult(expected, result);
}

inline void assertOk(const Return<void>& ret) {
    ASSERT_TRUE(ret.isOk());
}

inline void assertOk(Result result) {
    assertResult(Result::OK, result);
}

inline void assertOk(const Return<Result>& ret) {
    assertResult(Result::OK, ret);
}
}

// Test anything provided is and contains only OK
#define ASSERT_OK(ret) ASSERT_NO_FATAL_FAILURE(detail::assertOk(ret))
#define EXPECT_OK(ret) EXPECT_NO_FATAL_FAILURE(detail::assertOk(ret))

#define ASSERT_RESULT(expected, ret) \
    ASSERT_NO_FATAL_FAILURE(detail::assertResult(expected, ret))
#define EXPECT_RESULT(expected, ret) \
    EXPECT_NO_FATAL_FAILURE(detail::assertResult(expected, ret))
