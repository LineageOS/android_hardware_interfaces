/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator.h"

#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;

void ChannelSoundingDistanceEstimator::ResetVariables() {};

double ChannelSoundingDistanceEstimator::EstimateDistance(
    [[maybe_unused]] const ChannelSoudingRawData& raw_data) {
  return 0;
}

double ChannelSoundingDistanceEstimator::GetConfidenceLevel() { return 0; }

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
