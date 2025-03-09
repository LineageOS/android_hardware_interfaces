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
/* @hide */
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum NvItem {
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  INVALID = 0,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_MEID = 1,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_MIN = 2,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_MDN = 3,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_ACCOLC = 4,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  DEVICE_MSL = 11,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_RECONDITIONED_STATUS = 12,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_ACTIVATION_DATE = 13,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_LIFE_TIMER = 14,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_LIFE_CALLS = 15,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_LIFE_DATA_TX = 16,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  RTN_LIFE_DATA_RX = 17,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  OMADM_HFA_LEVEL = 18,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_NAI = 31,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_HOME_ADDRESS = 32,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_AAA_AUTH = 33,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_HA_AUTH = 34,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_PRI_HA_ADDR = 35,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_SEC_HA_ADDR = 36,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_REV_TUN_PREF = 37,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_HA_SPI = 38,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_AAA_SPI = 39,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_MN_HA_SS = 40,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  MIP_PROFILE_MN_AAA_SS = 41,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_PRL_VERSION = 51,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_BC10 = 52,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_BC14 = 53,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_SO68 = 54,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_SO73_COP0 = 55,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_SO73_COP1TO7 = 56,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_1X_ADVANCED_ENABLED = 57,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_EHRPD_ENABLED = 58,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  CDMA_EHRPD_FORCED = 59,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_BAND_ENABLE_25 = 71,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_BAND_ENABLE_26 = 72,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_BAND_ENABLE_41 = 73,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_SCAN_PRIORITY_25 = 74,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_SCAN_PRIORITY_26 = 75,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_SCAN_PRIORITY_41 = 76,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_HIDDEN_BAND_PRIORITY_25 = 77,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_HIDDEN_BAND_PRIORITY_26 = 78,
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  LTE_HIDDEN_BAND_PRIORITY_41 = 79,
}
