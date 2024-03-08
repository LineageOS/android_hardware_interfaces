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

package android.hardware.gnss;
/* @hide */
@VintfStability
interface IAGnssRil {
  void setCallback(in android.hardware.gnss.IAGnssRilCallback callback);
  void setRefLocation(in android.hardware.gnss.IAGnssRil.AGnssRefLocation agnssReflocation);
  void setSetId(in android.hardware.gnss.IAGnssRil.SetIdType type, in @utf8InCpp String setid);
  void updateNetworkState(in android.hardware.gnss.IAGnssRil.NetworkAttributes attributes);
  void injectNiSuplMessageData(in byte[] msgData, in int slotIndex);
  const int NETWORK_CAPABILITY_NOT_METERED = 0x01;
  const int NETWORK_CAPABILITY_NOT_ROAMING = 0x02;
  @Backing(type="int") @VintfStability
  enum AGnssRefLocationType {
    GSM_CELLID = 1,
    UMTS_CELLID = 2,
    LTE_CELLID = 4,
    NR_CELLID = 8,
  }
  @Backing(type="int") @VintfStability
  enum SetIdType {
    NONE = 0,
    IMSI = 1,
    MSISDM = 2,
  }
  @VintfStability
  parcelable AGnssRefLocationCellID {
    android.hardware.gnss.IAGnssRil.AGnssRefLocationType type;
    int mcc;
    int mnc;
    int lac;
    long cid;
    int tac;
    int pcid;
    int arfcn;
  }
  @VintfStability
  parcelable AGnssRefLocation {
    android.hardware.gnss.IAGnssRil.AGnssRefLocationType type;
    android.hardware.gnss.IAGnssRil.AGnssRefLocationCellID cellID;
  }
  @VintfStability
  parcelable NetworkAttributes {
    long networkHandle;
    boolean isConnected;
    int capabilities;
    @utf8InCpp String apn;
  }
}
