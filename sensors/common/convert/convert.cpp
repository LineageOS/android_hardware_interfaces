/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <sensors/common_convert.h>
#include <cstring>

namespace android {
namespace hardware {
namespace sensors {
namespace implementation {
namespace common {

sensors_event_t convertASensorEvent(const ASensorEvent& src) {
    // Attempt to ensure these types are compatible.
    static_assert(sizeof(sensors_event_t) == sizeof(ASensorEvent));
    static_assert(offsetof(sensors_event_t, timestamp) == offsetof(ASensorEvent, timestamp));
    static_assert(offsetof(sensors_event_t, flags) == offsetof(ASensorEvent, flags));

    // TODO(b/259711109) Follow up work to handle this in a safer way.
    return *reinterpret_cast<const sensors_event_t*>(&src);
}

}  // namespace common
}  // namespace implementation
}  // namespace sensors
}  // namespace hardware
}  // namespace android
