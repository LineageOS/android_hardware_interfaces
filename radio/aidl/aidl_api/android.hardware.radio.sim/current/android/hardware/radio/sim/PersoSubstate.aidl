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

package android.hardware.radio.sim;
/* @hide */
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum PersoSubstate {
  UNKNOWN,
  IN_PROGRESS,
  READY,
  SIM_NETWORK,
  SIM_NETWORK_SUBSET,
  SIM_CORPORATE,
  SIM_SERVICE_PROVIDER,
  SIM_SIM,
  SIM_NETWORK_PUK,
  SIM_NETWORK_SUBSET_PUK,
  SIM_CORPORATE_PUK,
  SIM_SERVICE_PROVIDER_PUK,
  SIM_SIM_PUK,
  RUIM_NETWORK1,
  RUIM_NETWORK2,
  RUIM_HRPD,
  RUIM_CORPORATE,
  RUIM_SERVICE_PROVIDER,
  RUIM_RUIM,
  RUIM_NETWORK1_PUK,
  RUIM_NETWORK2_PUK,
  RUIM_HRPD_PUK,
  RUIM_CORPORATE_PUK,
  RUIM_SERVICE_PROVIDER_PUK,
  RUIM_RUIM_PUK,
  SIM_SPN,
  SIM_SPN_PUK,
  SIM_SP_EHPLMN,
  SIM_SP_EHPLMN_PUK,
  SIM_ICCID,
  SIM_ICCID_PUK,
  SIM_IMPI,
  SIM_IMPI_PUK,
  SIM_NS_SP,
  SIM_NS_SP_PUK,
}
