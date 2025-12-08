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

#define LOG_TAG "bluetooth_hal.transport_interface"

#include "bluetooth_hal/transport/transport_interface.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/uart_h4/transport_uart_h4.h"

namespace bluetooth_hal {
namespace transport {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::HalConfigLoader;

TransportInterface& TransportInterface::GetTransport() {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);

  if (current_transport_) {
    return *current_transport_;
  }

  // TODO: b/442448282 - It is not safe to do auto initialize here, need to
  // separate the logic from GetTransport().
  const std::vector<TransportType>& current_transport_type_priorities =
      HalConfigLoader::GetLoader().GetTransportTypePriority();

  for (auto type : current_transport_type_priorities) {
    if (UpdateTransportType(type)) {
      return *current_transport_;
    }
  }

  LOG(WARNING) << __func__
               << ": No transport from priority list initialized. Falling back "
                  "to UartH4.";
  UpdateTransportType(TransportType::kUartH4);

  return *current_transport_;
}

std::pair<std::unique_ptr<TransportInterface>, TransportType>
TransportInterface::CreateOrAcquireTransport(TransportType requested_type) {
  std::unique_ptr<TransportInterface> new_transport;
  TransportType new_transport_type = requested_type;

  switch (requested_type) {
    case TransportType::kVendorStart... TransportType::kVendorEnd: {
      new_transport = VendorFactory::Create(requested_type);
      if (!new_transport) {
        LOG(ERROR) << __func__ << ": Vendor factory for type "
                   << static_cast<int>(requested_type)
                   << " not found or returned null.";
        return {nullptr, requested_type};
      }
      if (new_transport->GetInstanceTransportType() != requested_type) {
        LOG(ERROR) << __func__ << ": Vendor factory for type "
                   << static_cast<int>(requested_type)
                   << " returned mismatched transport type: "
                   << static_cast<int>(
                          new_transport->GetInstanceTransportType());
        return {nullptr, requested_type};
      }
      break;
    }
    case TransportType::kUartH4: {
      new_transport = std::make_unique<TransportUartH4>();
      break;
    }
    case TransportType::kUnknown:
    default:
      LOG(WARNING) << __func__ << ": Requested unhandled or kUnknown type: "
                   << static_cast<int>(requested_type)
                   << ".  Defaulting to kUartH4.";
      new_transport_type = TransportType::kUartH4;
      new_transport = std::make_unique<TransportUartH4>();
      break;
  }

  return {std::move(new_transport), new_transport_type};
}

bool TransportInterface::UpdateTransportType(TransportType requested_type) {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);

  if (current_transport_type_ == requested_type && current_transport_) {
    return true;
  }

  auto [new_transport, new_transport_type] =
      CreateOrAcquireTransport(requested_type);

  // If the new transport instance could not be created or acquired.
  if (!new_transport) {
    LOG(ERROR) << __func__
               << ": Failed to create or acquire new transport for type: "
               << static_cast<int>(requested_type);
    return false;
  }

  // New transport is ready. Now, cleanup and replace the old one.
  if (current_transport_) {
    CleanupTransport();
  }

  // Activate the new transport.
  current_transport_ = std::move(new_transport);
  current_transport_type_ = new_transport_type;

  if (current_transport_) {
    LOG(INFO) << __func__
              << ": Successfully initialized transport for priority type: "
              << static_cast<int>(current_transport_type_);
  }

  return current_transport_ != nullptr;
}

void TransportInterface::CleanupTransport() {
  if (current_transport_) {
    current_transport_->Cleanup();
    current_transport_.reset();
    current_transport_type_ = TransportType::kUnknown;
  }
}

bool TransportInterface::RegisterVendorTransport(TransportType type,
                                                 FactoryFn factory) {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);

  if (!factory) {
    LOG(ERROR) << __func__ << ": Cannot register null factory.";
    return false;
  }

  if (current_transport_ && current_transport_type_ == type) {
    LOG(WARNING) << __func__ << ": Current vendor transport is active for type "
                 << static_cast<int>(type) << ", close it first.";
    return false;
  }

  if (type < TransportType::kVendorStart || type > TransportType::kVendorEnd) {
    LOG(ERROR) << __func__
               << ": Invalid vendor transport type: " << static_cast<int>(type);
    return false;
  }

  if (VendorFactory::IsRegistered(type)) {
    LOG(WARNING) << __func__
                 << ": Vendor transport factory already registered for type: "
                 << static_cast<int>(type) << ". Overwriting.";
  }
  VendorFactory::RegisterProviderFactory(type, std::move(factory));

  return true;
}

bool TransportInterface::UnregisterVendorTransport(TransportType type) {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);

  if (type < TransportType::kVendorStart || type > TransportType::kVendorEnd) {
    LOG(ERROR) << __func__
               << ": Invalid transport type for unregistration (not a vendor "
                  "type): "
               << static_cast<int>(type);
    return false;
  }

  if (current_transport_ && current_transport_type_ == type) {
    LOG(WARNING) << __func__ << ": Cannot unregister currently active "
                 << "vendor transport type: " << static_cast<int>(type);
    return false;
  }

  if (!VendorFactory::IsRegistered(type)) {
    LOG(WARNING) << __func__
                 << ": Vendor transport factory not found for type: "
                 << static_cast<int>(type);
    return false;
  }

  VendorFactory::UnregisterProviderFactory(type);
  LOG(INFO) << __func__
            << ": Successfully unregistered vendor transport factory for type: "
            << static_cast<int>(type);
  return true;
}

TransportType TransportInterface::GetTransportType() {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);
  return current_transport_type_;
}

void TransportInterface::SetHciRouterBusy(bool is_busy) {
  is_hci_router_busy_ = is_busy;
}

void TransportInterface::NotifyHalStateChange(HalState hal_state) {
  if (hal_state_ == hal_state) {
    return;
  }

  hal_state_ = hal_state;

  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);
  for (const std::reference_wrapper<Subscriber>& subscriber : subscribers_) {
    subscriber.get().NotifyHalStateChange(hal_state);
  }
}

void TransportInterface::Subscribe(Subscriber& subscriber) {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);
  const auto it = std::find_if(
      subscribers_.begin(), subscribers_.end(),
      [&](const std::reference_wrapper<Subscriber>& member_wrapper) {
        return subscriber == member_wrapper.get();
      });
  if (it == subscribers_.end()) {
    subscribers_.push_back(std::ref(subscriber));
  }
}

void TransportInterface::Unsubscribe(Subscriber& subscriber) {
  std::lock_guard<std::recursive_mutex> lock(transport_mutex_);
  const auto it = std::find_if(
      subscribers_.begin(), subscribers_.end(),
      [&](const std::reference_wrapper<Subscriber>& member_wrapper) {
        return subscriber == member_wrapper.get();
      });
  if (it != subscribers_.end()) {
    subscribers_.erase(it);
  }
}

}  // namespace transport
}  // namespace bluetooth_hal
