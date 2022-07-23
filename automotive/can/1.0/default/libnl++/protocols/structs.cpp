/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "structs.h"

#include <iomanip>

namespace android::nl::protocols {

AttributeDefinition::ToStream flagsToStream(FlagsMap flags) {
    return [flags](std::stringstream& ss, const Buffer<nlattr> attr) {
        auto value = attr.data<uint64_t>().copyFirst();
        flagsToStream(ss, flags, value);
    };
}

void flagsToStream(std::stringstream& ss, const FlagsMap& flags, uint64_t val) {
    bool first = true;
    for (const auto& [flag, name] : flags) {
        if ((val & flag) != flag) continue;
        val &= ~flag;

        if (!first) ss << '|';
        first = false;

        ss << name;
    }

    if (val == 0) return;

    if (!first) ss << '|';
    ss << std::hex << val << std::dec;
}

void hwaddrToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    ss << std::hex;
    bool first = true;
    for (const auto byte : attr.data<uint8_t>().getRaw()) {
        if (!first) ss << ':';
        first = false;

        ss << std::setw(2) << unsigned(byte);
    }
    ss << std::dec;
}

}  // namespace android::nl::protocols
