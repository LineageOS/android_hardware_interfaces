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

#include <memory>

#include "bluetooth_hal/chip/chip_provisioner_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator_interface.h"
#include "bluetooth_hal/transport/transport_interface.h"

namespace bluetooth_hal {

class BluetoothHal {
 public:
  static BluetoothHal& GetHal();
  bool RegisterVendorTransport(
      ::bluetooth_hal::transport::TransportType type,
      ::bluetooth_hal::transport::TransportInterface::FactoryFn factory);
  void RegisterVendorChipProvisioner(
      ::bluetooth_hal::chip::ChipProvisionerInterface::FactoryFn factory);
  void RegisterVendorChannelSoundingDistanceEstimator(
      ::bluetooth_hal::extensions::cs::
          ChannelSoundingDistanceEstimatorInterface::FactoryFn factory);
  void Start();
  void StartOffloadHal();

 private:
  void StartExtensions();
};

}  // namespace bluetooth_hal
