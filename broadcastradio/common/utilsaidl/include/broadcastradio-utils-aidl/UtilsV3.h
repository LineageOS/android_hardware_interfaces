/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <aidl/android/hardware/broadcastradio/ProgramInfo.h>

namespace aidl::android::hardware::broadcastradio {

namespace utils {

bool parseAlertStatus(const std::string& s, AlertStatus& out);

bool parseAlertMessageType(const std::string& s, AlertMessageType& out);

bool parseAlertCategory(const std::string& s, AlertCategory& out);

bool parseAlertUrgency(const std::string& s, AlertUrgency& out);

bool parseAlertSeverity(const std::string& s, AlertSeverity& out);

bool parseAlertCertainty(const std::string& s, AlertCertainty& out);
}  // namespace utils
}  // namespace aidl::android::hardware::broadcastradio