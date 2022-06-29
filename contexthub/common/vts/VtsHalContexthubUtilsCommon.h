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

/**
 * Utils file for any Context Hub VTS code (i.e. not specific to e.g. HIDL).
 */

#pragma once

#include <chrono>
#include <future>

namespace android {
namespace hardware {
namespace contexthub {
namespace vts_utils {

// App ID with vendor "GoogT" (Google Testing), app identifier 0x555555. This
// app ID is reserved and must never appear in the list of loaded apps.
constexpr uint64_t kNonExistentAppId = 0x476f6f6754555555;

// Helper that does explicit conversion of an enum class to its underlying/base
// type. Useful for stream output of enum values.
template <typename EnumType>
inline constexpr typename std::underlying_type<EnumType>::type asBaseType(EnumType value) {
    return static_cast<typename std::underlying_type<EnumType>::type>(value);
}

// Wait for a callback to occur (signaled by the given future) up to the
// provided timeout. If the future is invalid or the callback does not come
// within the given time, returns false.
template <class ReturnType>
bool waitForCallback(std::future<ReturnType> future, ReturnType* result,
                     std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    auto expiration = std::chrono::system_clock::now() + timeout;

    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(future.valid());
    if (result != nullptr && future.valid()) {
        std::future_status status = future.wait_until(expiration);
        EXPECT_NE(status, std::future_status::timeout) << "Timed out waiting for callback";

        if (status == std::future_status::ready) {
            *result = future.get();
            return true;
        }
    }

    return false;
}

}  // namespace vts_utils
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
