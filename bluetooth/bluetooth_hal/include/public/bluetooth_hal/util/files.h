/*
 * Copyright 2020 The Android Open Source Project
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

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace bluetooth_hal {
namespace os {

// Get FS Debug Dump
bool GetFsDebugDump(int fd, const std::string& debugfs);

// Get Battery Percentage
bool GetBatteryPercentage(std::string& batt_level);

// Return true if |path| exists on disk
bool FileExists(const std::string& path);

// Rename file from |from| to |to|
bool RenameFile(const std::string& from, const std::string& to);

// Implement ability to read a whole file from |path| into a C++ string, return
// std::nullopt on failure
//
// Do not use this with large files
std::optional<std::string> ReadSmallFile(const std::string& path);

// Implement ability to safely write to a file. This function is needed because
// of deficiencies in existing C++ file libraries, namely:
// - The ability to open and sync directories with storage media
// - The ability to block and sync file to storage media
// Return true on success, false on failure
bool WriteToFile(const std::string& path, const std::string& data);

// Remove file and print error message if failed
// Print error log when file is failed to be removed, hence user should make
// sure file exists before calling this Return true on success, false on failure
// (e.g. file not exist, failed to remove, etc)
bool RemoveFile(const std::string& path);

// Returns created time_point of given file, return std::nullopt on failure
std::optional<std::chrono::time_point<std::chrono::system_clock,
                                      std::chrono::nanoseconds>>
FileCreatedTime(const std::string& path);

std::string GetLastLogPath(std::string log_file_path);

void CreateLogFile(const std::string& log_file_path,
                   std::ofstream& log_file_stream);

void CloseLogFileStream(std::ofstream& log_file_stream);

// Delete the oldest files in `directory` that match `file_prefix` until only
// `files_to_keep` files remain.
void DeleteOldestFiles(std::string_view directory,
                       std::optional<std::string_view> file_prefix,
                       size_t files_to_keep);

}  // namespace os
}  // namespace bluetooth_hal
