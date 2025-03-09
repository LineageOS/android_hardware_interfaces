/*
 * Copyright (C) 2024 The Android Open Source Project
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
#ifndef VIBRATOR_HAL_TEST_UTILS_H
#define VIBRATOR_HAL_TEST_UTILS_H

#include <android/binder_auto_utils.h>
#include <gtest/gtest.h>

#if !defined(EXPECT_OK)
#define EXPECT_OK(expression)                                                \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                            \
    if (const ::ndk::ScopedAStatus&& _status = (expression); _status.isOk()) \
        ;                                                                    \
    else                                                                     \
        ADD_FAILURE() << "Expected STATUS_OK for: " << #expression << "\n  Actual: " << _status
#else
#error Macro EXPECT_OK already defined unexpectedly
#endif

#if !defined(EXPECT_UNKNOWN_OR_UNSUPPORTED)
#define EXPECT_UNKNOWN_OR_UNSUPPORTED(expression)                                                \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                                                \
    if (const ::ndk::ScopedAStatus&& _status = (expression);                                     \
        _status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||                                \
        _status.getStatus() == STATUS_UNKNOWN_TRANSACTION)                                       \
        ;                                                                                        \
    else                                                                                         \
        ADD_FAILURE() << "Expected STATUS_UNKNOWN_TRANSACTION or EX_UNSUPPORTED_OPERATION for: " \
                      << #expression << "\n  Actual: " << _status
#else
#error Macro EXPECT_UNKNOWN_OR_UNSUPPORTED already defined unexpectedly
#endif

#if !defined(EXPECT_ILLEGAL_ARGUMENT)
#define EXPECT_ILLEGAL_ARGUMENT(expression)                                  \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                            \
    if (const ::ndk::ScopedAStatus&& _status = (expression);                 \
        _status.getExceptionCode() == EX_ILLEGAL_ARGUMENT)                   \
        ;                                                                    \
    else                                                                     \
        ADD_FAILURE() << "Expected EX_ILLEGAL_ARGUMENT for: " << #expression \
                      << "\n  Actual: " << _status
#else
#error Macro EXPECT_ILLEGAL_ARGUMENT already defined unexpectedly
#endif

#if !defined(EXPECT_ILLEGAL_STATE)
#define EXPECT_ILLEGAL_STATE(expression)                                  \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                         \
    if (const ::ndk::ScopedAStatus&& _status = (expression);              \
        _status.getExceptionCode() == EX_ILLEGAL_STATE)                   \
        ;                                                                 \
    else                                                                  \
        ADD_FAILURE() << "Expected EX_ILLEGAL_STATE for: " << #expression \
                      << "\n  Actual: " << _status
#else
#error Macro EXPECT_ILLEGAL_STATE already defined unexpectedly
#endif

#endif  // VIBRATOR_HAL_TEST_UTILS_H
