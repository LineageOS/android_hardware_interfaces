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
#include <vts_test_util.h>
#include <iostream>

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