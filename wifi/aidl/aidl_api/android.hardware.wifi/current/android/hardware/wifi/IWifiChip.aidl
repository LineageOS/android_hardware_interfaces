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
interface IWifiChip {
  void configureChip(in int modeId);
  @PropagateAllowBlocking android.hardware.wifi.IWifiApIface createApIface();
  @PropagateAllowBlocking android.hardware.wifi.IWifiApIface createBridgedApIface();
  @PropagateAllowBlocking android.hardware.wifi.IWifiNanIface createNanIface();
  @PropagateAllowBlocking android.hardware.wifi.IWifiP2pIface createP2pIface();
  @PropagateAllowBlocking android.hardware.wifi.IWifiRttController createRttController(in android.hardware.wifi.IWifiStaIface boundIface);
  @PropagateAllowBlocking android.hardware.wifi.IWifiStaIface createStaIface();
  void enableDebugErrorAlerts(in boolean enable);
  void flushRingBufferToFile();
  void forceDumpToDebugRingBuffer(in String ringName);
  @PropagateAllowBlocking android.hardware.wifi.IWifiApIface getApIface(in String ifname);
  String[] getApIfaceNames();
  android.hardware.wifi.IWifiChip.ChipMode[] getAvailableModes();
  int getFeatureSet();
  android.hardware.wifi.WifiDebugHostWakeReasonStats getDebugHostWakeReasonStats();
  android.hardware.wifi.WifiDebugRingBufferStatus[] getDebugRingBuffersStatus();
  int getId();
  int getMode();
  @PropagateAllowBlocking android.hardware.wifi.IWifiNanIface getNanIface(in String ifname);
  String[] getNanIfaceNames();
  @PropagateAllowBlocking android.hardware.wifi.IWifiP2pIface getP2pIface(in String ifname);
  String[] getP2pIfaceNames();
  @PropagateAllowBlocking android.hardware.wifi.IWifiStaIface getStaIface(in String ifname);
  String[] getStaIfaceNames();
  android.hardware.wifi.WifiRadioCombination[] getSupportedRadioCombinations();
  android.hardware.wifi.WifiChipCapabilities getWifiChipCapabilities();
  android.hardware.wifi.WifiUsableChannel[] getUsableChannels(in android.hardware.wifi.WifiBand band, in int ifaceModeMask, in int filterMask);
  void setAfcChannelAllowance(in android.hardware.wifi.AfcChannelAllowance afcChannelAllowance);
  void registerEventCallback(in android.hardware.wifi.IWifiChipEventCallback callback);
  void removeApIface(in String ifname);
  void removeIfaceInstanceFromBridgedApIface(in String brIfaceName, in String ifaceInstanceName);
  void removeNanIface(in String ifname);
  void removeP2pIface(in String ifname);
  void removeStaIface(in String ifname);
  android.hardware.wifi.IWifiChip.ChipDebugInfo requestChipDebugInfo();
  byte[] requestDriverDebugDump();
  byte[] requestFirmwareDebugDump();
  void resetTxPowerScenario();
  void selectTxPowerScenario(in android.hardware.wifi.IWifiChip.TxPowerScenario scenario);
  void setCoexUnsafeChannels(in android.hardware.wifi.IWifiChip.CoexUnsafeChannel[] unsafeChannels, in int restrictions);
  void setCountryCode(in byte[2] code);
  void setLatencyMode(in android.hardware.wifi.IWifiChip.LatencyMode mode);
  void setMultiStaPrimaryConnection(in String ifName);
  void setMultiStaUseCase(in android.hardware.wifi.IWifiChip.MultiStaUseCase useCase);
  void startLoggingToDebugRingBuffer(in String ringName, in android.hardware.wifi.WifiDebugRingBufferVerboseLevel verboseLevel, in int maxIntervalInSec, in int minDataSizeInBytes);
  void stopLoggingToDebugRingBuffer();
  void triggerSubsystemRestart();
  void enableStaChannelForPeerNetwork(in int channelCategoryEnableFlag);
  void setMloMode(in android.hardware.wifi.IWifiChip.ChipMloMode mode);
  @PropagateAllowBlocking android.hardware.wifi.IWifiApIface createApOrBridgedApIface(in android.hardware.wifi.IfaceConcurrencyType iface, in android.hardware.wifi.common.OuiKeyedData[] vendorData);
  void setVoipMode(in android.hardware.wifi.IWifiChip.VoipMode mode);
  const int NO_POWER_CAP_CONSTANT = 0x7FFFFFFF;
  @Backing(type="int") @VintfStability
  enum FeatureSetMask {
    SET_TX_POWER_LIMIT = (1 << 0) /* 1 */,
    D2D_RTT = (1 << 1) /* 2 */,
    D2AP_RTT = (1 << 2) /* 4 */,
    USE_BODY_HEAD_SAR = (1 << 3) /* 8 */,
    SET_LATENCY_MODE = (1 << 4) /* 16 */,
    P2P_RAND_MAC = (1 << 5) /* 32 */,
    WIGIG = (1 << 6) /* 64 */,
    SET_AFC_CHANNEL_ALLOWANCE = (1 << 7) /* 128 */,
    T2LM_NEGOTIATION = (1 << 8) /* 256 */,
    SET_VOIP_MODE = (1 << 9) /* 512 */,
  }
  @VintfStability
  parcelable ChipConcurrencyCombinationLimit {
    android.hardware.wifi.IfaceConcurrencyType[] types;
    int maxIfaces;
  }
  @VintfStability
  parcelable ChipConcurrencyCombination {
    android.hardware.wifi.IWifiChip.ChipConcurrencyCombinationLimit[] limits;
  }
  @VintfStability
  parcelable ChipDebugInfo {
    String driverDescription;
    String firmwareDescription;
  }
  @VintfStability
  parcelable ChipIfaceCombinationLimit {
    android.hardware.wifi.IfaceType[] types;
    int maxIfaces;
  }
  @VintfStability
  parcelable ChipIfaceCombination {
    android.hardware.wifi.IWifiChip.ChipIfaceCombinationLimit[] limits;
  }
  @VintfStability
  parcelable ChipMode {
    int id;
    android.hardware.wifi.IWifiChip.ChipConcurrencyCombination[] availableCombinations;
  }
  @Backing(type="int") @VintfStability
  enum CoexRestriction {
    WIFI_DIRECT = (1 << 0) /* 1 */,
    SOFTAP = (1 << 1) /* 2 */,
    WIFI_AWARE = (1 << 2) /* 4 */,
  }
  @VintfStability
  parcelable CoexUnsafeChannel {
    android.hardware.wifi.WifiBand band;
    int channel;
    int powerCapDbm;
  }
  @Backing(type="int") @VintfStability
  enum LatencyMode {
    NORMAL = 0,
    LOW = 1,
  }
  @Backing(type="byte") @VintfStability
  enum MultiStaUseCase {
    DUAL_STA_TRANSIENT_PREFER_PRIMARY = 0,
    DUAL_STA_NON_TRANSIENT_UNBIASED = 1,
  }
  @Backing(type="int") @VintfStability
  enum TxPowerScenario {
    VOICE_CALL = 0,
    ON_HEAD_CELL_OFF = 1,
    ON_HEAD_CELL_ON = 2,
    ON_BODY_CELL_OFF = 3,
    ON_BODY_CELL_ON = 4,
  }
  @Backing(type="int") @VintfStability
  enum UsableChannelFilter {
    CELLULAR_COEXISTENCE = (1 << 0) /* 1 */,
    CONCURRENCY = (1 << 1) /* 2 */,
    NAN_INSTANT_MODE = (1 << 2) /* 4 */,
  }
  @Backing(type="int") @VintfStability
  enum VoipMode {
    OFF = 0,
    VOICE = 1,
  }
  @Backing(type="int") @VintfStability
  enum ChannelCategoryMask {
    INDOOR_CHANNEL = (1 << 0) /* 1 */,
    DFS_CHANNEL = (1 << 1) /* 2 */,
  }
  @Backing(type="int") @VintfStability
  enum ChipMloMode {
    DEFAULT = 0,
    LOW_LATENCY = 1,
    HIGH_THROUGHPUT = 2,
    LOW_POWER = 3,
  }
}
