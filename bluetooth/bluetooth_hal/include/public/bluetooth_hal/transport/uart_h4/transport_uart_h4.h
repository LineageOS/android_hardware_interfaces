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

#include <cstddef>
#include <memory>
#include <mutex>

#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/device_control/power_manager.h"
#include "bluetooth_hal/transport/device_control/uart_manager.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/transport/uart_h4/data_processor.h"
#include "bluetooth_hal/util/timer_manager.h"

namespace bluetooth_hal {
namespace transport {

class TransportUartH4 : virtual public TransportInterface,
                        virtual public PowerManager,
                        virtual public UartManager,
                        virtual public Subscriber {
 public:
  TransportUartH4() = default;
  TransportUartH4(const TransportUartH4&) = delete;
  TransportUartH4& operator=(const TransportUartH4&) = delete;
  ~TransportUartH4() override;

  /**
   * @brief Returns the transport type matching the UART H4 transport.
   *
   * @return The transport type corresponding to kUartH4.
   */
  TransportType GetInstanceTransportType() const override;

  /**
   * @brief Initializes the transport interface with a transport callback.
   *
   * Sets up the transport interface including initialization of the
   * underlying device for operation.
   *
   * @param transport_interface_callback A pointer to a
   * `TransportInterfaceCallback` responsible for handling transport layer
   * events such as packet reception, connection closure, etc.
   *
   * @return True if initialization succeeds, false otherwise.
   *
   */
  bool Initialize(
      TransportInterfaceCallback* transport_interface_callback) override;

  /**
   * @brief Cleans up resources and disconnects the transport interface.
   *
   * Ensures that all allocated resources including the underlying device
   * are released and any active connections are safely terminated.
   *
   */
  void Cleanup() override;

  /**
   * @brief Checks if the current transport is active and operational.
   *
   * This method verifies if the underlying device is powered on and the
   * communication link to the device is established and functional.
   *
   * @return `true` if the transport is active and communication is operational,
   * false` otherwise.
   *
   */
  bool IsTransportActive() const override;

  /**
   * @brief Sends a single data packet with the specified type.
   *
   * This function transmits a single data packet, specifying its type and
   * content, and optionally requires an acknowledgment for the sent packet.
   *
   * @param packet The content of the data packet to be transmitted.
   *
   * @return `true` if data is sent successfully, `false` otherwise.
   *
   */
  bool Send(const ::bluetooth_hal::hci::HalPacket& packet) override;

  /**
   * @brief Resumes the underlying device from Low Power Mode (LPM) to an
   * active state.
   *
   * This method should be called to bring the underlying device back to
   * full operation after being in LPM.
   *
   * @return True if the resume operation is successful, false otherwise.
   *
   */
  bool ResumeFromLowPowerMode() override;

  /**
   * @brief Suspends the underlying device into Low Power Mode (LPM).
   *
   * This method should be called to transition underlying device to a low
   * power state.
   *
   * @return True if the suspend operation is successful, false otherwise.
   *
   */
  bool SuspendToLowPowerMode() override;

  /**
   * @brief Checks if the Low Power Mode (LPM) setup has been completed.
   *
   * This method checks whether the necessary setup for Low Power Mode (LPM)
   * has been successfully completed. It helps to determine if the system or
   * device is ready to enter or interact with LPM.
   *
   * @return True if the LPM setup is completed and the system is ready,
   * false otherwise.
   *
   */
  bool IsLowPowerModeSetupCompleted() const override;

  /**
   * @brief Adjusts the UART baud rate based on the current HAL state.
   *
   * This method is invoked to notify a change in the HAL (Hardware Abstraction
   * Layer) state. Depending on the specified `hal_state`, it updates the UART
   * baud rate to match the requirements of the firmware state.
   *
   * @param hal_state The current HAL state, which determines the desired baud
   * rate.
   *
   */
  void NotifyHalStateChange(::bluetooth_hal::HalState hal_state) override;

 protected:
  void EnableTransportWakelock(bool enable);
  bool IsTransportWakelockEnabled();

  TransportInterfaceCallback* transport_interface_callback_;

 private:
  bool InitializeDataPath();
  void TerminateDataPath();

  bool SetupLowPowerMode() override;
  void TeardownLowPowerMode() override;

  std::unique_ptr<DataProcessor> data_processor_;
  std::recursive_mutex mutex_;
  ::bluetooth_hal::util::Timer low_power_timer_;
  bool is_lpm_resumed_ = false;
  static constexpr int kLpmTimeoutMs = 500;
  bool transport_wakelock_enabled_ = true;
};

}  // namespace transport
}  // namespace bluetooth_hal
