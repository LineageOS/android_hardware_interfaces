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

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/subscriber.h"

namespace bluetooth_hal {
namespace transport {

/**
 * @brief Interface for handling transport-related events.
 *
 * This interface defines callbacks for handling transport events such as
 * connection closure and packet readiness. Implementations of this interface
 * should provide concrete behaviors for these events.
 */
class TransportInterfaceCallback {
 public:
  virtual ~TransportInterfaceCallback() = default;

  /**
   * @brief Called when the transport connection is closed.
   *
   * Implementations should handle any necessary cleanup or state updates when
   * the transport is closed.
   *
   */
  virtual void OnTransportClosed() = 0;

  /**
   * @brief Called when a packet is ready to be processed.
   *
   * @param packet The received packet that needs to be processed.
   *
   * Implementations should process the given packet accordingly. This method
   * does not return a callback, meaning the implementation is expected to
   * handle the packet directly within this function.
   *
   */
  virtual void OnTransportPacketReady(
      const ::bluetooth_hal::hci::HalPacket& packet) = 0;
};

/**
 * @brief Abstracts the transport layer for devices, providing interfaces for
 * control and data management.
 *
 * This class also manages subscriber lists to send messages or signals,
 * notifying the underlying transport instances for further operations.
 *
 */
class TransportInterface {
 public:
  virtual ~TransportInterface() = default;

  /**
   * @brief Initializes the transport interface with a transport callback.
   *
   * @param transport_interface_callback A pointer to a
   * `TransportInterfaceCallback` responsible for handling transport layer
   * events such as packet reception, connection closure, etc.
   *
   * @return True if initialization succeeds, false otherwise.
   *
   */
  virtual bool Initialize(
      TransportInterfaceCallback* transport_interface_callback) = 0;

  /**
   * @brief Cleans up resources and disconnects the transport interface.
   *
   */
  virtual void Cleanup() = 0;

  /**
   * @brief Checks if the current transport is active and operational.
   *
   * @return `true` if the transport is active and communication is operational,
   * `false` otherwise.
   *
   */
  virtual bool IsTransportActive() const = 0;

  /**
   * @brief Sends a single packet with the specified type.
   *
   * @param packet The content of the packet to be transmitted.
   *
   * @return `true` if packet is sent successfully, `false` otherwise.
   *
   */
  virtual bool Send(const ::bluetooth_hal::hci::HalPacket& packet) = 0;

  /**
   * @brief Retrieves the specific transport type of this instance.
   *
   * @return The TransportType of this concrete transport instance.
   *
   */
  virtual TransportType GetInstanceTransportType() const = 0;

  static TransportInterface& GetTransport();

  /**
   * Updates the current transport type for the TransportInterface.
   *
   * This method allows switching the transport type used by the
   * TransportInterface. If the provided type differs from the currently set
   * type, the internal transport type will be updated, and subsequent calls to
   * GetTransport() will return the transport instance corresponding to the
   * updated type.
   *
   * @param requested_type The new TransportType to set.
   *
   * @return `true` if the transport was successfully updated, `false`
   * otherwise.
   *
   */
  static bool UpdateTransportType(TransportType requested_type);

  /**
   * Retrieves the current transport type for the TransportInterface.
   *
   * This method returns the transport type that is currently configured. The
   * transport type determines the transport instance that will be used when
   * GetTransport() is called.
   *
   * @return The current TransportType.
   *
   */
  static TransportType GetTransportType();

  /**
   * @brief Registers a vendor-specific transport implementation.
   *
   * This static method allows for the registration of a custom transport
   * mechanism provided by a vendor. Once registered,
   * this transport can potentially be selected and used by the Bluetooth HAL.
   *
   * @param transport A unique pointer to an aobject that implements the
   * TransportInterface. The ownership of this object is
   * transferred to this class. The provided transport should
   * not be null.
   *
   * @return `true` if the vendor transport was successfully registered,
   * `false` otherwise.
   *
   */
  static bool RegisterVendorTransport(
      std::unique_ptr<TransportInterface> transport);

  /**
   * @brief Unregisters a vendor-specific transport implementation.
   *
   * This static method allows for the removal of a previously registered
   * custom transport mechanism.
   *
   * @param type The TransportType of the vendor transport to unregister.
   *
   * @return `true` if the vendor transport was successfully unregistered,
   * `false` otherwise (e.g., if the transport type was not found,
   * is not a vendor type, or is currently active).
   *
   */
  static bool UnregisterVendorTransport(TransportType type);

  /**
   * @brief Updates the busy state of the hci router.
   *
   * This function sets the internal state to indicate whether the hci router is
   * currently busy. This should be called by hci router.
   *
   * @param is_busy A boolean indicating the new busy state of the hci router.
   * Pass true if the hci router is busy, or false otherwise.
   *
   */
  static void SetHciRouterBusy(bool is_busy);

  /**
   * @brief Notifies the transport layer of a change in the HAL state.
   *
   * This function should be called whenever the HAL transitions to a new state.
   * This should be called by hci router.
   *
   * @param hal_state The new state of the HAL.
   *
   */
  static void NotifyHalStateChange(::bluetooth_hal::HalState hal_state);

  /**
   * @brief Subscribes a new subscriber to receive notifications.
   *
   * This function adds the given subscriber to the list of subscribers.
   * Once subscribed, the subscriber will receive notifications when events
   * occur.
   *
   * @param subscriber The subscriber to be added. It should be passed by
   * constant reference.
   *
   */
  static void Subscribe(Subscriber& subscriber);

  /**
   * @brief Unsubscribes an existing subscriber.
   *
   * This function removes the given subscriber from the list of subscribers.
   * Once unsubscribed, the subscriber will no longer receive notifications.
   *
   * @param subscriber The subscriber to be removed. It should be passed by
   * constant reference.
   *
   */
  static void Unsubscribe(Subscriber& subscriber);

 protected:
  static std::atomic<bool> is_hci_router_busy_;
  static std::atomic<::bluetooth_hal::HalState> hal_state_;
  static std::vector<std::reference_wrapper<Subscriber>> subscribers_;

 private:
  static std::pair<std::unique_ptr<TransportInterface>, TransportType>
  CreateOrAcquireTransport(TransportType requested_type);

  static TransportType current_transport_type_;
  static std::recursive_mutex transport_mutex_;
  static std::unique_ptr<TransportInterface> current_transport_;
  static std::unordered_map<TransportType, std::unique_ptr<TransportInterface>>
      vendor_transports_;
};

}  // namespace transport
}  // namespace bluetooth_hal
