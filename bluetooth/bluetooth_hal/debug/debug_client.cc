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

#include "bluetooth_hal/debug/debug_client.h"

#include <deque>
#include <sstream>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/util/logging.h"

namespace bluetooth_hal {
namespace debug {

using ::android::base::LogSeverity;
using ::bluetooth_hal::util::Logger;

DebugClient::DebugClient() { DebugCentral::Get().RegisterDebugClient(this); }

DebugClient::~DebugClient() { DebugCentral::Get().UnregisterDebugClient(this); }

void DebugClient::OnGenerateCoredump(
    [[maybe_unused]] CoredumpErrorCode error_code,
    [[maybe_unused]] uint8_t sub_error_code) {}

std::vector<Coredump> DebugClient::Dump() {
  if (log_tag_.empty()) {
    return std::vector<Coredump>();
  }

  Coredump coredump(log_tag_, GetClientLogsInString(), CoredumpPosition::kEnd);
  return {coredump};
}

DebugClient::ClientLogStream DebugClient::ClientLog(LogSeverity severity,
                                                    const char* tag) {
  if (log_tag_.empty()) {
    SetClientLogTag(tag);
  }
  return DebugClient::ClientLogStream(client_logs_, severity, tag);
}

const std::deque<std::string>& DebugClient::GetClientLogs() const {
  return client_logs_;
}

std::string DebugClient::GetClientLogsInString() const {
  std::ostringstream oss;
  for (const auto& log : client_logs_) {
    oss << log << "\n";
  }
  return oss.str();
}

void DebugClient::SetClientLogTag(const std::string& tag) { log_tag_ = tag; }

DebugClient::ClientLogStream::ClientLogStream(
    std::deque<std::string>& log_buffer, LogSeverity severity, const char* tag)
    : log_buffer_(log_buffer), severity_(severity), tag_(tag) {
  timestamp_stream_ << Logger::GetLogFormatTimestamp() << ": ";
}

DebugClient::ClientLogStream::~ClientLogStream() {
  auto log_message = stream_.str();
  if (!log_message.empty()) {
    if (log_buffer_.size() >= kMaxClientLogSize) {
      log_buffer_.pop_front();
    }
    LOG_WITH_TAG(severity_, tag_) << log_message;
    log_buffer_.push_back(timestamp_stream_.str() + log_message);
  }
}

}  // namespace debug
}  // namespace bluetooth_hal
