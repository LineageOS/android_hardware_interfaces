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

#define LOG_TAG "bluetooth_hal"

#include "bluetooth_hal/bluetooth_hal.h"

#include <memory>
#include <string>

#include "android-base/logging.h"
#include "android/binder_interface_utils.h"
#include "android/binder_manager.h"
#include "android/binder_process.h"
#include "android/binder_status.h"
#include "bluetooth_hal/chip/chip_provisioner_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator_interface.h"
#include "bluetooth_hal/extensions/finder/bluetooth_finder.h"
#include "bluetooth_hal/hci_proxy_aidl.h"
#include "bluetooth_hal/hci_proxy_ffi.h"
#include "bluetooth_hal/transport/transport_interface.h"

namespace bluetooth_hal {

using ::aidl::android::hardware::bluetooth::hal::IBluetoothHci_addService;
using ::bluetooth_hal::HciProxyAidl;
using ::bluetooth_hal::chip::ChipProvisionerInterface;
using ::bluetooth_hal::extensions::cs::BluetoothChannelSounding;
using ::bluetooth_hal::extensions::cs::
    ChannelSoundingDistanceEstimatorInterface;
using ::bluetooth_hal::extensions::finder::BluetoothFinder;
using ::bluetooth_hal::transport::TransportInterface;
using ::bluetooth_hal::transport::TransportType;

using ::ndk::SharedRefBase;

BluetoothHal& BluetoothHal::GetHal() {
  static BluetoothHal hal;
  return hal;
}

bool BluetoothHal::RegisterVendorTransport(
    TransportType type, TransportInterface::FactoryFn factory) {
  return TransportInterface::RegisterVendorTransport(type, std::move(factory));
}

void BluetoothHal::RegisterVendorChipProvisioner(
    ChipProvisionerInterface::FactoryFn factory) {
  ChipProvisionerInterface::RegisterVendorChipProvisioner(std::move(factory));
}

void BluetoothHal::RegisterVendorChannelSoundingDistanceEstimator(
    ChannelSoundingDistanceEstimatorInterface::FactoryFn factory) {
  ChannelSoundingDistanceEstimatorInterface::
      RegisterVendorChannelSoundingDistanceEstimator(std::move(factory));
}

void BluetoothHal::Start() {
  StartExtensions();

  std::string instance = std::string() + HciProxyAidl::descriptor + "/default";
  std::shared_ptr<HciProxyAidl> hci_proxy = SharedRefBase::make<HciProxyAidl>();
  int status =
      AServiceManager_addService(hci_proxy->asBinder().get(), instance.c_str());
  if (status == STATUS_OK) {
    ABinderProcess_joinThreadPool();
  } else {
    LOG(ERROR) << "Could not register as a service!";
  }
}

void BluetoothHal::StartOffloadHal() {
  StartExtensions();
  static HciProxyFfi ffi;
  IBluetoothHci_addService(&ffi);
  ABinderProcess_joinThreadPool();
}

void BluetoothHal::StartExtensions() {
  std::string instance;
  int status;

  instance = std::string() + BluetoothChannelSounding::descriptor + "/default";
  std::shared_ptr<BluetoothChannelSounding> bluetooth_channel_sounding =
      SharedRefBase::make<BluetoothChannelSounding>();
  status = AServiceManager_addService(
      bluetooth_channel_sounding->asBinder().get(), instance.c_str());
  if (status != STATUS_OK) {
    LOG(ERROR) << "Could not register BluetoothChannelSounding as a service!";
  }

  instance = std::string() + BluetoothFinder::descriptor + "/default";
  std::shared_ptr<BluetoothFinder> bluetooth_finder =
      SharedRefBase::make<BluetoothFinder>();
  status = AServiceManager_addService(bluetooth_finder->asBinder().get(),
                                      instance.c_str());
  if (status != STATUS_OK) {
    LOG(ERROR) << "Could not register BluetoothFinder as a service!";
  }
}

}  // namespace bluetooth_hal
