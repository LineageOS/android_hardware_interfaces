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
#include <cstdint>

namespace bluetooth_hal {

class Property {
 public:
  // Config properties.
  static constexpr char kBqrEventMask[] = "persist.bluetooth.bqr.event_mask";
  static constexpr char kA2dpOffloadCap[] =
      "persist.bluetooth.a2dp_offload.cap";
  static constexpr char kOpusEnabled[] = "persist.bluetooth.opus.enabled";
  static constexpr char kFinderEnable[] = "persist.bluetooth.finder.enable";
  static constexpr char kLdacDefaultQualityMode[] =
      "persist.bluetooth.a2dp_ldac.default_quality_mode";
  static constexpr char kTransportFallbackEnabled[] =
      "bluetooth.transport.fallback";
  static constexpr char kIsAcceleratedBtOnEnabled[] =
      "persist.bluetooth.accelerate.bt.on.enabled";
  static constexpr char kCdtHwId[] = "ro.boot.cdt_hwid";
  static constexpr char kProductName[] = "ro.product.name";
  static constexpr char kBuildType[] = "ro.build.type";
  static constexpr char kShutDownAction[] = "sys.shutdown.requested";

  // Transport properties.
  static constexpr char kUartPathOverride[] =
      "persist.vendor.bluetooth.uart_path_override";
  static constexpr char kBtSnoopLogMode[] = "persist.bluetooth.btsnooplogmode";
  static constexpr char kLastUartPath[] = "bluetooth.uart.last_uart_path";

  // Vendor logging properties.
  static constexpr char kBtSnoopMaxPacketsPerFileProperty[] =
      "persist.bluetooth.vendor.btsnoopsize";
  static constexpr char kBtVendorSnoopEnabledProperty[] =
      "persist.bluetooth.vendor.btsnoop";

  // Extension properties.
  static constexpr char kHrModeProperty[] = "persist.bluetooth.hr_mode";
  static constexpr char kThreadDispatcherSocketMode[] =
      "persist.bluetooth.thread_dispatcher.socket_mode";
  static constexpr char kChannelSoundingChangAlgoConfig[] =
      "bluetooth.vendor.cs.change_algo_config";
  static constexpr char kChannelSoundingVendorSpecificFirstDataByte[] =
      "bluetooth.vendor.cs.vendor_specific_data_byte_1";
};

enum class HalState : uint8_t {
  // Initial state, HAL is not yet started.
  kShutdown = 0,
  // HAL service is initially started.
  kInit,
  // Firmware is currently being downloaded.
  kFirmwareDownloading,
  // Firmware download is complete, ready to be loaded into RAM.
  kFirmwareDownloadCompleted,
  // Firmware is loaded into RAM, ready for initialization.
  kFirmwareReady,
  // All preparatory work is complete, Bluetooth chip is ready.
  kBtChipReady,
  // HAL is running with Bluetooth enabled.
  kRunning,
};

namespace hci {
// HCI UART transport packet types (refer to Bluetooth Core Specification,
// Volume 4, Part A, Section 2).
enum class HciPacketType : uint8_t {
  kUnknown = 0x00,
  kCommand = 0x01,
  kAclData = 0x02,
  kScoData = 0x03,
  kEvent = 0x04,
  kIsoData = 0x05,
  kThreadData = 0x70,  // Vendor-specific.
  kHdlcData = 0x7e,    // Vendor-specific.
};

enum class MonitorMode : int {
  kNone,
  kMonitor,
  kIntercept,
};

class HciConstants {
 public:
  // 1. Preamble includes bytes after the HCI packet type and ends at the
  // parameter total length.
  // 2. All offset constants below are indexed from the HCI packet type.

  // Two bytes for opcode, and one byte for parameter length (refer to
  // Bluetooth Core Specification 5.4, Volume 4, Part E, Section 5.4.1).
  static constexpr size_t kHciCommandPreambleSize = 3;
  static constexpr int kHciCommandOpcodeOffset = 1;
  static constexpr size_t kHciCommandLengthOffset = 3;

  // Two bytes are allocated for the handle and two bytes for the data length
  // (refer to Bluetooth Core Specification 5.4, Volume 4, Part E,
  // Section 5.4.2).
  static constexpr size_t kHciAclPreambleSize = 4;
  static constexpr size_t kHciAclLengthOffset = 3;

  // This structure consists of 2 bytes for the handle and 1 byte for the data
  // length, as defined in Bluetooth Core Specification 5.4, Volume 4, Part E,
  // section 5.4.3.
  static constexpr size_t kHciScoPreambleSize = 3;
  static constexpr size_t kHciScoLengthOffset = 3;

  // One byte for the event code and one byte for the parameter length (refer
  // to Bluetooth Core Specification 5.4, Volume 4, Part E, Section 5.4.4).
  static constexpr size_t kHciEventPreambleSize = 2;
  static constexpr size_t kHciEventCodeOffset = 1;
  static constexpr size_t kHciEventLengthOffset = 2;
  static constexpr size_t kHciBleEventSubCodeOffset = 3;

  // Two bytes are allocated for the handle and flags, and two bytes for the
  // data length (refer to Bluetooth Core Specification 5.4, Volume 4, Part E,
  // Section 5.4.5).
  static constexpr size_t kHciIsoPreambleSize = 4;
  static constexpr size_t kHciIsoLengthOffset = 3;

  // Two bytes are reserved and two bytes are allocated for the data length.
  static constexpr size_t kHciThreadPreambleSize = 4;
  static constexpr size_t kHciThreadLengthOffset = 3;

  static constexpr size_t kHciPreambleSizeMax = kHciAclPreambleSize;

  // Command Complete Event Specific Constants.
  static constexpr size_t kHciCommandCompleteResultOffset = 6;
  static constexpr size_t kHciCommandCompleteCommandOpcodeOffset = 4;

  // Command Status Event Specific Constants.
  static constexpr size_t kHciCommandStatusResultOffset = 3;
  static constexpr size_t kHciCommandStatusCommandOpcodeOffset = 5;

  // BQR Events Constants.
  static constexpr size_t kHciBqrEventSubCodeOffset = 3;
  static constexpr size_t kHciBqrReportIdOffset = 4;

  static constexpr size_t GetPreambleSize(HciPacketType type) {
    switch (type) {
      case HciPacketType::kCommand:
        return kHciCommandPreambleSize;
      case HciPacketType::kAclData:
        return kHciAclPreambleSize;
      case HciPacketType::kScoData:
        return kHciScoPreambleSize;
      case HciPacketType::kEvent:
        return kHciEventPreambleSize;
      case HciPacketType::kIsoData:
        return kHciIsoPreambleSize;
      case HciPacketType::kThreadData:
        return kHciThreadPreambleSize;
      default:
        return 0;
    }
  }

  static constexpr size_t GetPacketLengthOffset(HciPacketType type) {
    switch (type) {
      case HciPacketType::kCommand:
        return kHciCommandLengthOffset;
      case HciPacketType::kAclData:
        return kHciAclLengthOffset;
      case HciPacketType::kScoData:
        return kHciScoLengthOffset;
      case HciPacketType::kEvent:
        return kHciEventLengthOffset;
      case HciPacketType::kIsoData:
        return kHciIsoLengthOffset;
      case HciPacketType::kThreadData:
        return kHciThreadLengthOffset;
      default:
        return 0;
    }
  }
};

// Event codes as defined in Bluetooth Core Specification 5.4 Volume 4,
// Part E, section 7.7.
enum class EventCode : uint8_t {
  kCommandComplete = 0x0e,
  kCommandStatus = 0x0f,
  kBleMeta = 0x3e,
  kVendorSpecific = 0xff,
};

enum class GoogleEventSubCode : uint8_t {
  kControllerDebugInfo = 0x57,
  kBqrEvent = 0x58,
};

enum class CommandOpCode : uint16_t {
  // Command opcodes as defined in Bluetooth Core Specification 5.4 Volume 4,
  // Part E, section 7.
  kSetEventMask = 0x0c01,
  kHciReset = 0x0c03,
  kLeSetExtendedScanParam = 0x2041,
  kLeScanEnable = 0x2042,
  kLeExtCreateConnection = 0x2043,
  kCreateConnection = 0x0405,
  kDisconnection = 0x0406,
  kVendorSpecific = 0xfc00,
  // Vendor command opcodes defined by Google
  kGoogleVendorCapability = 0xfd53,
  kGoogleDebugInfo = 0xfd5b,
};

enum class EventResultCode : uint8_t {
  kSuccess = 0x00,
  kFailure = 0xff,
};

}  // namespace hci

namespace uart {

// This enum defines the various UART baud rates that the BT HAL must support.
enum class BaudRate : int {
  kRate115200 = 115200,
  kRate3000000 = 3000000,
  kRate4000000 = 4000000,
  kRate6000000 = 6000000,
  kRate9600000 = 9600000,
};

}  // namespace uart

namespace thread {

constexpr int kInvalidFileDescriptor = -1;

// Maximum Spinel payload size
constexpr uint16_t kRadioSpinelRxFrameBufferSize = 0x2000;

// Spinel command Hardware reset size
constexpr uint8_t kHardwareResetCommandSize = 0x03;

// Socket specific header
constexpr uint8_t kSocketSpecificHeader = 0x40;

// Spinel header
constexpr uint8_t kSpinelHeader = 0x80;

// Command Type
constexpr uint8_t kThreadCommandReset = 0x01;

// Sub command Type
constexpr uint8_t kThreadCommandResetHardware = 0x04;

constexpr char kThreadDispatcherFolderPath[] = "/data/vendor/bluetooth";

constexpr char kThreadDispatcherSocketPath[] =
    "/data/vendor/bluetooth/thread_dispatcher_socket";

}  // namespace thread

namespace transport {

constexpr int kMaxTransportTypes = 1000;

enum class TransportType : int {
  kUartH4 = 1,
  kVendorStart = 100,
  kVendorEnd = 199,  // Reserve types from 100 to 199 for vendors.
  kUnknown = kMaxTransportTypes,
};

}  // namespace transport

namespace util {
namespace power {

/**
 * @brief Enumerates the sources that can trigger a "wake-up" event in the
 * system.
 *
 * This enum defines the different components or processes that might need to
 * signal a wake-up condition. Each enumerator represents a specific source
 * and provides context for why a wake-up might be necessary.
 *
 * kTx: Used in all TX tasks, release after packet is written to transport.
 * kRx: Used in all RX tasks, release when packet is despatched to the client.
 * kHciBusy: Used to cover HCI command and event flow control.
 * kTransport: Used by the transport layer. The use case can be variant based on
 * it's requirement.
 * kInitialize: Used during the initialization of the HAL.
 * kClose: Used during the closing of the HAL.
 */
enum class WakeSource : uint8_t {
  kTx,
  kRx,
  kHciBusy,
  kTransport,
  kInitialize,
  kClose,
};

}  // namespace power
}  // namespace util

}  // namespace bluetooth_hal
