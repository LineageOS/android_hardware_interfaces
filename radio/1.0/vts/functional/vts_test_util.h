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

#include <android-base/logging.h>

#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/radio/1.0/types.h>

using ::android::hardware::radio::V1_0::RadioError;
using ::android::hardware::radio::V1_0::SapResultCode;
using namespace std;

enum CheckFlag {
    CHECK_DEFAULT = 0,
    CHECK_GENERAL_ERROR = 1,
    CHECK_OEM_ERROR = 2,
    CHECK_OEM_AND_GENERAL_ERROR = 3,
    CHECK_SAP_ERROR = 4,
};

/*
 * Generate random serial number for radio test
 */
int GetRandomSerialNumber();

/*
 * Check multiple radio error codes which are possibly returned because of the different
 * vendor/devices implementations. It allows optional checks for general errors or/and oem errors.
 */
::testing::AssertionResult CheckAnyOfErrors(RadioError err, std::vector<RadioError> generalError,
                                            CheckFlag flag = CHECK_DEFAULT);
/*
 * Check multiple sap error codes which are possibly returned because of the different
 * vendor/devices implementations.
 */
::testing::AssertionResult CheckAnyOfErrors(SapResultCode err, std::vector<SapResultCode> errors);
