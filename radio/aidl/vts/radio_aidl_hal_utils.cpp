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
#define LOG_TAG "RadioTest"

#include "radio_aidl_hal_utils.h"
#include <iostream>
#include "VtsCoreUtil.h"

using namespace aidl::android::hardware::radio::network;

#define WAIT_TIMEOUT_PERIOD 75

aidl::android::hardware::radio::sim::CardStatus cardStatus = {};

int GetRandomSerialNumber() {
    return rand();
}

::testing::AssertionResult CheckAnyOfErrors(RadioError err, std::vector<RadioError> errors,
                                            CheckFlag flag) {
    const static std::vector<RadioError> generalErrors = {
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

bool isSsSsEnabled() {
    // Do not use checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "")
    // until b/148904287 is fixed. We need exact matching instead of partial matching. (i.e.
    // by definition the empty string "" is a substring of any string).
    return !isDsDsEnabled() && !isTsTsEnabled();
}

bool isDsDsEnabled() {
    return testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "dsds");
}

bool isTsTsEnabled() {
    return testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "tsts");
}

bool isVoiceInService(RegState state) {
    return RegState::REG_HOME == state || RegState::REG_ROAMING == state;
}

bool isVoiceEmergencyOnly(RegState state) {
    return RegState::NOT_REG_MT_NOT_SEARCHING_OP_EM == state ||
           RegState::NOT_REG_MT_SEARCHING_OP_EM == state || RegState::REG_DENIED_EM == state ||
           RegState::UNKNOWN_EM == state;
}

bool isServiceValidForDeviceConfiguration(std::string& serviceName) {
    if (isSsSsEnabled()) {
        // Device is configured as SSSS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME) {
            ALOGI("%s instance is not valid for SSSS device.", serviceName.c_str());
            return false;
        }
    } else if (isDsDsEnabled()) {
        // Device is configured as DSDS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME && serviceName != RADIO_SERVICE_SLOT2_NAME) {
            ALOGI("%s instance is not valid for DSDS device.", serviceName.c_str());
            return false;
        }
    } else if (isTsTsEnabled()) {
        // Device is configured as TSTS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME && serviceName != RADIO_SERVICE_SLOT2_NAME &&
            serviceName != RADIO_SERVICE_SLOT3_NAME) {
            ALOGI("%s instance is not valid for TSTS device.", serviceName.c_str());
            return false;
        }
    }
    return true;
}

/*
 * Notify that the response message is received.
 */
void RadioResponseWaiter::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (serial == receivedSerial) {
        count_++;
        cv_.notify_one();
    }
}

/*
 * Wait till the response message is notified or till WAIT_TIMEOUT_PERIOD.
 */
std::cv_status RadioResponseWaiter::wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count_ == 0) {
        status = cv_.wait_until(lock, now + std::chrono::seconds(WAIT_TIMEOUT_PERIOD));
        if (status == std::cv_status::timeout) {
            return status;
        }
    }
    count_--;
    return status;
}

/**
 * Specific features on the Radio HAL rely on Radio HAL Capabilities.
 * The VTS test related to those features must not run if the related capability is disabled.
 * Typical usage within VTS: if (getRadioHalCapabilities()) return;
 */
bool RadioResponseWaiter::getRadioHalCapabilities() {
    // TODO(b/210712359): implement after RadioConfig VTS is created
    /**
    // Get HalDeviceCapabilities from the radio config
    std::shared_ptr<RadioConfigResponse> radioConfigRsp = new (std::nothrow)
    RadioConfigResponse(*this); radioConfig->setResponseFunctions(radioConfigRsp, nullptr); serial =
    GetRandomSerialNumber();

    radioConfig->getHalDeviceCapabilities(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    return radioConfigRsp->modemReducedFeatureSet1;
    **/
    return true;
}