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

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/debug/debug_client.h"

namespace bluetooth_hal {
namespace debug {

const std::string kCoredumpFilePath = "/data/vendor/ssrdump/coredump/";
const std::string kCoredumpPrefix = "coredump_bt_";
const std::regex kTimestampPattern(R"(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})");

inline std::string GenerateHalLogString(const std::string& title,
                                        const std::string& log,
                                        bool format_log = true) {
  std::stringstream ss;
  ss << "║\t=============================================" << std::endl;
  ss << "║\t" << title << std::endl;
  ss << "║\t=============================================" << std::endl;
  if (format_log) {
    std::istringstream iss(log);
    std::string line;
    while (std::getline(iss, line)) {
      ss << "║\t\t" << line << std::endl;
    }
  } else {
    ss << log;
  }
  ss << "║" << std::endl;
  return ss.str();
}

inline std::string GenerateHalLogStringFrame(const std::string& title,
                                             const std::string& log,
                                             bool format_log = true) {
  std::stringstream ss;
  ss << "╔══════════════════════════════════════════════════════════\n";
  ss << "║ BEGIN of " << title << std::endl;
  ss << "╠══════════════════════════════════════════════════════════\n";
  ss << "║\n";
  if (format_log) {
    std::istringstream iss(log);
    std::string line;
    while (std::getline(iss, line)) {
      ss << "║\t" << line << std::endl;
    }
  } else {
    ss << log;
  }
  ss << "║\n";
  ss << "╠══════════════════════════════════════════════════════════\n";
  ss << "║ END of " << title << std::endl;
  ss << "╚══════════════════════════════════════════════════════════\n";
  ss << std::endl;
  return ss.str();
}

inline std::string CoredumpToStringLog(const std::vector<Coredump> coredumps,
                                       const CoredumpPosition position) {
  std::stringstream ss;
  for (auto dump : coredumps) {
    if (dump.position == position) {
      switch (position) {
        case CoredumpPosition::kBegin:
        case CoredumpPosition::kEnd:
          ss << GenerateHalLogString(dump.title, dump.coredump);
          break;
        case CoredumpPosition::kCustomDumpsys:
          ss << dump.title << dump.coredump;
          break;
      }
    }
  }
  return ss.str();
}

inline std::string DumpDebugfs(const std::string& debugfs) {
  std::stringstream file_content_ss;
  std::ifstream file;

  file.open(debugfs);
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      file_content_ss << line << std::endl;
    }
    file.close();
  } else {
    file_content_ss << "Fail to read debugfs: " << debugfs << std::endl;
  }

  return GenerateHalLogString("Debugfs: " + debugfs, file_content_ss.str());
}

inline bool IsBinFilePatternMatch(const std::string& filename,
                                  const std::string& base_prefix) {
  if (!filename.starts_with(base_prefix)) {
    return false;
  }
  std::string remaining_part = filename.substr(base_prefix.length());

  if (!remaining_part.ends_with(".bin")) {
    return false;
  }

  std::string timestamp_str = remaining_part.substr(
      0, remaining_part.length() - std::string(".bin").length());
  return std::regex_match(timestamp_str, kTimestampPattern);
}

inline void DeleteOldestBinFiles(const std::string& directory,
                                 const std::string& base_file_prefix,
                                 size_t files_to_keep) {
  std::vector<std::filesystem::directory_entry> filtered_files;

  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const std::string filename = entry.path().filename().string();

    if (IsBinFilePatternMatch(filename, base_file_prefix)) {
      filtered_files.emplace_back(entry);
    }
  }

  // Sort files by their last write time
  std::sort(filtered_files.begin(), filtered_files.end(),
            [](const auto& a, const auto& b) {
              return std::filesystem::last_write_time(a) >
                     std::filesystem::last_write_time(b);
            });

  // Delete files, starting at files_to_keep
  for (size_t i = files_to_keep; i < filtered_files.size(); ++i) {
    std::filesystem::remove(filtered_files[i]);
    LOG(INFO) << "Deleted: " << filtered_files[i].path().c_str();
  }
}

inline void FlushCoredumpToFd(int fd) {
  std::unique_ptr<DIR, decltype(&closedir)> dir(
      opendir(kCoredumpFilePath.c_str()), closedir);
  if (!dir) {
    LOG(WARNING) << __func__
                 << ": Failed to open directory: " << kCoredumpFilePath;
    return;
  }

  std::stringstream combined_output_ss;
  struct dirent* entry;

  while ((entry = readdir(dir.get())) != nullptr) {
    std::string file_name = entry->d_name;

    if (file_name == "." || file_name == "..") {
      continue;
    }

    std::string full_path = kCoredumpFilePath + file_name;

    if (!IsBinFilePatternMatch(file_name, kCoredumpPrefix)) {
      continue;
    }

    struct stat file_stat;
    if (stat(full_path.c_str(), &file_stat) == -1 ||
        !S_ISREG(file_stat.st_mode)) {
      continue;
    }

    LOG(INFO) << __func__ << ": Dumping " << full_path;

    std::stringstream current_file_content_ss;
    std::ifstream input_file(full_path, std::ios::binary);
    if (!input_file.is_open()) {
      current_file_content_ss << "ERROR: Failed to open file: " << full_path
                              << std::endl;
      LOG(ERROR) << __func__ << ": Failed to open file: " << full_path;
    } else {
      current_file_content_ss << input_file.rdbuf();
      input_file.close();
    }

    std::string formatted_log = GenerateHalLogStringFrame(
        "LogFile: " + file_name, current_file_content_ss.str(), false);
    combined_output_ss << formatted_log;
  }

  std::string final_output = combined_output_ss.str();
  if (!final_output.empty()) {
    ssize_t bytes_written =
        write(fd, final_output.c_str(), final_output.length());
    if (bytes_written == -1) {
      LOG(ERROR) << __func__ << ": Failed to write to file descriptor " << fd
                 << ". Error: " << strerror(errno);
    } else if (static_cast<size_t>(bytes_written) != final_output.length()) {
      LOG(WARNING) << __func__ << ": Incomplete write to file descriptor " << fd
                   << ". Wrote " << bytes_written << " of "
                   << final_output.length() << " bytes.";
    }
  } else {
    LOG(INFO) << __func__ << ": No coredump files found to dump.";
  }
}

}  // namespace debug
}  // namespace bluetooth_hal
