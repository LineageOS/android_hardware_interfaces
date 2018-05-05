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

#ifndef AUDIO_HAL_VERSION
#error "AUDIO_HAL_VERSION must be set before including this file."
#endif

#ifndef ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_PRETTY_PRINT_AUDIO_TYPES_H
#define ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_PRETTY_PRINT_AUDIO_TYPES_H

#include <iosfwd>
#include <utility>

/** @file Use HIDL generated toString methods to pretty print gtest errors
 *        Unfortunately Gtest does not offer a template to specialize, only
 *        overloading PrintTo.
 *  @note that this overload can NOT be template because
 *        the fallback is already template, resulting in ambiguity.
 *  @note that the overload MUST be in the exact namespace
 *        the type is declared in, as per the ADL rules.
 */

namespace android {
namespace hardware {
namespace audio {

#define DEFINE_GTEST_PRINT_TO(T) \
    inline void PrintTo(const T& val, ::std::ostream* os) { *os << toString(val); }

namespace AUDIO_HAL_VERSION {
DEFINE_GTEST_PRINT_TO(IPrimaryDevice::TtyMode)
DEFINE_GTEST_PRINT_TO(Result)
}  // namespace AUDIO_HAL_VERSION

namespace common {
namespace AUDIO_HAL_VERSION {
DEFINE_GTEST_PRINT_TO(AudioConfig)
DEFINE_GTEST_PRINT_TO(AudioMode)
DEFINE_GTEST_PRINT_TO(AudioDevice)
DEFINE_GTEST_PRINT_TO(AudioFormat)
DEFINE_GTEST_PRINT_TO(AudioChannelMask)
}  // namespace AUDIO_HAL_VERSION
}  // namespace common

#undef DEFINE_GTEST_PRINT_TO

}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_AUDIO_COMMON_TEST_UTILITY_PRETTY_PRINT_AUDIO_TYPES_H
