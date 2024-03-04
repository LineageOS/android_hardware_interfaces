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

#define LOG_TAG "FingerprintConfig"

#include "FingerprintConfig.h"

#include <android-base/logging.h>

#include <fingerprint.sysprop.h>

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

// Wrapper to system property access functions
#define CREATE_GETTER_SETTER_WRAPPER(_NAME_, _T_)                  \
    ConfigValue _NAME_##Getter() {                                 \
        return FingerprintHalProperties::_NAME_();                 \
    }                                                              \
    bool _NAME_##Setter(const ConfigValue& v) {                    \
        return FingerprintHalProperties::_NAME_(std::get<_T_>(v)); \
    }

CREATE_GETTER_SETTER_WRAPPER(type, OptString)
CREATE_GETTER_SETTER_WRAPPER(enrollments, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(enrollment_hit, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(next_enrollment, OptString)
CREATE_GETTER_SETTER_WRAPPER(authenticator_id, OptInt64)
CREATE_GETTER_SETTER_WRAPPER(challenge, OptInt64)
CREATE_GETTER_SETTER_WRAPPER(sensor_id, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(sensor_location, OptString)
CREATE_GETTER_SETTER_WRAPPER(sensor_strength, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_fails, OptBool)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_duration, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_error, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_authenticate_acquired, OptString)
CREATE_GETTER_SETTER_WRAPPER(operation_enroll_error, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_enroll_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_error, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_latency, OptIntVec)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_duration, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(operation_detect_interaction_acquired, OptString)
CREATE_GETTER_SETTER_WRAPPER(max_enrollments, OptBool)
CREATE_GETTER_SETTER_WRAPPER(navigation_guesture, OptBool)
CREATE_GETTER_SETTER_WRAPPER(detect_interaction, OptBool)
CREATE_GETTER_SETTER_WRAPPER(display_touch, OptBool)
CREATE_GETTER_SETTER_WRAPPER(control_illumination, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout_enable, OptBool)
CREATE_GETTER_SETTER_WRAPPER(lockout_timed_threshold, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(lockout_timed_duration, OptInt32)
CREATE_GETTER_SETTER_WRAPPER(lockout_permanent_threshold, OptInt32)

// Name, Getter, Setter, Parser and default value
#define NGS(_NAME_) #_NAME_, _NAME_##Getter, _NAME_##Setter
static Config::Data configData[] = {
        {NGS(type), &Config::parseString, "rear"},
        {NGS(enrollments), &Config::parseIntVec, ""},
        {NGS(enrollment_hit), &Config::parseInt32, "0"},
        {NGS(next_enrollment), &Config::parseString, ""},
        {NGS(authenticator_id), &Config::parseInt64, "0"},
        {NGS(challenge), &Config::parseInt64, ""},
        {NGS(sensor_id), &Config::parseInt32, "5"},
        {NGS(sensor_location), &Config::parseString, ""},
        {NGS(sensor_strength), &Config::parseInt32, "2"},  // STRONG
        {NGS(operation_authenticate_fails), &Config::parseBool, "false"},
        {NGS(operation_authenticate_latency), &Config::parseIntVec, ""},
        {NGS(operation_authenticate_duration), &Config::parseInt32, "10"},
        {NGS(operation_authenticate_error), &Config::parseInt32, "0"},
        {NGS(operation_authenticate_acquired), &Config::parseString, "1"},
        {NGS(operation_enroll_error), &Config::parseInt32, "0"},
        {NGS(operation_enroll_latency), &Config::parseIntVec, ""},
        {NGS(operation_detect_interaction_latency), &Config::parseIntVec, ""},
        {NGS(operation_detect_interaction_error), &Config::parseInt32, "0"},
        {NGS(operation_detect_interaction_duration), &Config::parseInt32, "10"},
        {NGS(operation_detect_interaction_acquired), &Config::parseString, "1"},
        {NGS(max_enrollments), &Config::parseInt32, "5"},
        {NGS(navigation_guesture), &Config::parseBool, "false"},
        {NGS(detect_interaction), &Config::parseBool, "false"},
        {NGS(display_touch), &Config::parseBool, "true"},
        {NGS(control_illumination), &Config::parseBool, "false"},
        {NGS(lockout), &Config::parseBool, "false"},
        {NGS(lockout_enable), &Config::parseBool, "false"},
        {NGS(lockout_timed_threshold), &Config::parseInt32, "5"},
        {NGS(lockout_timed_duration), &Config::parseInt32, "10000"},
        {NGS(lockout_permanent_threshold), &Config::parseInt32, "20"},
};

Config::Data* FingerprintConfig::getConfigData(int* size) {
    *size = sizeof(configData) / sizeof(configData[0]);
    return configData;
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
