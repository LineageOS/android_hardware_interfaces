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

#include <iostream>

#include <android/hardware/keymaster/4.0/types.h>

#include "keymaster_tags.h"

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {

inline ::std::ostream& operator<<(::std::ostream& os, Algorithm value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, BlockMode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, Digest value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, EcCurve value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, ErrorCode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, KeyOrigin value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, PaddingMode value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, SecurityLevel value) {
    return os << toString(value);
}

template <typename ValueT>
::std::ostream& operator<<(::std::ostream& os, const NullOr<ValueT>& value) {
    if (!value.isOk()) {
        os << "(value not present)";
    } else {
        os << value.value();
    }
    return os;
}

::std::ostream& operator<<(::std::ostream& os, const hidl_vec<KeyParameter>& set);
::std::ostream& operator<<(::std::ostream& os, const KeyParameter& value);

inline ::std::ostream& operator<<(::std::ostream& os, const KeyCharacteristics& value) {
    return os << "SW: " << value.softwareEnforced << ::std::endl
              << "HW: " << value.hardwareEnforced << ::std::endl;
}

inline ::std::ostream& operator<<(::std::ostream& os, KeyPurpose value) {
    return os << toString(value);
}

inline ::std::ostream& operator<<(::std::ostream& os, Tag tag) {
    return os << toString(tag);
}

}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
