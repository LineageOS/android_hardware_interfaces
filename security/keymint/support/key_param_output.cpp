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

#include <keymint_support/key_param_output.h>

#include <iomanip>

#include <keymint_support/keymint_tags.h>

namespace android::hardware::security::keymint {

using ::std::endl;
using ::std::ostream;

ostream& operator<<(ostream& os, const ::std::vector<KeyParameter>& set) {
    if (set.size() == 0) {
        os << "(Empty)" << endl;
    } else {
        os << "\n";
        for (const auto& elem : set) os << elem << endl;
    }
    return os;
}

// TODO(seleneh) update this to a parsing that looks at each tags individually
// such as ALGORITHM BLOCK_MODE when aidl union support is added.
ostream& operator<<(ostream& os, const KeyParameter& param) {
    os << param.tag << ": ";
    switch (typeFromTag(param.tag)) {
        case TagType::INVALID:
            return os << " Invalid";
        case TagType::ENUM_REP:
        case TagType::ENUM:
        case TagType::UINT_REP:
        case TagType::UINT:
            return os << param.integer;
        case TagType::ULONG_REP:
        case TagType::ULONG:
        case TagType::DATE:
            return os << param.longInteger;
        case TagType::BOOL:
            return os << "true";
        case TagType::BIGNUM:
            os << " Bignum: ";
            for (size_t i = 0; i < param.blob.size(); ++i) {
                os << std::hex << ::std::setw(2) << static_cast<int>(param.blob[i]) << ::std::dec;
            }
            return os;
        case TagType::BYTES:
            os << " Bytes: ";
            for (size_t i = 0; i < param.blob.size(); ++i) {
                os << ::std::hex << ::std::setw(2) << static_cast<int>(param.blob[i]) << ::std::dec;
            }
            return os;
    }
    return os << "UNKNOWN TAG TYPE!";
}

}  // namespace android::hardware::security::keymint
