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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_SERVICE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_SERVICE_H

#include <nnapi/IDevice.h>
#include <nnapi/Types.h>
#include <memory>
#include <vector>

namespace android::hardware::neuralnetworks::service {

/**
 * @brief Get the NNAPI sAIDL and HIDL services declared in the VINTF.
 *
 * @pre maxFeatureLevelAllowed >= Version::Level::FEATURE_LEVEL_5
 *
 * @param maxFeatureLevelAllowed Maximum version of driver allowed to be used. Any driver version
 *     exceeding this must be clamped to `maxFeatureLevelAllowed`.
 * @return A list of devices and whether each device is updatable or not.
 */
std::vector<nn::SharedDevice> getDevices(nn::Version::Level maxFeatureLevelAllowed);

}  // namespace android::hardware::neuralnetworks::service

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_SERVICE_H
