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
#define LOG_TAG "RadioTest"

#include <vts_test_util.h>
#include <iostream>
#include "VtsCoreUtil.h"

int GetRandomSerialNumber() {
    return rand();
}

::testing::AssertionResult CheckAnyOfErrors(RadioError err, std::vector<RadioError> errors,
                                            CheckFlag flag) {
    const static vector<RadioError> generalErrors = {
        RadioError::RADIO_NOT_AVAILABLE,   RadioError::NO_MEMORY,
        RadioError::INTERNAL_ERR,          RadioError::SYSTEM_ERR,
        RadioError::REQUEST_NOT_SUPPORTED, RadioError::CANCELLED};
    if (flag == CHECK_GENERAL_ERROR || flag == CHECK_OEM_AND_GENERAL_ERROR) {
        for (size_t i = 0; i < generalErrors.size(); i++) {
            if (err == generalErrors[i]) {
                return testing::AssertionSuccess();
            }
        }
    }
    if (flag == CHECK_OEM_ERROR || flag == CHECK_OEM_AND_GENERAL_ERROR) {
        if (err >= RadioError::OEM_ERROR_1 && err <= RadioError::OEM_ERROR_25) {
            return testing::AssertionSuccess();
        }
    }
    for (size_t i = 0; i < errors.size(); i++) {
        if (err == errors[i]) {
            return testing::AssertionSuccess();
        }
    }
    return testing::AssertionFailure() << "RadioError:" + toString(err) + " is returned";
}

::testing::AssertionResult CheckAnyOfErrors(SapResultCode err, std::vector<SapResultCode> errors) {
    for (size_t i = 0; i < errors.size(); i++) {
        if (err == errors[i]) {
            return testing::AssertionSuccess();
        }
    }
    return testing::AssertionFailure() << "SapError:" + toString(err) + " is returned";
}

// Runs "pm list features" and attempts to find the specified feature in its output.
bool deviceSupportsFeature(const char* feature) {
    bool hasFeature = false;
    FILE* p = popen("/system/bin/pm list features", "re");
    if (p) {
        char* line = NULL;
        size_t len = 0;
        while (getline(&line, &len, p) > 0) {
            if (strstr(line, feature)) {
                hasFeature = true;
                break;
            }
        }
        pclose(p);
    } else {
        __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "popen failed: %d", errno);
        _exit(EXIT_FAILURE);
    }
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Feature %s: %ssupported", feature,
                        hasFeature ? "" : "not ");
    return hasFeature;
}

bool isDsDsEnabled() {
    return testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "dsds");
}

bool isTsTsEnabled() {
    return testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "tsts");
}

bool isVoiceInService(RegState state) {
    return ::android::hardware::radio::V1_0::RegState::REG_HOME == state ||
           ::android::hardware::radio::V1_0::RegState::REG_ROAMING == state;
}

bool isVoiceEmergencyOnly(RegState state) {
    return ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_NOT_SEARCHING_OP_EM == state ||
           ::android::hardware::radio::V1_0::RegState::NOT_REG_MT_SEARCHING_OP_EM == state ||
           ::android::hardware::radio::V1_0::RegState::REG_DENIED_EM == state ||
           ::android::hardware::radio::V1_0::RegState::UNKNOWN_EM == state;
}