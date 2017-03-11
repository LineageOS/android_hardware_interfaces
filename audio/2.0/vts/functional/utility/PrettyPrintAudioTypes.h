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
#include <type_traits>

/** @file Use HIDL generated toString methods to pretty print gtest errors */

namespace detail {

// Print the value of an enum as hex
template <class Enum>
inline void printUnderlyingValue(Enum value, ::std::ostream* os) {
    *os << std::hex << " (0x" << static_cast<std::underlying_type_t<Enum>>(value) << ")";
}

} // namespace detail

namespace android {
namespace hardware {
namespace audio {
namespace V2_0 {

inline void PrintTo(const Result& result, ::std::ostream* os) {
    *os << toString(result);
    detail::printUnderlyingValue(result, os);
}

} // namespace V2_0
namespace common {
namespace V2_0 {

inline void PrintTo(const AudioConfig& config, ::std::ostream* os) {
    *os << toString(config);
}

inline void PrintTo(const AudioDevice& device, ::std::ostream* os) {
    *os << toString(device);
    detail::printUnderlyingValue(device, os);
}

inline void PrintTo(const AudioChannelMask& channelMask, ::std::ostream* os) {
    *os << toString(channelMask);
    detail::printUnderlyingValue(channelMask, os);
}

} // namespace V2_0
} // namespace common
} // namespace audio
} // namespace hardware
} // namespace android
