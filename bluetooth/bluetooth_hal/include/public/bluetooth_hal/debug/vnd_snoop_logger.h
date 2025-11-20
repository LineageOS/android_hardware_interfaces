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

#include "bluetooth_hal/hal_packet.h"

namespace bluetooth_hal {
namespace debug {

class VndSnoopLogger {
 public:
  enum class Direction : int {
    kIncoming,
    kOutgoing,
  };

  virtual ~VndSnoopLogger() = default;

  /**
   * @brief Get the singleton instance of VndSnoopLogger.
   *
   * @return The singleton instance of VndSnoopLogger.
   *
   */
  static VndSnoopLogger& GetLogger();

  /**
   * @brief Initiates the logging process for Bluetooth events.
   *
   * Starts logging to new log file.
   */
  virtual void StartNewRecording() = 0;

  /**
   * @brief Stops the ongoing recording process.
   *
   * Ends the logging of Bluetooth events and closes any open resources.
   */
  virtual void StopRecording() = 0;

  /**
   * @brief Captures an HCI packet for logging.
   *
   * @param packet The HCI packet data to capture.
   * @param direction Specifies whether the packet is incoming or outgoing.
   *
   * Adds a Bluetooth HCI packet to the log, recording its metadata and type.
   */
  virtual void Capture(const ::bluetooth_hal::hci::HalPacket& packet,
                       Direction direction) = 0;
};

}  // namespace debug
}  // namespace bluetooth_hal
