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

#pragma once

#include "MessageDefinition.h"

#include <sstream>

namespace android::nl::protocols {

template <typename T>
void arrayToStream(std::stringstream& ss, const Buffer<nlattr> attr) {
    ss << '{';
    for (const auto it : attr.data<T>().getRaw()) {
        ss << it << ',';
    }
    ss.seekp(-1, std::ios_base::cur);
    ss << '}';
}

typedef std::map<uint64_t, std::string> FlagsMap;
AttributeDefinition::ToStream flagsToStream(FlagsMap flags);
void flagsToStream(std::stringstream& ss, const FlagsMap& flags, uint64_t value);

void hwaddrToStream(std::stringstream& ss, const Buffer<nlattr> attr);

}  // namespace android::nl::protocols
