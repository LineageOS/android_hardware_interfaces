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

#define LOG_TAG "BcRadioAidlDef.utilsV3"

#include "broadcastradio-utils-aidl/Utils.h"

#include <android-base/strings.h>

namespace aidl::android::hardware::broadcastradio {

namespace utils {

namespace {

using ::android::base::EqualsIgnoreCase;
using ::std::vector;
}  // namespace

bool parseAlertStatus(const std::string& s, AlertStatus& out) {
    if (EqualsIgnoreCase(s, toString(AlertStatus::ACTUAL))) {
        out = AlertStatus::ACTUAL;
    } else if (EqualsIgnoreCase(s, toString(AlertStatus::EXERCISE))) {
        out = AlertStatus::EXERCISE;
    } else if (EqualsIgnoreCase(s, toString(AlertStatus::TEST))) {
        out = AlertStatus::TEST;
    } else {
        return false;
    }
    return true;
}

bool parseAlertMessageType(const std::string& s, AlertMessageType& out) {
    if (EqualsIgnoreCase(s, toString(AlertMessageType::ALERT))) {
        out = AlertMessageType::ALERT;
    } else if (EqualsIgnoreCase(s, toString(AlertMessageType::UPDATE))) {
        out = AlertMessageType::UPDATE;
    } else if (EqualsIgnoreCase(s, toString(AlertMessageType::CANCEL))) {
        out = AlertMessageType::CANCEL;
    } else {
        return false;
    }
    return true;
}

bool parseAlertCategory(const std::string& s, AlertCategory& out) {
    if (EqualsIgnoreCase(s, toString(AlertCategory::GEO))) {
        out = AlertCategory::GEO;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::MET))) {
        out = AlertCategory::MET;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::SAFETY))) {
        out = AlertCategory::SAFETY;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::SECURITY))) {
        out = AlertCategory::SECURITY;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::RESCUE))) {
        out = AlertCategory::RESCUE;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::FIRE))) {
        out = AlertCategory::FIRE;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::HEALTH))) {
        out = AlertCategory::HEALTH;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::ENV))) {
        out = AlertCategory::ENV;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::TRANSPORT))) {
        out = AlertCategory::TRANSPORT;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::INFRA))) {
        out = AlertCategory::INFRA;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::CBRNE))) {
        out = AlertCategory::CBRNE;
    } else if (EqualsIgnoreCase(s, toString(AlertCategory::OTHER))) {
        out = AlertCategory::OTHER;
    } else {
        return false;
    }
    return true;
}

bool parseAlertUrgency(const std::string& s, AlertUrgency& out) {
    if (EqualsIgnoreCase(s, toString(AlertUrgency::IMMEDIATE))) {
        out = AlertUrgency::IMMEDIATE;
    } else if (EqualsIgnoreCase(s, toString(AlertUrgency::EXPECTED))) {
        out = AlertUrgency::EXPECTED;
    } else if (EqualsIgnoreCase(s, toString(AlertUrgency::FUTURE))) {
        out = AlertUrgency::FUTURE;
    } else if (EqualsIgnoreCase(s, toString(AlertUrgency::PAST))) {
        out = AlertUrgency::PAST;
    } else if (EqualsIgnoreCase(s, toString(AlertUrgency::UNKNOWN))) {
        out = AlertUrgency::UNKNOWN;
    } else {
        return false;
    }
    return true;
}

bool parseAlertSeverity(const std::string& s, AlertSeverity& out) {
    if (EqualsIgnoreCase(s, toString(AlertSeverity::EXTREME))) {
        out = AlertSeverity::EXTREME;
    } else if (EqualsIgnoreCase(s, toString(AlertSeverity::SEVERE))) {
        out = AlertSeverity::SEVERE;
    } else if (EqualsIgnoreCase(s, toString(AlertSeverity::MODERATE))) {
        out = AlertSeverity::MODERATE;
    } else if (EqualsIgnoreCase(s, toString(AlertSeverity::MINOR))) {
        out = AlertSeverity::MINOR;
    } else if (EqualsIgnoreCase(s, toString(AlertSeverity::UNKNOWN))) {
        out = AlertSeverity::UNKNOWN;
    } else {
        return false;
    }
    return true;
}

bool parseAlertCertainty(const std::string& s, AlertCertainty& out) {
    if (EqualsIgnoreCase(s, toString(AlertCertainty::OBSERVED))) {
        out = AlertCertainty::OBSERVED;
    } else if (EqualsIgnoreCase(s, toString(AlertCertainty::LIKELY))) {
        out = AlertCertainty::LIKELY;
    } else if (EqualsIgnoreCase(s, toString(AlertCertainty::POSSIBLE))) {
        out = AlertCertainty::POSSIBLE;
    } else if (EqualsIgnoreCase(s, toString(AlertCertainty::UNLIKELY))) {
        out = AlertCertainty::UNLIKELY;
    } else if (EqualsIgnoreCase(s, toString(AlertCertainty::UNKNOWN))) {
        out = AlertCertainty::UNKNOWN;
    } else {
        return false;
    }
    return true;
}

}  // namespace utils
}  // namespace aidl::android::hardware::broadcastradio
