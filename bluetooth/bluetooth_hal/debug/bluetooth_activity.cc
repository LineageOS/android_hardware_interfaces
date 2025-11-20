/*
 * Copyright 2022 The Android Open Source Project
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

#define LOG_TAG "bthal.activity"

#include "bluetooth_hal/debug/bluetooth_activity.h"

#include <sys/stat.h>

#include <future>
#include <mutex>
#include <sstream>
#include <string>

#include "android-base/logging.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/debug/command_error_code.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/files.h"
#include "bluetooth_hal/util/logging.h"

namespace {

using ::android::base::StringPrintf;
using ::bluetooth_hal::hci::CommandOpCode;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::Logger;

using Milliseconds = std::chrono::milliseconds;
using SteadyClockPoint = std::chrono::time_point<std::chrono::steady_clock>;

std::recursive_mutex mutex_activity;
timer_t acl_data_timer;
itimerspec acl_ts{};
bool acl_data_activity_flag = false;
bool acl_data_timer_created = false;
uint32_t acl_data_counter = 0;
uint32_t acl_tx_data_counter = 0;
uint32_t acl_rx_data_counter = 0;
uint32_t num_of_compl_packet = 0;
uint32_t hci_command_counter = 0;
uint32_t hci_event_counter = 0;
SteadyClockPoint first_acl_data_timepoint_;
acl_data_activities_t acl_stat{};

timer_t le_adv_timer;
itimerspec le_ts{};
bool le_adv_activity_flag = false;
bool le_adv_timer_created = false;
uint32_t le_adv_counter = 0;
SteadyClockPoint first_le_adv_timepoint_;
ble_adv_activities_t ble_stat{};

timer_t pkt_timer;
itimerspec pkt_ts{};
bool pkt_activity_flag = false;
bool pkt_timer_created = false;
int pkt_counter = 0;
SteadyClockPoint first_pkt_timepoint_;
SteadyClockPoint previous_last_pkt_timepoint_;
pkt_activities_t pkt_stat{};

bool is_logger_on = false;
constexpr uint16_t kBtContiBleAdvRecordPeriodMs = 10000;
constexpr uint16_t kBtContiAclDataRecordPeriodMs = 10000;
constexpr uint8_t kBtContiDataTimerExpiredSec = 1;
constexpr uint32_t kBtContiDataTimerExpiredNs = 0;

constexpr uint16_t kBtMaxConnectHistoryRecord = 1024;
constexpr uint8_t kConnectionCompleteEventCode = 0x03;
constexpr uint8_t kDisConnectionCompleteEventCode = 0x05;
constexpr uint8_t kLeAdvertisingEventCode = 0x3e;
constexpr uint8_t kNumberOfCompletedPacketsEvent = 0x13;

const std::string kBtActivitiesPacketLogPath =
    "/data/vendor/bluetooth/bt_activity_pkt.txt";
}  // namespace

namespace bluetooth_hal {
namespace debug {

void sum_of_acl_data(std::vector<acl_data_activities_t>& acl_data,
                     uint32_t* sum_of_duration, uint32_t* sum_of_acl_count,
                     uint32_t* sum_of_acltx_count,
                     uint32_t* sum_of_aclrx_count) {
  for (int i = 0; i < acl_data.size(); i++) {
    *sum_of_duration += acl_data[i].duration;
    *sum_of_acl_count += acl_data[i].acl_data_count;
    *sum_of_acltx_count += acl_data[i].acl_tx_data_count;
    *sum_of_aclrx_count += acl_data[i].acl_rx_data_count;
  }
}

void sum_of_ble_adv_data(std::vector<ble_adv_activities_t>& adv_data,
                         uint32_t* sum_of_duration, uint32_t* sum_of_count) {
  for (int i = 0; i < adv_data.size(); i++) {
    *sum_of_duration += adv_data[i].duration;
    *sum_of_count += adv_data[i].le_adv_count;
  }
}

BtActivitiesLogger bt_metrics_instance_;

std::ofstream BtActivitiesLogger::pkt_activity_ostream_;

BtActivitiesLogger* BtActivitiesLogger::GetInstacne() {
  return &bt_metrics_instance_;
}

void BtActivitiesLogger::update_connect_disconnect_history(
    const tCONN_DEVICE& device) {
  if (connection_history_.size() >= kBtMaxConnectHistoryRecord) {
    auto it = connection_history_.begin();
    // remove the eariset connecting/disconnecting record
    connection_history_.erase(it);
  }
  // push new connecting/disconnecting record to the back of list
  connection_history_.emplace_back(device);
}

void BtActivitiesLogger::open_new_hci_packet_log_file() {
  LOG(INFO) << __func__;
  bt_activities_pkt_log_path_ = std::move(kBtActivitiesPacketLogPath);
  os::CloseLogFileStream(pkt_activity_ostream_);
  os::CreateLogFile(bt_activities_pkt_log_path_, pkt_activity_ostream_);
  pkt_activity_ostream_ << "start_timestamp" << ", end_timestamp"
                        << ", pkt_duration (ms)" << ", idle_delta"
                        << ", packet_count" << ", total_packet_count"
                        << ", ble_adv:" << ", adv_timestamp"
                        << ", adv_duration (ms)" << ", adv_count"
                        << ", total_adv_count"
                        << ", acl_data:" << ", acl_timestamp"
                        << ", acl_duration (ms)" << ", acl_conn_handle"
                        << ", acl_data_count" << ", acl_tx_count"
                        << ", acl_rx_count" << ", total_acl_data_count"
                        << ", total_acl_tx_count" << ", total_acl_rx_count"
                        << ", cmd/evt:" << ", cmd_count" << ", evt_count"
                        << ", num_compl_pkt_evt_count" << std::endl;
  if (!pkt_activity_ostream_.flush()) {
    LOG(ERROR) << __func__ << ": Failed to flush, error: \"" << strerror(errno)
               << "\".";
  }
}

void BtActivitiesLogger::ForceUpdating() {
  LOG(INFO) << __func__;
  LeAdvTimeout();
  AclDataTimeout();
  HciPacketTimeout();
}

void BtActivitiesLogger::StartLogging() { open_new_hci_packet_log_file(); }

void BtActivitiesLogger::StopLogging() {
  LOG(INFO) << __func__;
  ForceUpdating();
  os::CloseLogFileStream(pkt_activity_ostream_);
}

void BtActivitiesLogger::OnBluetoothEnabled() {
  LOG(INFO) << __func__;
  int ret;
  ble_stat = {};
  acl_stat = {};
  pkt_stat = {};
  hci_command_counter = 0;
  hci_event_counter = 0;
  acl_data_counter = 0;
  acl_tx_data_counter = 0;
  acl_rx_data_counter = 0;
  num_of_compl_packet = 0;

  sigevent se{};
  se.sigev_notify = SIGEV_THREAD;
  se.sigev_value.sival_ptr = this;
  se.sigev_notify_function = (void (*)(sigval))LeAdvTimeout;
  se.sigev_notify_attributes = NULL;
  ret = timer_create(CLOCK_MONOTONIC, &se, &le_adv_timer);
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Cannot create le_adv_timer!";
  } else {
    le_adv_timer_created = true;
  }

  sigevent acl_se{};
  acl_se.sigev_notify = SIGEV_THREAD;
  acl_se.sigev_value.sival_ptr = this;
  acl_se.sigev_notify_function = (void (*)(sigval))AclDataTimeout;
  acl_se.sigev_notify_attributes = NULL;
  ret = timer_create(CLOCK_MONOTONIC, &acl_se, &acl_data_timer);
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Cannot create acl_data_timer!";
  } else {
    acl_data_timer_created = true;
  }

  sigevent pkt_se{};
  pkt_se.sigev_notify = SIGEV_THREAD;
  pkt_se.sigev_value.sival_ptr = this;
  pkt_se.sigev_notify_function = (void (*)(sigval))HciPacketTimeout;
  pkt_se.sigev_notify_attributes = NULL;
  ret = timer_create(CLOCK_MONOTONIC, &pkt_se, &pkt_timer);
  if (ret < 0) {
    LOG(ERROR) << __func__ << ": Cannot create pkt_timer!";
  } else {
    pkt_timer_created = true;
  }
  is_logger_on = true;
}

void BtActivitiesLogger::OnBluetoothDisabled() {
  LOG(INFO) << __func__;
  is_logger_on = false;
  if (le_adv_timer_created == true) {
    itimerspec se{};
    timer_settime(le_adv_timer, 0, &se, NULL);
    timer_delete(le_adv_timer);
    le_adv_timer_created = false;
  }
  if (acl_data_timer_created == true) {
    itimerspec acl_se{};
    timer_settime(acl_data_timer, 0, &acl_se, NULL);
    timer_delete(acl_data_timer);
    acl_data_timer_created = false;
  }
  if (pkt_timer_created == true) {
    itimerspec pkt_se{};
    timer_settime(pkt_timer, 0, &pkt_se, NULL);
    timer_delete(pkt_timer);
    pkt_timer_created = false;
  }

  LeAdvTimeout();
  AclDataTimeout();
  HciPacketTimeout();
  ble_stat = {};
  acl_stat = {};
  pkt_stat = {};
}

void BtActivitiesLogger::AclDataTimeout() {
  std::unique_lock<std::recursive_mutex> lock(mutex_activity);
  if (acl_data_activity_flag == true) {
    SteadyClockPoint last_acl_data_timepoint_ =
        std::chrono::steady_clock::now();
    Milliseconds delta_ms = std::chrono::duration_cast<Milliseconds>(
        last_acl_data_timepoint_ - first_acl_data_timepoint_);
    acl_stat.duration = delta_ms.count();
    acl_stat.end_timestamp = Logger::GetLogFormatTimestamp();
    acl_stat.acl_tx_data_count = acl_tx_data_counter;
    acl_stat.acl_rx_data_count = acl_rx_data_counter;
    acl_stat.acl_data_count = acl_data_counter;
    acl_stat.total_acl_data_count += acl_data_counter;

    LOG(INFO) << __func__
              << ": bt_power: AclDataStat: " << acl_stat.start_timestamp
              << " - " << acl_stat.end_timestamp
              << ", conn_handle: " << acl_stat.acl_conn_handle
              << ", duration: " << acl_stat.duration
              << ", acl_data_count: " << acl_stat.acl_data_count
              << ", acl_tx_data_count: " << acl_stat.acl_tx_data_count
              << ", acl_rx_data_count: " << acl_stat.acl_rx_data_count
              << ", total_acl_data_count: " << acl_stat.total_acl_data_count
              << ".";

    BtActivitiesLogger::GetInstacne()->UpdateAclDataStat(acl_stat);
    pkt_stat.acl_data.push_back(std::move(acl_stat));
    acl_data_activity_flag = false;
    acl_data_counter = 0;
    acl_tx_data_counter = 0;
    acl_rx_data_counter = 0;
  }
  acl_ts.it_value.tv_sec = 0;
  acl_ts.it_value.tv_nsec = 0;
  if (acl_data_timer_created == true && is_logger_on == true) {
    timer_settime(acl_data_timer, 0, &acl_ts, NULL);
  }
}

void BtActivitiesLogger::LeAdvTimeout() {
  std::unique_lock<std::recursive_mutex> lock(mutex_activity);
  if (le_adv_activity_flag == true) {
    SteadyClockPoint last_le_adv_timepoint_ = std::chrono::steady_clock::now();
    Milliseconds delta_ms = std::chrono::duration_cast<Milliseconds>(
        last_le_adv_timepoint_ - first_le_adv_timepoint_);
    ble_stat.duration = delta_ms.count();
    ble_stat.le_adv_count = le_adv_counter;
    ble_stat.total_le_adv_count += le_adv_counter;
    ble_stat.end_timestamp = Logger::GetLogFormatTimestamp();

    LOG(INFO) << __func__
              << ": bt_power: LeAdvStat: " << ble_stat.start_timestamp << " - "
              << ble_stat.end_timestamp << ", duration: " << ble_stat.duration
              << ", le_adv_count: " << ble_stat.le_adv_count
              << ", total_le_adv_count: " << ble_stat.total_le_adv_count
              << ", total_event_count: " << hci_event_counter << ".";

    BtActivitiesLogger::GetInstacne()->UpdateLeAdvStat(ble_stat);
    pkt_stat.ble_stat.push_back(std::move(ble_stat));
    le_adv_activity_flag = false;
    le_adv_counter = 0;
  }
  le_ts.it_value.tv_sec = 0;
  le_ts.it_value.tv_nsec = 0;
  if (le_adv_timer_created == true && is_logger_on == true) {
    timer_settime(le_adv_timer, 0, &le_ts, NULL);
  }
}

void BtActivitiesLogger::HciPacketTimeout() {
  std::unique_lock<std::recursive_mutex> lock(mutex_activity);
  if (pkt_activity_flag == true) {
    SteadyClockPoint last_pkt_timepoint_ = std::chrono::steady_clock::now();
    previous_last_pkt_timepoint_ = last_pkt_timepoint_;
    Milliseconds delta_ms = std::chrono::duration_cast<Milliseconds>(
        last_pkt_timepoint_ - first_pkt_timepoint_);
    pkt_stat.duration = delta_ms.count();
    pkt_stat.pkt_count = pkt_counter;
    pkt_stat.total_pkt_count += pkt_counter;
    pkt_stat.hci_command_count = hci_command_counter;
    pkt_stat.hci_event_count = hci_event_counter;
    pkt_stat.num_cmpl_pkt_evt_count = num_of_compl_packet;
    pkt_stat.end_timestamp = Logger::GetLogFormatTimestamp();
    pkt_activity_flag = false;
    pkt_counter = 0;
    std::future<void> future = std::async(
        BtActivitiesLogger::UpdateHciPacketStat, std::move(pkt_stat));
    future.wait();
  }
  hci_command_counter = 0;
  hci_event_counter = 0;
  pkt_ts.it_value.tv_sec = 0;
  pkt_ts.it_value.tv_nsec = 0;
  if (pkt_timer_created == true && is_logger_on == true) {
    timer_settime(pkt_timer, 0, &pkt_ts, NULL);
  }
}

void BtActivitiesLogger::handle_ble_meta_event(const HalPacket& data) {
  int ret;
  uint16_t le_meta_subevent = data[2];
  if (le_meta_subevent == 0x0d || le_meta_subevent == 0x02) {
    // LE Meta Event, SubEvent: LE Extended Advertising Report
    if (le_adv_activity_flag == false) {
      first_le_adv_timepoint_ = std::chrono::steady_clock::now();
      ble_stat.start_timestamp = Logger::GetLogFormatTimestamp();
      le_adv_activity_flag = true;
    }
    le_adv_counter++;
    // update timer
    le_ts.it_value.tv_sec = kBtContiDataTimerExpiredSec;
    le_ts.it_value.tv_nsec = kBtContiDataTimerExpiredNs;
    if (le_adv_timer_created == true && is_logger_on == true) {
      ret = timer_settime(le_adv_timer, 0, &le_ts, NULL);
      if (ret < 0) {
        LOG(ERROR) << __func__ << ": Cannot arm le_adv_timer!";
      }
    }
  } else if (le_meta_subevent == 0x0a && data[3] == 0x00) {
    // LE Meta Event, SubEvent: LE Enhanced Connection Complete
    tCONN_DEVICE bt_device{};
    bt_device.connect_handle = data[4] + ((data[5] << 8u) & 0x0F00);
    bt_device.bd_addr = StringPrintf("XX:XX:%02hhx:%02hhx:%02hhx:%02hhx",
                                     data[11], data[10], data[9], data[8]);
    bt_device.status = GetResultString(data[3]);
    if (bt_device.status == "Success") {
      connected_bda_[bt_device.connect_handle] = bt_device.bd_addr;
    }
    bt_device.timestamp = Logger::GetLogFormatTimestamp();
    bt_device.rcvd_event = "LE Enhanced Connection Complete";
    LOG(INFO) << __func__
              << ": LE Enhanced Connection Complete, conn_handle: " << std::hex
              << std::setw(3) << std::setfill('0') << bt_device.connect_handle
              << ", conn_bda: " << bt_device.bd_addr << ".";

    // push a new coming record to the back of list
    update_connect_disconnect_history(bt_device);
  }
}

void BtActivitiesLogger::handle_connect_complete_event(const HalPacket& data) {
  tCONN_DEVICE bt_device{};
  uint16_t conn_handle = data[3] + ((data[4] << 8u) & 0x0F00);
  bt_device.connect_handle = conn_handle;
  bt_device.timestamp = Logger::GetLogFormatTimestamp();
  bt_device.status = GetResultString(data[2]);
  bt_device.bd_addr = StringPrintf("XX:XX:%02hhx:%02hhx:%02hhx:%02hhx", data[8],
                                   data[7], data[6], data[5]);
  if (bt_device.status == "Success") {
    connected_bda_[conn_handle] = bt_device.bd_addr;
    LOG(INFO) << __func__
              << ": CreateConnectCompleteEvent, conn_handle: " << std::hex
              << std::setw(3) << std::setfill('0') << bt_device.connect_handle
              << ", conn_bda: " << bt_device.bd_addr << ".";
  }
  bt_device.rcvd_event = "Connect Complete";
  // push a new coming record to the back of list
  update_connect_disconnect_history(bt_device);
}

void BtActivitiesLogger::handle_disconnect_complete_event(
    const HalPacket& data) {
  tCONN_DEVICE bt_device{};
  uint16_t conn_handle = data[3] + ((data[4] << 8u) & 0x0F00);
  bt_device.connect_handle = conn_handle;
  bt_device.bd_addr = connected_bda_[conn_handle];
  bt_device.timestamp = Logger::GetLogFormatTimestamp();
  bt_device.status = GetResultString(data[5]);
  if (bt_device.status == "Success") {
    connected_bda_.erase(conn_handle);
    LOG(INFO) << __func__
              << ": DisConnectCompleteEvent, conn_handle: " << std::hex
              << std::setw(3) << std::setfill('0') << bt_device.connect_handle
              << ", conn_bda: " << bt_device.bd_addr << ".";
  }
  bt_device.rcvd_event = "Disconnect Complete";
  // push a new disconnecting record to the back of list
  update_connect_disconnect_history(bt_device);
}

void BtActivitiesLogger::LogActivities(BtActivityPacketType activity_type,
                                       const HalPacket& data) {
  int ret;
  uint16_t opcode;
  std::unique_lock<std::recursive_mutex> lock(mutex_activity);
  if (pkt_activity_flag == false) {
    pkt_stat.ble_stat = {};
    pkt_stat.acl_data = {};
    pkt_counter = 0;
    num_of_compl_packet = 0;
    first_pkt_timepoint_ = std::chrono::steady_clock::now();
    if (first_pkt_timepoint_ > previous_last_pkt_timepoint_) {
      Milliseconds delta_ms = std::chrono::duration_cast<Milliseconds>(
          first_pkt_timepoint_ - previous_last_pkt_timepoint_);
      pkt_stat.delta = delta_ms.count();
    } else {
      pkt_stat.delta = 0;
    }
    pkt_stat.start_timestamp = Logger::GetLogFormatTimestamp();
    pkt_activity_flag = true;
  }
  pkt_counter++;
  switch (activity_type) {
    case BtActivityPacketType::COMMAND:
      hci_command_counter++;
      opcode = data[0] + ((data[1] << 8u) & 0xFF00);
      if (opcode == static_cast<uint16_t>(CommandOpCode::kLeScanEnable)) {
        if (data[3] == 0x01) {
          // Enable
          LOG(INFO) << __func__ << ": Enable LE Scanning, 0x" << std::hex
                    << std::setw(2) << std::setfill('0') << opcode << ".";
        } else if (data[3] == 0x00) {
          // Disable
          LOG(INFO) << __func__ << ": Disable LE Scanning, 0x" << std::hex
                    << std::setw(2) << std::setfill('0') << opcode << ".";
        } else {
          LOG(INFO) << __func__ << ": Invalid Parameter of LE_Set_Scan_Enable.";
        }
      } else if (opcode == static_cast<uint16_t>(
                               CommandOpCode::kLeSetExtendedScanParam)) {
        uint16_t scan_interval = data[7] + ((data[8] << 8u) & 0xFF00);
        uint16_t scan_window = data[9] + ((data[10] << 8u) & 0xFF00);
        int ratio = scan_interval / scan_window;
        if (ratio == 1) {
          LOG(INFO) << __func__ << ": LOW_LATENCY ScanMode.";
        } else if (ratio == 4) {
          LOG(INFO) << __func__ << ": BALANCED ScanMode.";

        } else if (ratio == 10) {
          LOG(INFO) << __func__ << ": LOW_POWER ScanMode.";
        } else {
          LOG(INFO) << __func__ << ": Other ScanMode, ratio: " << ratio << ".";
        }
      } else if (opcode ==
                 static_cast<uint16_t>(CommandOpCode::kLeExtCreateConnection)) {
        LOG(INFO) << __func__ << ": LE Extended Create Connection.";
      }
      break;
    case BtActivityPacketType::EVENT:
      hci_event_counter++;
      // LE Meta Event
      if (data[0] == kLeAdvertisingEventCode) {
        handle_ble_meta_event(data);
      } else if (data[0] == kConnectionCompleteEventCode) {
        handle_connect_complete_event(data);
      } else if (data[0] == kDisConnectionCompleteEventCode) {
        handle_disconnect_complete_event(data);
      } else if (data[0] == kNumberOfCompletedPacketsEvent) {
        num_of_compl_packet++;
      }
      break;
    case BtActivityPacketType::ACL_TX_DATA:
    case BtActivityPacketType::ACL_RX_DATA: {
      uint16_t conn_handle = data[0] + ((data[1] << 8u) & 0x0F00);
      acl_data_counter++;
      if (acl_data_activity_flag == false) {
        acl_stat.acl_conn_handle = conn_handle;
        first_acl_data_timepoint_ = std::chrono::steady_clock::now();
        acl_stat.start_timestamp = Logger::GetLogFormatTimestamp();
        acl_data_activity_flag = true;
      }
      if (activity_type == BtActivityPacketType::ACL_TX_DATA) {
        acl_tx_data_counter++;
        acl_stat.total_acl_tx_data_count++;
      } else {
        acl_rx_data_counter++;
        acl_stat.total_acl_rx_data_count++;
      }
      // update timer
      acl_ts.it_value.tv_sec = kBtContiDataTimerExpiredSec;
      acl_ts.it_value.tv_nsec = kBtContiDataTimerExpiredNs;
      if (acl_data_timer_created == true && is_logger_on == true) {
        ret = timer_settime(acl_data_timer, 0, &acl_ts, NULL);
        if (ret < 0) {
          LOG(ERROR) << __func__ << ": Cannot arm acl_data_timer!";
        }
      }
    } break;
    default:
      LOG(WARNING) << __func__ << ": Unexpected Packet Type: "
                   << static_cast<uint8_t>(activity_type) << ".";
      break;
  }

  // update pkt_timer
  pkt_ts.it_value.tv_sec = kBtContiDataTimerExpiredSec;
  pkt_ts.it_value.tv_nsec = kBtContiDataTimerExpiredNs;
  if (pkt_timer_created == true && is_logger_on == true) {
    ret = timer_settime(pkt_timer, 0, &pkt_ts, NULL);
    if (ret < 0) {
      LOG(ERROR) << __func__ << ": Cannot arm pkt_timer!";
    }
  }
}

void BtActivitiesLogger::UpdateBthalWakelockStat(
    const bthal_wakelock_stat_t& stat) {
  LOG(INFO) << __func__
            << ": bt_power: TxWakeLockStat: " << stat.start_timestamp << " - "
            << stat.end_timestamp << ", duration: " << stat.duration
            << ", tx_packet_count: " << stat.tx_packet_count
            << ", total_tx_packet_count: " << stat.total_tx_packet_count << ".";

  if (bthal_tx_wakelock_recorder_.size() >= kMaxRecordHistory) {
    bthal_tx_wakelock_recorder_.pop_front();
  }
  bthal_tx_wakelock_recorder_.push_back(stat);
}

void BtActivitiesLogger::UpdateLeAdvStat(const ble_adv_activities_t& stat) {
  if (stat.duration > kBtContiBleAdvRecordPeriodMs) {
    if (ble_adv_activities_recorder_.size() >= kMaxRecordHistory) {
      ble_adv_activities_recorder_.pop_front();
    }
    ble_adv_activities_recorder_.push_back(stat);
  }
}

void BtActivitiesLogger::UpdateAclDataStat(const acl_data_activities_t& stat) {
  if (stat.duration > kBtContiAclDataRecordPeriodMs) {
    if (acl_data_activities_recorder_.size() >= kMaxRecordHistory) {
      acl_data_activities_recorder_.pop_front();
    }
    acl_data_activities_recorder_.push_back(stat);
  }
}

void BtActivitiesLogger::UpdateHciPacketStat(const pkt_activities_t& pkt_stat) {
  // HCI Packets.
  pkt_activity_ostream_ << pkt_stat.start_timestamp << ", "
                        << pkt_stat.end_timestamp << ", "
                        << std::to_string(pkt_stat.duration) << ", "
                        << std::to_string(pkt_stat.delta) << ", "
                        << std::to_string(pkt_stat.pkt_count) << ", "
                        << std::to_string(pkt_stat.total_pkt_count);
  // BLE ADV Packets.
  if (pkt_stat.ble_stat.size() > 0) {
    uint32_t sum_of_adv_duration = 0;
    uint32_t sum_of_adv_count = 0;
    std::vector<ble_adv_activities_t> ble_adv(pkt_stat.ble_stat);
    sum_of_ble_adv_data(ble_adv, &sum_of_adv_duration, &sum_of_adv_count);
    pkt_activity_ostream_
        << " , ble_adv:," << ble_adv[0].start_timestamp << ", "
        << std::to_string(sum_of_adv_duration) << ", "
        << std::to_string(sum_of_adv_count) << ", "
        << std::to_string(ble_adv[ble_adv.size() - 1].total_le_adv_count);
  } else {
    pkt_activity_ostream_ << ", ble_adv:" << ", --:--:--:--" << ", 0" << ", 0"
                          << ", 0";
  }
  // ACL_Data paxkets.
  if (pkt_stat.acl_data.size() > 0) {
    uint32_t sum_of_acl_duration = 0;
    uint32_t sum_of_acl_count = 0;
    uint32_t sum_of_acltx_count = 0;
    uint32_t sum_of_aclrx_count = 0;
    std::vector<acl_data_activities_t> acl_data(pkt_stat.acl_data);
    sum_of_acl_data(acl_data, &sum_of_acl_duration, &sum_of_acl_count,
                    &sum_of_acltx_count, &sum_of_aclrx_count);
    pkt_activity_ostream_
        << " , acl_data:," << acl_data[0].start_timestamp << ", "
        << std::to_string(sum_of_acl_duration) << ", "
        << acl_data[0].acl_conn_handle << ", "
        << std::to_string(sum_of_acl_count) << ", "
        << std::to_string(sum_of_acltx_count) << ", "
        << std::to_string(sum_of_aclrx_count) << ", "
        << std::to_string(acl_data[acl_data.size() - 1].total_acl_data_count)
        << ", "
        << std::to_string(acl_data[acl_data.size() - 1].total_acl_tx_data_count)
        << ", "
        << std::to_string(
               acl_data[acl_data.size() - 1].total_acl_rx_data_count);
  } else {
    pkt_activity_ostream_ << ", acl_data:" << ", --:--:--:--" << ", 0" << ", 0"
                          << ", 0" << ", 0" << ", 0" << ", 0" << ", 0" << ", 0";
  }
  // Command/Event packets
  pkt_activity_ostream_ << " , cmd/evt:,"
                        << std::to_string(pkt_stat.hci_command_count) << ", "
                        << std::to_string(pkt_stat.hci_event_count) << ", "
                        << std::to_string(pkt_stat.num_cmpl_pkt_evt_count);
  // End of this record
  pkt_activity_ostream_ << std::endl;

  if (!pkt_activity_ostream_.flush()) {
    LOG(ERROR) << __func__ << ": Failed to flush, error: \"" << strerror(errno)
               << "\".";
  }
}

void BtActivitiesLogger::DumpBtActivitiesStatistics(int fd) {
  std::stringstream ss;

  ss << "*********************************************" << std::endl;
  ss << "*   Begin Of Bluetooth Activities Reports   *" << std::endl;
  ss << "*********************************************" << std::endl;
  ss << "=============================================" << std::endl;
  ss << " 1. Connected devices Report :" << std::endl;
  ss << "=============================================" << std::endl;
  ss << "handle" << ", bt_address" << ", timestamp" << ", rcvd_event"
     << ", event_status" << std::endl;

  for (auto it = connection_history_.begin(); it != connection_history_.end();
       it++) {
    LOG(INFO) << __func__ << ": HCI_ACL: conn_handle: " << std::hex
              << std::setw(3) << std::setfill('0') << it->connect_handle
              << ", bda: " << it->bd_addr
              << ", created_timestamp: " << it->timestamp
              << ", rcvd_event: " << it->rcvd_event
              << ", event_status: " << it->status << ".";

    ss << it->connect_handle << ", " << it->bd_addr << ", " << it->timestamp
       << ", " << it->rcvd_event << ", " << it->status << std::endl;
  }

  ss << "=============================================" << std::endl;
  ss << " 2. BtHal Tx Wakelock Report :" << std::endl;
  ss << "=============================================" << std::endl;
  auto begin_wakelock = bthal_tx_wakelock_recorder_.begin();
  auto end_wakelock = bthal_tx_wakelock_recorder_.end();

  ss << "start_timestamp" << ", end_timestamp" << ", during"
     << ", tx_packet_count" << ", total tx_packet_count" << std::endl;
  for (auto it = begin_wakelock; it != end_wakelock; ++it) {
    ss << it->start_timestamp << ", " << it->end_timestamp << ", "
       << std::to_string(it->duration) << ", "
       << std::to_string(it->tx_packet_count) << ", "
       << std::to_string(it->total_tx_packet_count) << std::endl;
  }

  ss << "=============================================" << std::endl;
  ss << " 3. BtHal ACL Data Report :" << std::endl;
  ss << "=============================================" << std::endl;
  auto begin_acl = acl_data_activities_recorder_.begin();
  auto end_acl = acl_data_activities_recorder_.end();

  ss << "start_timestamp" << ", end_timestamp" << ", conn_handle" << ", during"
     << ", acl_data_count" << ", acl_tx_data_count" << ", acl_rx_data_count"
     << ", total_acl_data_count" << ", total_acl_tx_data_count"
     << ", total_acl_rx_data_count" << std::endl;
  for (auto it = begin_acl; it != end_acl; ++it) {
    ss << it->start_timestamp << ", " << it->end_timestamp << ", "
       << it->acl_conn_handle << ", " << std::to_string(it->duration) << ", "
       << std::to_string(it->acl_data_count) << ", "
       << std::to_string(it->acl_tx_data_count) << ", "
       << std::to_string(it->acl_rx_data_count) << ", "
       << std::to_string(it->total_acl_data_count) << ", "
       << std::to_string(it->total_acl_tx_data_count) << ", "
       << std::to_string(it->total_acl_rx_data_count) << std::endl;
  }

  ss << "=============================================" << std::endl;
  ss << " 4. BLE Advertising Report :" << std::endl;
  ss << "=============================================" << std::endl;
  auto begin_entry = ble_adv_activities_recorder_.begin();
  auto end_entry = ble_adv_activities_recorder_.end();

  ss << "start_timestamp" << ", end_timestamp" << ", duration"
     << ", le_adv_count" << ", total le_adv_count" << std::endl;
  for (auto it = begin_entry; it != end_entry; ++it) {
    ss << it->start_timestamp << ", " << it->end_timestamp << ", "
       << std::to_string(it->duration) << ", "
       << std::to_string(it->le_adv_count) << ", "
       << std::to_string(it->total_le_adv_count) << std::endl;
  }

  ss << "*********************************************" << std::endl;
  ss << "*    End Of Bluetooth Activities Reports    *" << std::endl;
  ss << "*********************************************" << std::endl;

  write(fd, ss.str().c_str(), ss.str().length());
}

bool BtActivitiesLogger::HasConnectedDevices() {
  return connected_bda_.size() > 0;
}

}  // namespace debug
}  // namespace bluetooth_hal
