/*
 * Copyright (C) 2022 The Android Open Source Project
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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.wifi;
@VintfStability
interface IWifiStaIface {
  String getName();
  void configureRoaming(in android.hardware.wifi.StaRoamingConfig config);
  void disableLinkLayerStatsCollection();
  void enableLinkLayerStatsCollection(in boolean debug);
  void enableNdOffload(in boolean enable);
  android.hardware.wifi.StaApfPacketFilterCapabilities getApfPacketFilterCapabilities();
  android.hardware.wifi.StaBackgroundScanCapabilities getBackgroundScanCapabilities();
  int getFeatureSet();
  android.hardware.wifi.WifiDebugRxPacketFateReport[] getDebugRxPacketFates();
  android.hardware.wifi.WifiDebugTxPacketFateReport[] getDebugTxPacketFates();
  byte[6] getFactoryMacAddress();
  android.hardware.wifi.StaLinkLayerStats getLinkLayerStats();
  android.hardware.wifi.StaRoamingCapabilities getRoamingCapabilities();
  void installApfPacketFilter(in byte[] program);
  byte[] readApfPacketFilterData();
  void registerEventCallback(in android.hardware.wifi.IWifiStaIfaceEventCallback callback);
  void setMacAddress(in byte[6] mac);
  void setRoamingState(in android.hardware.wifi.StaRoamingState state);
  void setScanMode(in boolean enable);
  void startBackgroundScan(in int cmdId, in android.hardware.wifi.StaBackgroundScanParameters params);
  void startDebugPacketFateMonitoring();
  void startRssiMonitoring(in int cmdId, in int maxRssi, in int minRssi);
  void startSendingKeepAlivePackets(in int cmdId, in byte[] ipPacketData, in char etherType, in byte[6] srcAddress, in byte[6] dstAddress, in int periodInMs);
  void stopBackgroundScan(in int cmdId);
  void stopRssiMonitoring(in int cmdId);
  void stopSendingKeepAlivePackets(in int cmdId);
  void setDtimMultiplier(in int multiplier);
  android.hardware.wifi.CachedScanData getCachedScanData();
  android.hardware.wifi.TwtCapabilities twtGetCapabilities();
  void twtSessionSetup(in int cmdId, in android.hardware.wifi.TwtRequest twtRequest);
  void twtSessionUpdate(in int cmdId, in int sessionId, in android.hardware.wifi.TwtRequest twtRequest);
  void twtSessionSuspend(in int cmdId, in int sessionId);
  void twtSessionResume(in int cmdId, in int sessionId);
  void twtSessionTeardown(in int cmdId, in int sessionId);
  void twtSessionGetStats(in int cmdId, in int sessionId);
  @Backing(type="int") @VintfStability
  enum FeatureSetMask {
    APF = (1 << 0) /* 1 */,
    BACKGROUND_SCAN = (1 << 1) /* 2 */,
    LINK_LAYER_STATS = (1 << 2) /* 4 */,
    RSSI_MONITOR = (1 << 3) /* 8 */,
    CONTROL_ROAMING = (1 << 4) /* 16 */,
    PROBE_IE_ALLOWLIST = (1 << 5) /* 32 */,
    SCAN_RAND = (1 << 6) /* 64 */,
    STA_5G = (1 << 7) /* 128 */,
    HOTSPOT = (1 << 8) /* 256 */,
    PNO = (1 << 9) /* 512 */,
    TDLS = (1 << 10) /* 1024 */,
    TDLS_OFFCHANNEL = (1 << 11) /* 2048 */,
    ND_OFFLOAD = (1 << 12) /* 4096 */,
    KEEP_ALIVE = (1 << 13) /* 8192 */,
    ROAMING_MODE_CONTROL = (1 << 14) /* 16384 */,
    CACHED_SCAN_DATA = (1 << 15) /* 32768 */,
  }
}
