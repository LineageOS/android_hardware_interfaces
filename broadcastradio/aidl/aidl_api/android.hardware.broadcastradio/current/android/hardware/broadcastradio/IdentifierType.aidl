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

package android.hardware.broadcastradio;
@Backing(type="int") @JavaDerive(equals=true, toString=true) @VintfStability
enum IdentifierType {
  VENDOR_START = 1000,
  VENDOR_END = 1999,
  INVALID = 0,
  AMFM_FREQUENCY_KHZ,
  RDS_PI,
  HD_STATION_ID_EXT,
  HD_STATION_NAME,
  DAB_SID_EXT,
  DAB_ENSEMBLE,
  DAB_SCID,
  DAB_FREQUENCY_KHZ,
  DRMO_SERVICE_ID,
  DRMO_FREQUENCY_KHZ,
  /**
   * @deprecated SiriusXM Satellite Radio is not supported.
   */
  SXM_SERVICE_ID = (DRMO_FREQUENCY_KHZ + 2) /* 12 */,
  /**
   * @deprecated SiriusXM Satellite Radio is not supported.
   */
  SXM_CHANNEL,
  HD_STATION_LOCATION,
}
