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

#define LOG_TAG "bluetooth_hal.device_control"

#include "bluetooth_hal/transport/device_control/power_manager.h"

#include <sys/types.h>

#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>

#include "android-base/logging.h"
#include "bluetooth_hal/bqr/bqr_types.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace transport {
namespace {

using ::android::base::unique_fd;
using ::bluetooth_hal::bqr::BqrErrorCode;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::util::SystemCallWrapper;

// TODO: b/391226112 - Move to property config manager.
constexpr std::chrono::milliseconds kLpmWakeupSettlementMs{10};

std::string GetRfkillStatePath() {
  std::string state_path;
  constexpr int kMaxRfkillNodes = 256;

  for (int i = 0; i < kMaxRfkillNodes; ++i) {
    const std::string type_path =
        HalConfigLoader::GetLoader().GetRfkillFolderPrefix() +
        std::to_string(i) + "/type";
    unique_fd fd(
        SystemCallWrapper::GetWrapper().Open(type_path.c_str(), O_RDONLY));

    if (!fd.ok()) {
      LOG(INFO) << __func__ << ": Open(" << type_path
                << "): " << strerror(errno) << " (" << errno << ").";
      break;
    }

    std::array<char, 16> buffer{};
    const ssize_t length =
        TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Read(
            fd.get(), buffer.data(), buffer.size() - 1));

    if (length < 1) {
      continue;
    }

    if (buffer[length - 1] == '\n') {
      buffer[length - 1] = '\0';
    }

    LOG(DEBUG) << __func__ << ": rfkill candidate " << type_path << " is ["
               << buffer.data() << "].";

    if ((std::string_view(buffer.data()) ==
         HalConfigLoader::GetLoader().GetRfkillTypeBluetooth())) {
      state_path = HalConfigLoader::GetLoader().GetRfkillFolderPrefix() +
                   std::to_string(i) + "/state";
      LOG(INFO) << __func__ << ": Use rfkill " << state_path << ".";
      break;
    }
  }

  return state_path;
}

}  // namespace

// TODO: b/421766932 - Add battery level query.

bool PowerManager::PowerControl(bool is_enabled) {
  SCOPED_ANCHOR(AnchorType::kPowerControl, __func__);

  const std::string state_path = GetRfkillStatePath();
  if (state_path.empty()) {
    LOG(INFO) << __func__
              << ": Power sequence is not controlled by Bluetooth HAL.";
    return true;
  }

  unique_fd fd(
      SystemCallWrapper::GetWrapper().Open(state_path.c_str(), O_WRONLY));
  if (!fd.ok()) {
    LOG(ERROR) << __func__ << ": Unable to open rfkill state {" << state_path
               << "}: " << strerror(errno) << " (" << errno << ")";
#ifndef UNIT_TEST
    DebugCentral::Get().ReportBqrError(BqrErrorCode::kHostPowerUpController,
                                       "Unable to open rfkill state");
#endif
    return false;
  }

  ANCHOR_LOG_INFO(AnchorType::kLowPowerMode)
      << __func__ << ": " << (is_enabled ? "Enabling" : "Disabling")
      << ", state_path: " << state_path;

  char power = is_enabled ? '1' : '0';
  const ssize_t length =
      SystemCallWrapper::GetWrapper().Write(fd.get(), &power, sizeof(power));

  if (length < 1) {
    LOG(ERROR) << __func__
               << ": Failed to change rfkill state: " << strerror(errno) << " ("
               << errno << ")";
#ifndef UNIT_TEST
    DebugCentral::Get().ReportBqrError(BqrErrorCode::kHostPowerUpController,
                                       "Cannot write power control data");
#endif
    return false;
  }

  return true;
}

bool PowerManager::SetupLowPowerMode() {
  HAL_LOG(INFO) << __func__ << ": LPM enabling";

  lpm_fd_.reset(SystemCallWrapper::GetWrapper().Open(
      HalConfigLoader::GetLoader().GetLpmWakingProcNode().c_str(), O_WRONLY));
  if (!lpm_fd_.ok()) {
    HAL_LOG(WARNING) << __func__ << ": Unable to open LPM control port ("
                     << HalConfigLoader::GetLoader().GetLpmWakingProcNode()
                     << "): " << strerror(errno) << " (" << errno << ").";
    return false;
  }

  // Enable Host LPM.
  unique_fd enable_fd(SystemCallWrapper::GetWrapper().Open(
      HalConfigLoader::GetLoader().GetLpmEnableProcNode().c_str(), O_WRONLY));
  if (!enable_fd.ok()) {
    HAL_LOG(WARNING) << __func__ << ": Unable to open LPM driver, "
                     << strerror(errno) << "(" << errno << ")";
    return false;
  }

  constexpr char enable_cmd = '1';
  ssize_t length = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
      enable_fd.get(), &enable_cmd, sizeof(enable_cmd)));
  if (length < 1) {
    LOG(WARNING) << __func__ << ": Unable to enable LPM driver ("
                 << HalConfigLoader::GetLoader().GetLpmEnableProcNode()
                 << "): " << strerror(errno) << " (" << errno << ").";
    TeardownLowPowerMode();
    return false;
  }

  length = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
      lpm_fd_.get(), &enable_cmd, sizeof(enable_cmd)));
  if (length < 1) {
    HAL_LOG(WARNING) << __func__
                     << ": Unable to wake up LPM:" << strerror(errno) << " ("
                     << errno << ").";
    TeardownLowPowerMode();
    return false;
  }

  return true;
}

void PowerManager::TeardownLowPowerMode() {
  HAL_LOG(INFO) << __func__ << ": LPM disabling.";

  lpm_fd_.reset();

  unique_fd disable_fd(SystemCallWrapper::GetWrapper().Open(
      HalConfigLoader::GetLoader().GetLpmEnableProcNode().c_str(), O_WRONLY));
  if (!disable_fd.ok()) {
    HAL_LOG(WARNING) << __func__ << ": Unable to close LPM driver ("
                     << HalConfigLoader::GetLoader().GetLpmEnableProcNode()
                     << "): " << strerror(errno) << " (" << errno << ").";
    return;
  }

  constexpr char disable_cmd = '0';
  const ssize_t length = SystemCallWrapper::GetWrapper().Write(
      disable_fd.get(), &disable_cmd, sizeof(disable_cmd));
  if (length < 1) {
    LOG(WARNING) << __func__ << ": Unable to disable LPM driver ("
                 << HalConfigLoader::GetLoader().GetLpmEnableProcNode()
                 << "): " << strerror(errno) << " (" << errno << ")";
  }
}

bool PowerManager::ResumeFromLowPowerMode() {
  if (!lpm_fd_.ok()) {
    // LPM is not enabled.
    return true;
  }

  constexpr char resume_cmd = '1';
  const ssize_t length =
      TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
          lpm_fd_.get(), &resume_cmd, sizeof(resume_cmd)));
  if (length < 1) {
    HAL_LOG(ERROR) << __func__ << ": Unable to wake up LPM:" << strerror(errno)
                   << " (" << errno << ").";
    return false;
  }

  std::this_thread::sleep_for(kLpmWakeupSettlementMs);
  HAL_LOG(VERBOSE) << __func__ << ": Assert";
  return true;
}

bool PowerManager::SuspendToLowPowerMode() {
  if (!lpm_fd_.ok()) {
    // LPM is not enabled.
    return true;
  }

  constexpr char suspend_cmd = '0';
  const ssize_t length =
      TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
          lpm_fd_.get(), &suspend_cmd, sizeof(suspend_cmd)));
  if (length < 1) {
    HAL_LOG(ERROR) << __func__ << ": Unable to suspend LPM:" << strerror(errno)
                   << " (" << errno << ").";
    return false;
  }

  HAL_LOG(VERBOSE) << __func__ << ": Deassert";
  return true;
}

bool PowerManager::IsLowPowerModeSetupCompleted() const { return lpm_fd_.ok(); }

bool PowerManager::ConfigRxWakelockTime(int duration) {
  if (duration == 0) {
    return true;
  }

  if (duration < 0) {
    LOG(WARNING) << __func__ << ": Invalid value: " << duration;
    return false;
  }

  LOG(INFO) << __func__ << ": config rx wakelock time: " << duration;

  unique_fd wake_ctrl_fd(SystemCallWrapper::GetWrapper().Open(
      HalConfigLoader::GetLoader().GetLpmWakelockCtrlProcNode().c_str(),
      O_WRONLY));
  if (!wake_ctrl_fd.ok()) {
    LOG(WARNING) << __func__
                 << ": Unable to open Kernel Wakelock control port ("
                 << HalConfigLoader::GetLoader().GetLpmWakelockCtrlProcNode()
                 << "): " << strerror(errno) << " (" << errno << ").";
    return false;
  }

  const ssize_t length =
      TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
          wake_ctrl_fd.get(), &duration, sizeof(duration)));
  if (length < 1) {
    LOG(ERROR) << __func__
               << ": Unable to config kernel wakelock time:" << strerror(errno)
               << " (" << errno << ").";
    return false;
  }

  return true;
}

}  // namespace transport
}  // namespace bluetooth_hal
