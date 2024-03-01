/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.modem;
/**
 * @hide
 * @deprecated NV APIs are deprecated starting from Android U.
 */
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum NvItem {
  CDMA_MEID = 1,
  CDMA_MIN = 2,
  CDMA_MDN = 3,
  CDMA_ACCOLC = 4,
  DEVICE_MSL = 11,
  RTN_RECONDITIONED_STATUS = 12,
  RTN_ACTIVATION_DATE = 13,
  RTN_LIFE_TIMER = 14,
  RTN_LIFE_CALLS = 15,
  RTN_LIFE_DATA_TX = 16,
  RTN_LIFE_DATA_RX = 17,
  OMADM_HFA_LEVEL = 18,
  MIP_PROFILE_NAI = 31,
  MIP_PROFILE_HOME_ADDRESS = 32,
  MIP_PROFILE_AAA_AUTH = 33,
  MIP_PROFILE_HA_AUTH = 34,
  MIP_PROFILE_PRI_HA_ADDR = 35,
  MIP_PROFILE_SEC_HA_ADDR = 36,
  MIP_PROFILE_REV_TUN_PREF = 37,
  MIP_PROFILE_HA_SPI = 38,
  MIP_PROFILE_AAA_SPI = 39,
  MIP_PROFILE_MN_HA_SS = 40,
  MIP_PROFILE_MN_AAA_SS = 41,
  CDMA_PRL_VERSION = 51,
  CDMA_BC10 = 52,
  CDMA_BC14 = 53,
  CDMA_SO68 = 54,
  CDMA_SO73_COP0 = 55,
  CDMA_SO73_COP1TO7 = 56,
  CDMA_1X_ADVANCED_ENABLED = 57,
  CDMA_EHRPD_ENABLED = 58,
  CDMA_EHRPD_FORCED = 59,
  LTE_BAND_ENABLE_25 = 71,
  LTE_BAND_ENABLE_26 = 72,
  LTE_BAND_ENABLE_41 = 73,
  LTE_SCAN_PRIORITY_25 = 74,
  LTE_SCAN_PRIORITY_26 = 75,
  LTE_SCAN_PRIORITY_41 = 76,
  LTE_HIDDEN_BAND_PRIORITY_25 = 77,
  LTE_HIDDEN_BAND_PRIORITY_26 = 78,
  LTE_HIDDEN_BAND_PRIORITY_41 = 79,
}
