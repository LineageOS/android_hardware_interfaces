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

/** Converting between a bitfield or itself. */
template <class Enum>
class EnumBitfield {
   public:
    using Bitfield = ::android::hardware::hidl_bitfield<Enum>;

    EnumBitfield(const EnumBitfield&) = default;
    explicit EnumBitfield(Enum value) : mValue(value) {}
    explicit EnumBitfield(Bitfield value) : EnumBitfield(static_cast<Enum>(value)) {}

    EnumBitfield& operator=(const EnumBitfield&) = default;
    EnumBitfield& operator=(Enum value) { return *this = EnumBitfield{value}; }
    EnumBitfield& operator=(Bitfield value) { return *this = EnumBitfield{value}; }

    operator Enum() const { return mValue; }
    operator Bitfield() const { return static_cast<Bitfield>(mValue); }

   private:
    Enum mValue;
};

/** ATD way to create a EnumBitfield. */
template <class Enum>
EnumBitfield<Enum> mkEnumBitfield(Enum value) {
    return EnumBitfield<Enum>{value};
}

}  // namespace utils
}  // namespace common
}  // namespace audio
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_audio_common_VersionUtils_H_
