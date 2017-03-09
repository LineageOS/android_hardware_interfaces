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
#include <hidl/Status.h>

namespace detail {

inline void assertOk(::android::hardware::Return<void> ret) {
    ASSERT_TRUE(ret.isOk());
}

inline void assertOk(::android::hardware::audio::V2_0::Result result) {
    ASSERT_EQ(decltype(result)::OK, result);
}

inline void assertOk(::android::hardware::Return<::android::hardware::audio::V2_0::Result> ret) {
    ASSERT_TRUE(ret.isOk());
    ::android::hardware::audio::V2_0::Result result = ret;
    assertOk(result);
}

inline void assertInvalidArguments(::android::hardware::audio::V2_0::Result result) {
    ASSERT_EQ(decltype(result)::INVALID_ARGUMENTS, result);
}

inline void assertInvalidArguments(
        ::android::hardware::Return<::android::hardware::audio::V2_0::Result> ret) {
    ASSERT_TRUE(ret.isOk());
    ::android::hardware::audio::V2_0::Result result = ret;
    assertInvalidArguments(result);
}
}

// Test anything provided is and contains only OK
#define ASSERT_OK(ret) ASSERT_NO_FATAL_FAILURE(detail::assertOk(ret))
#define EXPECT_OK(ret) EXPECT_NO_FATAL_FAILURE(detail::assertOk(ret))

#define ASSERT_INVALID_ARGUMENTS(ret) ASSERT_NO_FATAL_FAILURE(detail::assertInvalidArguments(ret))
