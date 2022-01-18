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

#include "TestUtils.h"

#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <string>

namespace aidl::android::hardware::neuralnetworks::utils {

std::string printTestVersion(const testing::TestParamInfo<nn::Version>& info) {
    switch (info.param.level) {
        case nn::Version::Level::FEATURE_LEVEL_5:
            return "v1";
        case nn::Version::Level::FEATURE_LEVEL_6:
            return "v2";
        case nn::Version::Level::FEATURE_LEVEL_7:
            return "v3";
        case nn::Version::Level::FEATURE_LEVEL_8:
            return "v4";
        default:
            LOG(FATAL) << "Invalid AIDL version: " << info.param;
            return "invalid";
    }
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
