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

#pragma once

#include <fstream>
#include <list>
#include <map>

#include "bluetooth_hal/hal_packet.h"

typedef struct {
  uint16_t connect_handle; /* connection handle */
  std::string bd_addr;     /* Remote BD address */
  std::string timestamp;   /* Connection Created Timestamp */
  std::string status;
  std::string rcvd_event; /* connect or disconnect */
} tCONN_DEVICE;

struct bthal_wakelock_stat_t {
  std::string start_timestamp;
  std::string end_timestamp;
  uint32_t duration;
  uint32_t tx_packet_count;
  uint64_t total_tx_packet_count;
};

struct ble_adv_activities_t {
  std::string start_timestamp;
  std::string end_timestamp;
  uint32_t duration;
  uint32_t le_adv_count;
  uint64_t total_le_adv_count;
};

struct acl_data_activities_t {
  std::string start_timestamp;
  std::string end_timestamp;
  uint16_t acl_conn_handle;
  uint32_t duration;
  uint32_t acl_data_count;
  uint32_t acl_tx_data_count;
  uint32_t total_acl_tx_data_count;
  uint32_t acl_rx_data_count;
  uint32_t total_acl_rx_data_count;
  uint64_t total_acl_data_count;
};

struct pkt_activities_t {
  std::string start_timestamp;
  std::string end_timestamp;
  uint32_t duration;
  uint32_t pkt_count;
  uint64_t total_pkt_count;
  std::vector<acl_data_activities_t> acl_data;
  std::vector<ble_adv_activities_t> ble_stat;
  uint32_t hci_command_count;
  uint32_t hci_event_count;
  uint32_t num_cmpl_pkt_evt_count;
  uint32_t delta;
};

// Bluetooth Activity Packet Types
enum class BtActivityPacketType : uint8_t {
  COMMAND = 1,
  EVENT = 2,
  ACL_TX_DATA = 3,
  ACL_RX_DATA = 4,
};

namespace bluetooth_hal {
namespace debug {

class BtActivitiesLogger {
 public:
  void OnBluetoothEnabled();
  void OnBluetoothDisabled();
  void StartLogging();
  void StopLogging();
  void ForceUpdating();
  bool HasConnectedDevices();
  void LogActivities(BtActivityPacketType type,
                     const ::bluetooth_hal::hci::HalPacket& data);
  void DumpBtActivitiesStatistics(int fd);
  void UpdateBthalWakelockStat(const bthal_wakelock_stat_t& stat);
  void UpdateAclDataStat(const acl_data_activities_t& stat);
  void UpdateLeAdvStat(const ble_adv_activities_t& stat);
  static void UpdateHciPacketStat(const pkt_activities_t& stat);
  static BtActivitiesLogger* GetInstacne();

 private:
  int kMaxRecordHistory = 512;
  std::list<ble_adv_activities_t> ble_adv_activities_recorder_;
  std::list<acl_data_activities_t> acl_data_activities_recorder_;
  std::list<bthal_wakelock_stat_t> bthal_tx_wakelock_recorder_;
  std::list<tCONN_DEVICE> connection_history_;
  std::map<uint16_t, std::string> connected_bda_;
  std::string bt_activities_pkt_log_path_;
  static std::ofstream pkt_activity_ostream_;

  static void LeAdvTimeout();
  static void AclDataTimeout();
  static void HciPacketTimeout();
  void handle_ble_meta_event(const ::bluetooth_hal::hci::HalPacket& data);
  void handle_connect_complete_event(
      const ::bluetooth_hal::hci::HalPacket& data);
  void handle_disconnect_complete_event(
      const ::bluetooth_hal::hci::HalPacket& data);
  void update_connect_disconnect_history(const tCONN_DEVICE& device);
  void open_new_hci_packet_log_file();
};

}  // namespace debug
}  // namespace bluetooth_hal
