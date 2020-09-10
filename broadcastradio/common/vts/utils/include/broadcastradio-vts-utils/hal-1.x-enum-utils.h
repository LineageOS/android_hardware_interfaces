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

namespace android::hardware::broadcastradio::V1_0::vts {

using android::hardware::broadcastradio::V1_0::Class;

/**
 * Convert a string of Class name to its enum value. Fail the test if the enum
 * value is not found.
 *
 * @param className string value of a Class enum.
 * @return Class enum that matches the string value.
 */
Class RadioClassFromString(std::string className) {
    if (className == "AM_FM") return Class::AM_FM;
    if (className == "SAT") return Class::SAT;
    if (className == "DT") return Class::DT;
    // Fail the test run.
    CHECK(false) << "Class name not found: " << className;
    // Return some arbitrary enum.
    return Class::AM_FM;
}
}  // namespace android::hardware::broadcastradio::V1_0::vts
