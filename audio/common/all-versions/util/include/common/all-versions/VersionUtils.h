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

#ifndef android_hardware_audio_common_VersionUtils_H_
#define android_hardware_audio_common_VersionUtils_H_

#include <hidl/HidlSupport.h>
#include <type_traits>

namespace android {
namespace hardware {
namespace audio {
namespace common {
namespace utils {

/** Similar to static_cast but also casts to hidl_bitfield depending on
 * return type inference (emulated through user-define conversion).
 */
template <class Source, class Destination = Source>
class EnumConverter {
   public:
    static_assert(std::is_enum<Source>::value || std::is_enum<Destination>::value,
                  "Source or destination should be an enum");

    explicit EnumConverter(Source source) : mSource(source) {}

    operator Destination() const { return static_cast<Destination>(mSource); }

    template <class = std::enable_if_t<std::is_enum<Destination>::value>>
    operator ::android::hardware::hidl_bitfield<Destination>() {
        return static_cast<std::underlying_type_t<Destination>>(mSource);
    }

   private:
    const Source mSource;
};
template <class Destination, class Source>
auto mkEnumConverter(Source source) {
    return EnumConverter<Source, Destination>{source};
}

/** Allows converting an enum to its bitfield or itself. */
template <class Enum>
EnumConverter<Enum> mkBitfield(Enum value) {
    return EnumConverter<Enum>{value};
}

}  // namespace utils
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_audio_common_VersionUtils_H_
