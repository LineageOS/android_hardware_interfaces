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

package android.hardware.gnss;
/* @hide */
@VintfStability
interface IGnssNavigationMessageCallback {
  void gnssNavigationMessageCb(in android.hardware.gnss.IGnssNavigationMessageCallback.GnssNavigationMessage message);
  @VintfStability
  parcelable GnssNavigationMessage {
    int svid;
    android.hardware.gnss.IGnssNavigationMessageCallback.GnssNavigationMessage.GnssNavigationMessageType type;
    int status;
    int messageId;
    int submessageId;
    byte[] data;
    const int STATUS_PARITY_PASSED = (1 << 0) /* 1 */;
    const int STATUS_PARITY_REBUILT = (1 << 1) /* 2 */;
    const int STATUS_UNKNOWN = 0;
    @Backing(type="int") @VintfStability
    enum GnssNavigationMessageType {
      UNKNOWN = 0,
      GPS_L1CA = 0x0101,
      GPS_L2CNAV = 0x0102,
      GPS_L5CNAV = 0x0103,
      SBS = 0x0201,
      GPS_CNAV2 = 0x0104,
      GLO_L1CA = 0x0301,
      QZS_L1CA = 0x0401,
      BDS_D1 = 0x0501,
      BDS_D2 = 0x0502,
      BDS_CNAV1 = 0x0503,
      BDS_CNAV2 = 0x0504,
      GAL_I = 0x0601,
      GAL_F = 0x0602,
      /**
       * @deprecated Use IRN_L5 instead.
       */
      IRN_L5CA = 0x0701,
      IRN_L5 = 0x0702,
      IRN_L1 = 0x0703,
    }
  }
}
