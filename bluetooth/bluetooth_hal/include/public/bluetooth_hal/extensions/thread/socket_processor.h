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

#include <cstdint>
#include <string>
#include <vector>

#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace thread {

enum class SocketMode : int {
  kSockModeStream = 1,
  kSockModeSeqPacket = 5,
};

/**
 * @brief A base class for managing socket communication, providing an
 * abstraction layer for sending, receiving data, and handling connections.
 *
 * It acts as an interface for concrete socket implementations, allowing
 * different types of socket communication (e.g., Unix domain sockets, TCP/IP
 * sockets) to be handled through a common set of methods.
 *
 * @note This class is designed to be extended. The behavior and functionality
 * are defined by the derived classes that implement the pure virtual functions.
 */
class SocketProcessor {
 public:
  /**
   * @brief Virtual destructor for proper cleanup in derived classes.
   *
   */
  virtual ~SocketProcessor() = default;

  /**
   * @brief Initializes the socket processor.
   *
   * This function must be called before any other functions in this class are
   * used.
   *
   * @param socket_path The path to the socket file (for Unix domain sockets).
   * @param hal_packet_cb A callback function triggered when a packet is ready
   * to be processed.
   *
   */
  static void Initialize(
      const std::string& socket_path,
      std::optional<::bluetooth_hal::hci::HalPacketCallback> hal_packet_cb);

  /**
   * @brief Cleans up the socket processor resources.
   *
   * This function should be called when the socket processor is no longer
   * needed.
   *
   */
  static void Cleanup();

  /**
   * @brief Returns a pointer to the singleton instance of the SocketProcessor.
   *
   * @return A pointer to the singleton instance.
   *
   */
  static SocketProcessor* GetProcessor();

  /**
   * @brief Sends data over the socket.
   *
   * @param data The data to send.
   *
   * @return True if the data was sent successfully, false otherwise.
   *
   */
  virtual bool Send(const std::vector<uint8_t>& data) = 0;

  /**
   * @brief Receives data from the socket.
   *
   * @return True if data was received successfully, false otherwise.
   */
  virtual bool Recv() = 0;

  /**
   * @brief Opens a server socket for accepting connections.
   *
   * @return True if the server socket was opened successfully, false otherwise.
   */
  virtual bool OpenServer() = 0;

  /**
   * @brief Closes the server socket.
   */
  virtual void CloseServer() = 0;

  /**
   * @brief Closes the client socket.
   *
   */
  virtual void CloseClient() = 0;

  /**
   * @brief Accepts a new client connection on the server socket.
   *
   * @return The file descriptor of the accepted client socket, or a negative
   * value on error.
   *
   */
  virtual int AcceptClient() = 0;

  /**
   * @brief Sets the file descriptor for the server socket.
   *
   * @param server_socket The file descriptor for the server socket.
   *
   */
  virtual void SetServerSocket(int server_socket) = 0;

  /**
   * @brief Sets the file descriptor for the client socket.
   *
   * @param client_socket The file descriptor for the client socket.
   *
   */
  virtual void SetClientSocket(int client_socket) = 0;

  /**
   * @brief Sets the socket mode (e.g., server or client).
   *
   * @param socket_mode The socket mode.
   *
   */
  virtual void SetSocketMode(SocketMode socket_mode) = 0;

  /**
   * @brief Gets the file descriptor for the server socket.
   *
   * @return The file descriptor for the server socket.
   *
   */
  virtual int GetServerSocket() const = 0;

  /**
   * @brief Gets the file descriptor for the client socket.
   *
   * @return The file descriptor for the client socket.
   *
   */
  virtual int GetClientSocket() const = 0;

  /**
   * @brief Checks if the socket file exists.
   *
   * @return True if the socket file exists, false otherwise.
   */
  virtual bool IsSocketFileExisted() const = 0;

  /**
   * @brief Opens a file monitor to watch for changes in the socket file.
   *
   * @return The file descriptor for the socket file monitor, or a negative
   * value on error.
   *
   */
  virtual int OpenSocketFileMonitor() = 0;

  /**
   * @brief Closes the socket file monitor.
   *
   */
  virtual void CloseSocketFileMonitor() = 0;

  /**
   * @brief Gets the file descriptor for the socket file monitor.
   *
   * @return The file descriptor for the socket file monitor.
   *
   */
  virtual int GetSocketFileMonitor() = 0;

 private:
  static inline SocketProcessor* processor_{nullptr};
};

}  // namespace thread
}  // namespace bluetooth_hal
