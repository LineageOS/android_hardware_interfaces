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

#define LOG_TAG "bluetooth_hal.hal_config"

#include "bluetooth_hal/config/hal_config_loader.h"

#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/config/config_constants.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/android_base_wrapper.h"
#include "google/protobuf/util/json_util.h"
#include "hal_config.pb.h"

namespace bluetooth_hal {
namespace config {
namespace {

using ::bluetooth_hal::Property;
using ::bluetooth_hal::config::proto::HalConfig;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::transport::TransportType;
using ::bluetooth_hal::uart::BaudRate;
using ::bluetooth_hal::util::AndroidBaseWrapper;

using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonStringToMessage;

namespace cfg_consts = ::bluetooth_hal::config::constants;

std::string TransportTypeToString(TransportType type) {
  return std::to_string(static_cast<int>(type));
}

template <typename T>
std::string VectorToString(const std::vector<T>& vec) {
  std::stringstream ss;
  ss << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    if constexpr (std::is_same_v<T, TransportType>) {
      ss << TransportTypeToString(vec[i]);
    } else {
      ss << vec[i];
    }
    if (i < vec.size() - 1) {
      ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

}  // namespace

class HalConfigLoaderImpl : public HalConfigLoader {
 public:
  HalConfigLoaderImpl();
  ~HalConfigLoaderImpl() override = default;

  bool LoadConfig() override;
  bool LoadConfigFromFile(std::string_view path) override;
  bool LoadConfigFromString(std::string_view content) override;

  bool IsFastDownloadEnabled() const override;
  bool IsSarBackoffHighResolutionEnabled() const override;
  int GetBtRegOnDelayMs() const override;
  const std::string& GetBtUartDevicePort() const override;
  const std::vector<TransportType>& GetTransportTypePriority() const override;
  bool IsAcceleratedBtOnSupported() const override;
  bool IsThreadDispatcherEnabled() const override;
  bool IsBtPowerControlledByLpp() const override;
  const std::vector<std::string>& GetHwStagesWithoutLppControlBtPowerPin()
      const override;
  const std::vector<std::string>& GetUnsupportedHwStages() const override;
  int GetVendorTransportCrashIntervalSec() const override;
  bool IsHpUartSkipSuspendSupported() const override;
  bool IsEnergyControllerLoggingSupported() const override;
  bool IsBtHalRestartRecoverySupported() const override;
  bool IsBleNonConnectionSarEnabled() const override;
  int GetKernelRxWakelockTimeMilliseconds() const override;
  bool IsLowPowerModeSupported() const override;
  bool IsTranportFallbackEnabled() const override;
  bool IsBtSnoopLogFullModeOn() const override;
  BaudRate GetUartBaudRate(TransportType type) const override;
  bool IsUserDebugOrEngBuild() const override;
  const std::string& GetLpmEnableProcNode() const override;
  const std::string& GetLpmWakingProcNode() const override;
  const std::string& GetLpmWakelockCtrlProcNode() const override;
  const std::string& GetRfkillFolderPrefix() const override;
  const std::string& GetRfkillTypeBluetooth() const override;
  bool IsEnhancedPacketValidationSupported() const override;

  std::string DumpConfigToString() const override;

 private:
  void UpdateBqrEventMask(const std::string& mask);
  void UpdateTransportFallbackType(int type);
  void UpdateLdacQualityMode(const std::string& mode);

  int reg_on_delay_ms_{cfg_consts::kDefaultBtRegOnDelay};
  int vendor_transport_crash_interval_sec_{
      cfg_consts::kDefaultVendorTransportCrashIntervalSec};
  std::string uart_device_port_{cfg_consts::kDefaultBtUartDevicePort};
  std::vector<TransportType> transport_priority_list_{
      cfg_consts::kDefaultBtTransportType};
  std::vector<std::string> hw_stages_without_lpp_control_bt_power_pin_;
  std::vector<std::string> unsupported_hw_stages_;
  TransportType transport_fallback_type_{cfg_consts::kDefaultBtTransportType};
  bool is_fast_download_enabled_{false};
  bool is_sar_backoff_high_resolution_enabled_{false};
  bool is_accel_bt_on_enabled_{false};
  bool is_thread_dispatcher_enabled_{false};
  bool is_bt_power_controlled_by_lpp_{false};
  bool is_debug_image_build_{IsUserDebugOrEngBuild()};
  bool is_hp_uart_skip_suspend_enabled_{false};
  bool is_energy_controller_logging_enabled_{false};
  bool is_self_restart_recovery_enabled_{false};
  bool is_ble_non_connection_sar_enabled_{false};
  int kernel_rx_wake_lock_time_ms_{0};
  bool is_low_power_mode_enabled_{false};
  bool is_enhanced_packet_validation_supported_{false};
  std::string lpm_enable_proc_node_{cfg_consts::kLpmEnableProcNode};
  std::string lpm_waking_proc_node_{cfg_consts::kLpmWakingProcNode};
  std::string lpm_wakelock_ctrl_proc_node_{
      cfg_consts::kLpmWakelockCtrlProcNode};
  std::string rfkill_folder_prefix_{cfg_consts::kRfkillFolderPrefix};
  std::string rfkill_type_bluetooth_{cfg_consts::kRfkillTypeBluetooth};
};

bool HalConfigLoaderImpl::IsFastDownloadEnabled() const {
  return is_fast_download_enabled_;
}

bool HalConfigLoaderImpl::IsSarBackoffHighResolutionEnabled() const {
  return is_sar_backoff_high_resolution_enabled_;
}

int HalConfigLoaderImpl::GetBtRegOnDelayMs() const { return reg_on_delay_ms_; }

const std::string& HalConfigLoaderImpl::GetBtUartDevicePort() const {
  return uart_device_port_;
}

const std::vector<TransportType>&
HalConfigLoaderImpl::GetTransportTypePriority() const {
  if (IsTranportFallbackEnabled() &&
      transport_fallback_type_ != TransportType::kUnknown) {
    static const std::vector<TransportType> fallback_types{
        transport_fallback_type_};
    return fallback_types;
  }
  return transport_priority_list_;
}

bool HalConfigLoaderImpl::IsAcceleratedBtOnSupported() const {
  return is_accel_bt_on_enabled_;
}

bool HalConfigLoaderImpl::IsThreadDispatcherEnabled() const {
  return is_thread_dispatcher_enabled_;
}

bool HalConfigLoaderImpl::IsBtPowerControlledByLpp() const {
  return is_bt_power_controlled_by_lpp_;
}

const std::vector<std::string>&
HalConfigLoaderImpl::GetHwStagesWithoutLppControlBtPowerPin() const {
  return hw_stages_without_lpp_control_bt_power_pin_;
}

const std::vector<std::string>& HalConfigLoaderImpl::GetUnsupportedHwStages()
    const {
  return unsupported_hw_stages_;
}

int HalConfigLoaderImpl::GetVendorTransportCrashIntervalSec() const {
  return vendor_transport_crash_interval_sec_;
}

bool HalConfigLoaderImpl::IsHpUartSkipSuspendSupported() const {
  return is_hp_uart_skip_suspend_enabled_;
}

bool HalConfigLoaderImpl::IsEnergyControllerLoggingSupported() const {
  return is_energy_controller_logging_enabled_;
}

bool HalConfigLoaderImpl::IsBtHalRestartRecoverySupported() const {
  return is_self_restart_recovery_enabled_;
}

bool HalConfigLoaderImpl::IsBleNonConnectionSarEnabled() const {
  return is_ble_non_connection_sar_enabled_;
}

int HalConfigLoaderImpl::GetKernelRxWakelockTimeMilliseconds() const {
  return kernel_rx_wake_lock_time_ms_;
}

bool HalConfigLoaderImpl::IsLowPowerModeSupported() const {
  return is_low_power_mode_enabled_;
}

bool HalConfigLoaderImpl::IsUserDebugOrEngBuild() const {
  const std::string build_type =
      AndroidBaseWrapper::GetWrapper().GetProperty(Property::kBuildType, "");
  return build_type == "userdebug" || build_type == "eng";
}

bool HalConfigLoaderImpl::IsTranportFallbackEnabled() const {
  return AndroidBaseWrapper::GetWrapper().GetBoolProperty(
      Property::kTransportFallbackEnabled, false);
}

BaudRate HalConfigLoaderImpl::GetUartBaudRate(
    [[maybe_unused]] TransportType type) const {
  // TODO: b/421025035 - Put the value into the json file.
  return BaudRate::kRate4000000;
}

bool HalConfigLoaderImpl::IsBtSnoopLogFullModeOn() const {
  const std::string bt_snoop_full_mode("full");

  const std::string bt_snoop_log_mode =
      AndroidBaseWrapper::GetWrapper().GetProperty(Property::kBtSnoopLogMode,
                                                   "disabled");

  return bt_snoop_log_mode == bt_snoop_full_mode;
}

const std::string& HalConfigLoaderImpl::GetLpmEnableProcNode() const {
  return lpm_enable_proc_node_;
}

const std::string& HalConfigLoaderImpl::GetLpmWakingProcNode() const {
  return lpm_waking_proc_node_;
}

const std::string& HalConfigLoaderImpl::GetLpmWakelockCtrlProcNode() const {
  return lpm_wakelock_ctrl_proc_node_;
}

const std::string& HalConfigLoaderImpl::GetRfkillFolderPrefix() const {
  return rfkill_folder_prefix_;
}

const std::string& HalConfigLoaderImpl::GetRfkillTypeBluetooth() const {
  return rfkill_type_bluetooth_;
}

bool HalConfigLoaderImpl::IsEnhancedPacketValidationSupported() const {
  return is_enhanced_packet_validation_supported_;
}

void HalConfigLoaderImpl::UpdateBqrEventMask(const std::string& mask) {
  const std::string current_bqr_event_mask =
      AndroidBaseWrapper::GetWrapper().GetProperty(Property::kBqrEventMask,
                                                   "false");

  if ((mask != current_bqr_event_mask) && is_debug_image_build_) {
    LOG(INFO) << __func__ << ": Set to default bqr.event_mask: " << mask << ".";
    AndroidBaseWrapper::GetWrapper().SetProperty(Property::kBqrEventMask, mask);
  }
}

void HalConfigLoaderImpl::UpdateTransportFallbackType(int type) {
  const auto fallback_type = static_cast<TransportType>(type);
  transport_fallback_type_ = (fallback_type >= TransportType::kUartH4 &&
                              fallback_type < TransportType::kUnknown)
                                 ? fallback_type
                                 : TransportType::kUnknown;
}

void HalConfigLoaderImpl::UpdateLdacQualityMode(const std::string& mode) {
  LOG(INFO) << __func__ << ": Set " << Property::kLdacDefaultQualityMode << ": "
            << mode << ".";
  AndroidBaseWrapper::GetWrapper().SetProperty(
      Property::kLdacDefaultQualityMode, mode);
}

HalConfigLoaderImpl::HalConfigLoaderImpl() {
#ifndef UNIT_TEST
  LoadConfig();
#endif
}

bool HalConfigLoaderImpl::LoadConfig() {
  return LoadConfigFromFile(constants::kHalConfigFile);
}

bool HalConfigLoaderImpl::LoadConfigFromFile(std::string_view path) {
  std::ifstream json_file(path.data());
  if (!json_file.is_open()) {
    LOG(ERROR) << __func__ << ": Failed to open json file " << path.data();
    return false;
  }

  std::string json_str((std::istreambuf_iterator<char>(json_file)),
                       std::istreambuf_iterator<char>());

  return LoadConfigFromString(json_str);
}

bool HalConfigLoaderImpl::LoadConfigFromString(std::string_view content) {
  HalConfig config;
  JsonParseOptions options;
  options.ignore_unknown_fields = true;

  auto status = JsonStringToMessage(content, &config, options);
  if (!status.ok()) {
    LOG(ERROR) << __func__
               << ": Failed to parse json file, error: " << status.message();
    return false;
  }

  if (config.has_fast_download_enabled()) {
    is_fast_download_enabled_ = config.fast_download_enabled();
  }

  if (config.has_sar_backoff_high_resolution_enabled()) {
    is_sar_backoff_high_resolution_enabled_ =
        config.sar_backoff_high_resolution_enabled();
  }

  if (config.has_reg_on_delay_ms()) {
    reg_on_delay_ms_ = config.reg_on_delay_ms();
  }

  if (config.has_uart_device_port()) {
    uart_device_port_ = config.uart_device_port();
  }

  if (config.transport_type_priority_size()) {
    transport_priority_list_.clear();
    for (const auto type : config.transport_type_priority()) {
      transport_priority_list_.push_back(static_cast<TransportType>(type));
    }
  }

  if (config.has_accelerated_bt_on_enabled()) {
    is_accel_bt_on_enabled_ = config.accelerated_bt_on_enabled();
  }

  if (config.has_thread_dispatcher_enabled()) {
    is_thread_dispatcher_enabled_ = config.thread_dispatcher_enabled();
  }

  if (config.has_bt_power_controlled_by_lpp()) {
    is_bt_power_controlled_by_lpp_ = config.bt_power_controlled_by_lpp();
  }

  if (config.hw_stages_without_lpp_control_bt_power_pin_size()) {
    hw_stages_without_lpp_control_bt_power_pin_.clear();
    hw_stages_without_lpp_control_bt_power_pin_.assign(
        config.hw_stages_without_lpp_control_bt_power_pin().begin(),
        config.hw_stages_without_lpp_control_bt_power_pin().end());
  }

  if (config.unsupported_hw_stages_size()) {
    unsupported_hw_stages_.clear();
    unsupported_hw_stages_.assign(config.unsupported_hw_stages().begin(),
                                  config.unsupported_hw_stages().end());
  }

  if (config.has_vendor_transport_crash_interval_sec()) {
    vendor_transport_crash_interval_sec_ =
        config.vendor_transport_crash_interval_sec();
  }

  if (config.has_hp_uart_skip_suspend_enabled()) {
    is_hp_uart_skip_suspend_enabled_ = config.hp_uart_skip_suspend_enabled();
  }

  if (config.has_energy_controller_logging_enabled()) {
    is_energy_controller_logging_enabled_ =
        config.energy_controller_logging_enabled();
  }

  if (config.has_self_restart_recovery_enabled()) {
    is_self_restart_recovery_enabled_ = config.self_restart_recovery_enabled();
  }

  if (config.has_ble_non_connection_sar_enabled()) {
    is_ble_non_connection_sar_enabled_ =
        config.ble_non_connection_sar_enabled();
  }

  if (config.has_kernel_rx_wakelock_time_ms()) {
    kernel_rx_wake_lock_time_ms_ = config.kernel_rx_wakelock_time_ms();
  }

  if (config.has_low_power_mode_enabled()) {
    is_low_power_mode_enabled_ = config.low_power_mode_enabled();
  }

  if (config.has_bqr_event_mask()) {
    UpdateBqrEventMask(config.bqr_event_mask());
  }

  if (config.has_ldac_quality_mode()) {
    UpdateLdacQualityMode(config.ldac_quality_mode());
  }

  if (config.has_transport_fallback_type()) {
    UpdateTransportFallbackType(config.transport_fallback_type());
  }

  if (config.has_lpm_enable_proc_node()) {
    lpm_enable_proc_node_ = config.lpm_enable_proc_node();
  }

  if (config.has_lpm_waking_proc_node()) {
    lpm_waking_proc_node_ = config.lpm_waking_proc_node();
  }

  if (config.has_lpm_wakelock_ctrl_proc_node()) {
    lpm_wakelock_ctrl_proc_node_ = config.lpm_wakelock_ctrl_proc_node();
  }

  if (config.has_rfkill_folder_prefix()) {
    rfkill_folder_prefix_ = config.rfkill_folder_prefix();
  }

  if (config.has_rfkill_type_bluetooth()) {
    rfkill_type_bluetooth_ = config.rfkill_type_bluetooth();
  }

  if (config.has_enhanced_packet_validation_supported()) {
    is_enhanced_packet_validation_supported_ =
        config.enhanced_packet_validation_supported();
  }

  LOG(INFO) << DumpConfigToString();

  return true;
}

std::string HalConfigLoaderImpl::DumpConfigToString() const {
  std::stringstream ss;
  ss << std::boolalpha;

  ss << "--- HalConfigLoader State ---\n";
  ss << "IsFastDownloadEnabled: " << IsFastDownloadEnabled() << "\n";
  ss << "IsSarBackoffHighResolutionEnabled: "
     << IsSarBackoffHighResolutionEnabled() << "\n";
  ss << "GetBtRegOnDelayMs: " << GetBtRegOnDelayMs() << "\n";
  ss << "GetBtUartDevicePort: \"" << GetBtUartDevicePort() << "\"\n";
  ss << "GetTransportTypePriority (Effective): "
     << VectorToString(GetTransportTypePriority()) << "\n";
  ss << "  (Configured List): " << VectorToString(transport_priority_list_)
     << "\n";
  ss << "  (Fallback Type): " << TransportTypeToString(transport_fallback_type_)
     << "\n";
  ss << "IsAcceleratedBtOnSupported: " << IsAcceleratedBtOnSupported() << "\n";
  ss << "IsThreadDispatcherEnabled: " << IsThreadDispatcherEnabled() << "\n";
  ss << "IsBtPowerControlledByLpp: " << IsBtPowerControlledByLpp() << "\n";
  ss << "GetHwStagesWithoutLppControlBtPowerPin: "
     << VectorToString(GetHwStagesWithoutLppControlBtPowerPin()) << "\n";
  ss << "GetUnsupportedHwStages: " << VectorToString(GetUnsupportedHwStages())
     << "\n";
  ss << "GetVendorTransportCrashIntervalSec: "
     << GetVendorTransportCrashIntervalSec() << "\n";
  ss << "IsHpUartSkipSuspendSupported: " << IsHpUartSkipSuspendSupported()
     << "\n";
  ss << "IsEnergyControllerLoggingSupported: "
     << IsEnergyControllerLoggingSupported() << "\n";
  ss << "IsBtHalRestartRecoverySupported: " << IsBtHalRestartRecoverySupported()
     << "\n";
  ss << "IsBleNonConnectionSarEnabled: " << IsBleNonConnectionSarEnabled()
     << "\n";
  ss << "GetKernelRxWakelockTimeMilliseconds: "
     << GetKernelRxWakelockTimeMilliseconds() << "\n";
  ss << "IsLowPowerModeSupported: " << IsLowPowerModeSupported() << "\n";
  // Runtime checks.
  ss << "--- Runtime Checks ---\n";
  ss << "IsTranportFallbackEnabled (Property): " << IsTranportFallbackEnabled()
     << "\n";
  ss << "IsBtSnoopLogFullModeOn (Property): " << IsBtSnoopLogFullModeOn()
     << "\n";
  ss << "GetLpmEnableProcNode: \"" << GetLpmEnableProcNode() << "\"\n";
  ss << "GetLpmWakingProcNode: \"" << GetLpmWakingProcNode() << "\"\n";
  ss << "GetLpmWakelockCtrlProcNode: \"" << GetLpmWakelockCtrlProcNode()
     << "\"\n";
  ss << "GetRfkillFolderPrefix: \"" << GetRfkillFolderPrefix() << "\"\n";
  ss << "GetRfkillTypeBluetooth: \"" << GetRfkillTypeBluetooth() << "\"\n";
  ss << "IsEnhancedPacketValidationSupported: \""
     << IsEnhancedPacketValidationSupported() << "\"\n";

  // Runtime checks.
  ss << "IsUserDebugOrEngBuild (Property): " << IsUserDebugOrEngBuild() << "\n";
  ss << "---------------------------------\n";

  return ss.str();
}

HalConfigLoader& HalConfigLoader::GetLoader() {
  std::lock_guard<std::mutex> lock(loader_mutex_);
  if (loader_ == nullptr) {
    loader_ = new HalConfigLoaderImpl();
  }
  return *loader_;
}

void HalConfigLoader::ResetLoader() {
  std::lock_guard<std::mutex> lock(loader_mutex_);
  if (loader_ != nullptr) {
    delete loader_;
    loader_ = nullptr;
  }
}

}  // namespace config
}  // namespace bluetooth_hal
