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

#include "Empty.h"

namespace android::nl::protocols::base {

// clang-format off
Empty::Empty() : MessageDefinition<char>("nlmsg", {
    {NLMSG_NOOP, {"NOOP", MessageGenre::Unknown}},
    {NLMSG_DONE, {"DONE", MessageGenre::Unknown}},
    {NLMSG_OVERRUN, {"OVERRUN", MessageGenre::Unknown}},
}) {}
// clang-format on

void Empty::toStream(std::stringstream&, const char&) const {}

}  // namespace android::nl::protocols::base
