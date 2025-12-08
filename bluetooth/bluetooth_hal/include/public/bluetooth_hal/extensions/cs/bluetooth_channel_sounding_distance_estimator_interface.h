/*
 * Copyright 2025 The Android Open Source Project
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

#pragma once

#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"
#include "bluetooth_hal/util/provider_factory.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

class ChannelSoundingDistanceEstimator;

class ChannelSoundingDistanceEstimatorInterface
    : public ::bluetooth_hal::util::ProviderFactory<
          ChannelSoundingDistanceEstimatorInterface,
          ChannelSoundingDistanceEstimator> {
 public:
  /**
   * @brief Registers a vendor-specific factory for creating
   * ChannelSoundingDistanceEstimatorInterface instances.
   *
   * @param factory The factory function to register.
   */
  static void RegisterVendorChannelSoundingDistanceEstimator(
      FactoryFn factory) {
    RegisterProviderFactory(std::move(factory));
  }

  /**
   * @brief Unregisters the vendor-specific factory.
   *
   */
  static void UnregisterVendorhannelSoundingDistanceEstimator() {
    UnregisterProviderFactory();
  }

  virtual ~ChannelSoundingDistanceEstimatorInterface() = default;

  /**
   * @brief Resets the internal state of the estimator.
   */
  virtual void ResetVariables() = 0;

  /**
   * @brief Estimates the distance based on the provided raw data.
   *
   * @param raw_data The raw data from the channel sounding procedure.
   * @return The estimated distance.
   */
  virtual double EstimateDistance(
      const ::aidl::android::hardware::bluetooth::ranging::
          ChannelSoudingRawData& raw_data) = 0;

  /**
   * @brief Gets the confidence level of the last estimation.
   *
   * @return The confidence level.
   */
  virtual double GetConfidenceLevel() = 0;
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
