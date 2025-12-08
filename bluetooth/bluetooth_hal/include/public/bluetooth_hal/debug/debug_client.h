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

#include <cstdint>
#include <cstring>
#include <deque>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_types.h"

#ifdef UNIT_TEST
#define CLIENT_LOG(severity) LOG(severity)
#else
/*
 * CLIENT_LOG pinrts system log, as well as stores it in the DebugClient for
 * Dump()
 */
#define CLIENT_LOG(severity)                        \
  ([](auto&& logger) -> auto&& { return logger; })( \
      ClientLog(::android::base::severity, LOG_TAG))
#endif

namespace bluetooth_hal {
namespace debug {

enum class CoredumpPosition : uint8_t {
  kBegin,
  kEnd,
  kCustomDumpsys,
};

struct Coredump {
  std::string title;
  std::string coredump;
  CoredumpPosition position;
};

/**
 * @brief A child class extends DebugCentral will automatically receive
 * OnGenerateCoredump and Dump callbacks for debugging. The child class can
 * choose to not implement any of those functions if they are not needed.
 *
 * The coredump is generated with the format below:
 *
 * ╔══════════════════════════════════════════════════════════
 * ║ BEGIN of Bluetooth HAL DUMP
 * ╠══════════════════════════════════════════════════════════
 * ║
 * ║    =============================================
 * ║    TITLE FOR CoredumpPosition::kBegin 1
 * ║    =============================================
 * ║        COREDUMP for CoredumpPosition::kBegin 1
 * ║
 * ║    =============================================
 * ║    TITLE FOR CoredumpPosition::kBegin 2
 * ║    =============================================
 * ║        COREDUMP for CoredumpPosition::kBegin 2
 * ║    ...
 * ║
 * ║    =============================================
 * ║    Default Bluetooth HAL dump
 * ║    =============================================
 * ║        dump
 * ║
 * ║    =============================================
 * ║    TITLE FOR CoredumpPosition::kEnd 1
 * ║    =============================================
 * ║        COREDUMP for CoredumpPosition::kEnd 1
 * ║    ...
 * ║
 * ╠══════════════════════════════════════════════════════════
 * ║ END of Bluetooth HAL DUMP
 * ╚══════════════════════════════════════════════════════════
 *
 * ** Everything with CoredumpPosition::kCustomDumpsys are printed outside **
 * ** the HAL DUMP frame.                                                  **
 *
 */
class DebugClient {
 public:
#ifndef UNIT_TEST
  DebugClient();
  virtual ~DebugClient();

  /**
   * @brief OnGenerateCoredump is automatically called by the DebugCentral if
   * any error was detected and the HAL decided to generate a coredump for the
   * following crash.
   *
   * A child class can decide to collect logs or generate their own dump files
   * if required.
   *
   * Dump() will be called soon after OnGenerateCoredump() is invoked.
   *
   * @param error_code The main coredump error code of the coredump.
   * @param sub_error_code The sub error code of the coredump.
   */
  virtual void OnGenerateCoredump(CoredumpErrorCode error_code,
                                  uint8_t sub_error_code);

  /**
   * @brief Dump() can be called for two scenarios:
   *    1. When the Android dumpsys or bugreport is triggered.
   *    2. When the DebugCentral detects an error, called after
   *    OnGenerateCoredump().
   *
   *
   * @return A vector of Coredump is returned to the DebugCentral. the Coredumps
   * will be transformed into text logs based on the parameters set in it.
   * By default it returns the logs logged with the macro CLIENT_LOG(severity)
   * with LOG_TAG as the title.
   */
  virtual std::vector<Coredump> Dump();

 protected:
  /**
   * @brief ClientLogStream provides a stream-like interface for logging.
   * It captures whatever is streamed to it and, upon destruction,
   * adds the complete string to the DebugClient's log buffer.
   */
  class ClientLogStream {
   public:
    explicit ClientLogStream(std::deque<std::string>& log_buffer,
                             ::android::base::LogSeverity severity,
                             const char* tag);
    ~ClientLogStream();

    template <typename T>
    ClientLogStream& operator<<(const T& value) {
      stream_ << value;
      return *this;
    }

    ClientLogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
      stream_ << manip;
      return *this;
    }

   private:
    std::ostringstream stream_;
    std::ostringstream timestamp_stream_;
    std::deque<std::string>& log_buffer_;
    ::android::base::LogSeverity severity_;
    const char* tag_;
    static constexpr int kMaxClientLogSize = 10;
  };

  /**
   * @brief ClientLog() returns a ClientLogStream object, allowing child classes
   * to log messages using stream-like syntax (e.g., ClientLog() << "message").
   * The log message is stored internally by DebugClient.
   *
   * This function should not be used directly without the CLIENT_LOG macro.
   */
  ClientLogStream ClientLog(::android::base::LogSeverity severity,
                            const char* tag);

  /**
   * @brief GetClientLogs retrieves all collected log messages.
   * @return A constant reference to the vector of collected log strings.
   */
  const std::deque<std::string>& GetClientLogs() const;

  /**
   * @brief GetClientLogsInString returns a single string containing all
   * collected logs, with each log entry separated by a newline.
   * @return A string with all log entries concatenated.
   */
  std::string GetClientLogsInString() const;

  void SetClientLogTag(const std::string& tag);

  std::deque<std::string> client_logs_;
  std::string log_tag_;

#else
  // Empty methods for the child classes' unit tests.
  // There is no point to create a mock solution for the logger besides adding
  // more complexity in the code.
  virtual ~DebugClient() = default;
  virtual void OnGenerateCoredump([[maybe_unused]] CoredumpErrorCode error_code,
                                  [[maybe_unused]] uint8_t sub_error_code) {}
  const std::deque<std::string>& GetClientLogs() const {
    static const std::deque<std::string> empty_logs;
    return empty_logs;
  }
  std::string GetClientLogsInString() const { return ""; }
  void SetClientLogTag([[maybe_unused]] const std::string& tag) {}
#endif
};

}  // namespace debug
}  // namespace bluetooth_hal
