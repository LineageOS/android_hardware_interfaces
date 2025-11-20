/*
 * Copyright 2023 The Android Open Source Project
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

#include <sys/select.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/extensions/thread/socket_processor.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace thread {

// A class representing a Thread daemon for transmitting/receiving packets from
// Thread HAL to the controller or from the controller to Thread HAL.
class ThreadDaemon {
 public:
  /**
   * @brief Constructor for the ThreadDaemon class.
   *
   * Initializes the daemon with a callback function that will be invoked when a
   * packet is received from the remote client.
   *
   * @param hal_packet_cb The callback function to be invoked when a packet is
   * ready to be processed. Must not be nullopt.
   *
   * @throws std::runtime_error if hal_packet_cb is nullopt.
   */
  explicit ThreadDaemon(
      std::optional<::bluetooth_hal::hci::HalPacketCallback> hal_packet_cb)
      : is_daemon_running_(false),
        is_client_connected_(false),
        require_starting_(false),
        notification_listen_fd_(kInvalidFileDescriptor),
        notification_write_fd_(kInvalidFileDescriptor),
        hal_packet_cb_(hal_packet_cb) {
    CHECK(hal_packet_cb_ != std::nullopt)
        << __func__ << ": hal_packet_cb == nullptr";
    ConfigureSocketProcessor();
  }

  /**
   * @brief Destructor for the ThreadDaemon class.
   *
   * Stops the daemon thread and cleans up any allocated resources.
   *
   */
  ~ThreadDaemon() {
    Stop();
    SocketProcessor::Cleanup();
  }

  /**
   * @brief Sends an uplink packet (to the Bluetooth controller/HAL).
   *
   * @param packet The packet to send.
   *
   */
  void SendUplink(const ::bluetooth_hal::hci::HalPacket& packet);

  /**
   * @brief Sends a downlink packet (to the remote client).
   *
   * @param packet The packet to send.
   *
   */
  void SendDownlink(const std::vector<uint8_t>& packet);

  /**
   * @brief Checks if the daemon thread is currently running.
   *
   * @return True if the daemon thread is running, false otherwise.
   *
   */
  bool IsDaemonRunning() const;

  /**
   * @brief Starts the daemon service.
   *
   * Initializes resources, spawns the background thread, and starts listening
   * for client connection requests.
   *
   * @return True if the daemon was successfully started, false otherwise.
   *
   */
  bool Start();

  /**
   * @brief Stops the daemon service.
   *
   * Gracefully terminates the daemon thread, closes the client connection (if
   * any), and releases resources.
   *
   * @return True if the daemon was successfully stopped, false otherwise.
   *
   */
  bool Stop();

 private:
  // Configures the socket processor for communication with the daemon.
  void ConfigureSocketProcessor();

  // Starts the Thread daemon process.
  // Returns true if the daemon was successfully started, false otherwise.
  bool StartDaemon();

  // Stops the Thread daemon process.
  // Returns true if the daemon was successfully stopped, false otherwise.
  bool StopDaemon();

  // Notifies the daemon to stop gracefully.
  // Returns true if the notification was successful, false otherwise.
  bool NotifyDaemonToStop();

  // Accepts a client connection from the Thread daemon.
  // Returns true if a client connection was successfully accepted, false
  // otherwise.
  bool AcceptClient();

  // Monitors the socket for incoming data and events.
  void MonitorSocket();

  // The main routine for the daemon thread, handling communication and events.
  void DaemonRoutine();

  // Cleans up server-side resources after the daemon is stopped.
  void CleanUpServer();

  // Cleans up client-side resources after the connection is closed.
  void CleanUpClient();

  // Checks if a received packet indicates a hardware reset event.
  // Returns true if the packet indicates a hardware reset, false otherwise.
  bool CheckIfHardwareReset(const std::vector<uint8_t>& packet);

  // Prepares file descriptors for monitoring in the `select` system call.
  void PrepareFdsForMonitor(fd_set* monitor_fds);

  // Constructs a Hal packet from the given packet data.
  ::bluetooth_hal::hci::HalPacket ConstructToHalPacket(
      const std::vector<uint8_t>& packet);

  // Extracts the original packet data from a Hal packet.
  std::vector<uint8_t> ExtractFromHalPacket(
      const ::bluetooth_hal::hci::HalPacket& packet);

  std::atomic<bool> is_daemon_running_;
  std::atomic<bool> is_client_connected_;
  std::atomic<bool> require_starting_;
  std::thread server_thread_;
  int notification_listen_fd_;
  int notification_write_fd_;
  std::mutex client_mtx_;

  SocketProcessor* socket_processor_;
  std::optional<::bluetooth_hal::hci::HalPacketCallback> hal_packet_cb_;
};

}  // namespace thread
}  // namespace bluetooth_hal
