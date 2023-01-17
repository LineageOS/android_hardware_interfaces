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

package android.hardware.radio;
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum RadioAccessFamily {
  UNKNOWN = (1 << android.hardware.radio.RadioTechnology.UNKNOWN),
  GPRS = (1 << android.hardware.radio.RadioTechnology.GPRS),
  EDGE = (1 << android.hardware.radio.RadioTechnology.EDGE),
  UMTS = (1 << android.hardware.radio.RadioTechnology.UMTS),
  IS95A = (1 << android.hardware.radio.RadioTechnology.IS95A),
  IS95B = (1 << android.hardware.radio.RadioTechnology.IS95B),
  ONE_X_RTT = (1 << android.hardware.radio.RadioTechnology.ONE_X_RTT),
  EVDO_0 = (1 << android.hardware.radio.RadioTechnology.EVDO_0),
  EVDO_A = (1 << android.hardware.radio.RadioTechnology.EVDO_A),
  HSDPA = (1 << android.hardware.radio.RadioTechnology.HSDPA),
  HSUPA = (1 << android.hardware.radio.RadioTechnology.HSUPA),
  HSPA = (1 << android.hardware.radio.RadioTechnology.HSPA),
  EVDO_B = (1 << android.hardware.radio.RadioTechnology.EVDO_B),
  EHRPD = (1 << android.hardware.radio.RadioTechnology.EHRPD),
  LTE = (1 << android.hardware.radio.RadioTechnology.LTE),
  HSPAP = (1 << android.hardware.radio.RadioTechnology.HSPAP),
  GSM = (1 << android.hardware.radio.RadioTechnology.GSM),
  TD_SCDMA = (1 << android.hardware.radio.RadioTechnology.TD_SCDMA),
  IWLAN = (1 << android.hardware.radio.RadioTechnology.IWLAN),
  /**
   * @deprecated use LTE instead.
   */
  LTE_CA = (1 << android.hardware.radio.RadioTechnology.LTE_CA),
  NR = (1 << android.hardware.radio.RadioTechnology.NR),
}
