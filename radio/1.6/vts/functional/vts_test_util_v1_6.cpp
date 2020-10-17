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

#define LOG_TAG "RadioTest"

#include <vts_test_util_v1_6.h>
#include <iostream>
#include "VtsCoreUtil.h"

::testing::AssertionResult CheckAnyOfErrors(
        ::android::hardware::radio::V1_6::RadioError err,
        std::vector<::android::hardware::radio::V1_6::RadioError> errors, CheckFlag flag) {
    const static vector<::android::hardware::radio::V1_6::RadioError> generalErrors = {
            ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
            ::android::hardware::radio::V1_6::RadioError::NO_MEMORY,
            ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
            ::android::hardware::radio::V1_6::RadioError::SYSTEM_ERR,
            ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED,
            ::android::hardware::radio::V1_6::RadioError::CANCELLED};
    if (flag == CHECK_GENERAL_ERROR || flag == CHECK_OEM_AND_GENERAL_ERROR) {
        for (size_t i = 0; i < generalErrors.size(); i++) {
            if (err == generalErrors[i]) {
                return testing::AssertionSuccess();
            }
        }
    }
    if (flag == CHECK_OEM_ERROR || flag == CHECK_OEM_AND_GENERAL_ERROR) {
        if (err >= ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_1 &&
            err <= ::android::hardware::radio::V1_6::RadioError::OEM_ERROR_25) {
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
