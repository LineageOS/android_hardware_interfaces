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

#include <memory>
#include <mutex>

#include "bluetooth_hal/extensions/thread/thread_daemon.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace thread {

/**
 * @brief This class provides a handler for processing Thread packets and
 * specifically designed for communication with a remote client.
 *
 * This class manages a dedicated thread (daemon) for handling incoming and
 * outgoing Thread packets. The thread daemon is automatically started when
 * the `OnBluetoothChipReady()` callback is triggered, signaling that the
 * underlying Bluetooth chip is initialized and ready for operation. It
 * continuously monitors for incoming packets and forwards them to the remote
 * client. The thread daemon is stopped when the `OnBluetoothChipClosed()`
 * callback is triggered, indicating that the Bluetooth chip is being shut down.
 *
 */
class ThreadHandler : public ::bluetooth_hal::hci::HciRouterClient {
 public:
  ThreadHandler();

  ~ThreadHandler() override;

  /**
   * @brief Initializes the thread handler and associated resources.
   *
   * This function should be called before any other functions in this class are
   * used.
   *
   */
  static void Initialize();

  /**
   * @brief Cleans up the thread handler and releases associated resources.
   *
   * This function should be called when the thread handler is no longer needed.
   * It ensures that all resources are properly released and the handler is
   * stopped gracefully.
   *
   */
  static void Cleanup();

  /**
   * @brief Checks if the handler is initialized.
   *
   * @return True if the handler is initialized, false otherwise.
   *
   */
  static bool IsHandlerRunning();

  /**
   * @brief Returns a reference to the ThreadHandler instance.
   *
   * This provides access to the underlying thread handler for advanced
   * operations.
   *
   * @return A reference to the ThreadHandler instance.
   *
   */
  static ThreadHandler& GetHandler();

  /**
   * @brief Called when a command packet is received. Not used in this
   * implementation.
   *
   * @param packet The received command packet.
   *
   */
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& packet) override;

  /**
   * @brief Called when a monitor packet is received. Forwards the
   * packet to the remote client.
   *
   * This function is the primary mechanism for receiving packets from the
   * Bluetooth chip and transmitting them to a connected remote client for
   * processing.
   *
   * @param mode The monitor mode.
   * @param packet The received packet from the HCI router.
   *
   */
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;

  /**
   * @brief Called when the Bluetooth chip is ready. Starts the handler's thread
   * daemon.
   *
   * This function is invoked as a callback when the underlying Bluetooth chip
   * signals that it has completed its initialization and is ready for
   * operation. It triggers the start of the thread daemon, enabling packet
   * processing.
   *
   */
  void OnBluetoothChipReady() override;

  /**
   * @brief Called when the Bluetooth chip is closed. Stops the handler's thread
   * daemon.
   *
   * This function is invoked as a callback when the underlying Bluetooth chip
   * signals that it is being shut down. It triggers the stop of the handler's
   * thread daemon, preventing further packet processing.
   *
   */
  void OnBluetoothChipClosed() override;

  /**
   * @brief Called when Bluetooth is enabled. Not used in this implementation.
   *
   */
  void OnBluetoothEnabled() override;

  /**
   * @brief Called when Bluetooth is disabled. Not used in this implementation.
   *
   */
  void OnBluetoothDisabled() override;

  /**
   * @brief Checks if the Thread daemon is currently running.
   *
   * @return True if the daemon is running, false otherwise.
   *
   */
  bool IsDaemonRunning() const;

 private:
  std::unique_ptr<ThreadDaemon> thread_daemon_;
  static std::mutex mutex_;
  static std::unique_ptr<ThreadHandler> handler_;
  ::bluetooth_hal::hci::HciThreadMonitor thread_data_monitor_;
};

}  // namespace thread
}  // namespace bluetooth_hal
