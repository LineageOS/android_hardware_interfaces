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

#define LOG_TAG "bluetooth_hal.transport.data_processor"

#include "bluetooth_hal/transport/uart_h4/data_processor.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/debug/debug_types.h"
#include "bluetooth_hal/util/fd_watcher.h"
#include "bluetooth_hal/util/system_call_wrapper.h"
#include "com_android_bluetooth_bluetooth_hal_flags.h"

namespace bluetooth_hal {
namespace transport {

namespace hal_flags = ::com::android::bluetooth::bluetooth_hal::flags;

using ::bluetooth_hal::debug::CoredumpErrorCode;
using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::util::SystemCallWrapper;

DataProcessor::~DataProcessor() { fd_watcher_.StopWatching(); }

void DataProcessor::StartProcessing() {
  fd_watcher_.StartWatching(fd_, std::bind_front(&DataProcessor::Recv, this));
}

size_t DataProcessor::Send(std::span<const uint8_t> packet) {
  if (packet.empty()) {
    return 0;
  }

  iovec iov;
  iov.iov_base = const_cast<uint8_t*>(packet.data());
  iov.iov_len = packet.size_bytes();

  const size_t total_bytes = packet.size_bytes();
  size_t bytes_written = 0;
  size_t remaining_bytes = total_bytes;

  while (remaining_bytes > 0) {
    ssize_t ret = TEMP_FAILURE_RETRY(
        SystemCallWrapper::GetWrapper().Writev(fd_, &iov, 1));
    if (ret == -1) {
      if (errno == EAGAIN) {
        continue;
      }
      HAL_LOG(ERROR) << __func__ << ": Error writing to UART ("
                     << strerror(errno) << ").";
      break;
    } else if (ret == 0) {
      HAL_LOG(ERROR) << __func__ << ": Zero bytes written.";
      break;
    } else if (static_cast<size_t>(ret) == remaining_bytes) {
      bytes_written += ret;
      break;
    }

    bytes_written += ret;
    remaining_bytes -= ret;
    HAL_LOG(WARNING) << __func__ << ": " << bytes_written << " bytes written, "
                     << remaining_bytes << " bytes remaining.";

    // Adjust iov to skip the written data.
    iov.iov_base = static_cast<uint8_t*>(iov.iov_base) + ret;
    iov.iov_len -= ret;
  }

  return bytes_written;
}

void DataProcessor::Recv(int fd) {
  // The maximum length for ACL is 2 bytes, so the buffer size is set to 64 KB.
  constexpr size_t max_len = 64 * 1024;
  uint8_t buffer[max_len] = {0};

  const ssize_t bytes_read = TEMP_FAILURE_RETRY(
      SystemCallWrapper::GetWrapper().Read(fd, buffer, max_len));
  if (bytes_read == 0) {
    // This is only expected if the UART got closed when shutting down.
    HAL_LOG(WARNING) << __func__ << ": Unexpected EOF reading the packet type!";
    return;
  } else if (bytes_read < 0) {
    LOG(FATAL) << __func__ << ": Read packet type error: " << strerror(errno)
               << ".";
  }

  ParseHciPacket(std::span<const uint8_t>(buffer, bytes_read));
}

void DataProcessor::ParseHciPacket(std::span<const uint8_t> buffer) {
  while (!buffer.empty()) {
    const size_t bytes_handled = hci_packetizer_.ProcessData(buffer);

    if (!bytes_handled) {
      if (hal_flags::coredump_when_receiving_unimplemented_packet_type()) {
        DebugCentral::Get().GenerateCoredump(
            CoredumpErrorCode::kControllerUnimplementedPacketType);
        break;
      } else {
        LOG(FATAL) << __func__ << ": Cannot process data from hci packetizer!";
      }
    }

    buffer = buffer.subspan(bytes_handled);
  }
}

}  // namespace transport
}  // namespace bluetooth_hal
