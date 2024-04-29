/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.biometrics.fingerprint;
/* @hide */
@VintfStability
interface IVirtualHal {
  oneway void setEnrollments(in int[] id);
  oneway void setEnrollmentHit(in int hit_id);
  oneway void setNextEnrollment(in android.hardware.biometrics.fingerprint.NextEnrollment next_enrollment);
  oneway void setAuthenticatorId(in long id);
  oneway void setChallenge(in long challenge);
  oneway void setOperationAuthenticateFails(in boolean fail);
  oneway void setOperationAuthenticateLatency(in int[] latencyMs);
  oneway void setOperationAuthenticateDuration(in int durationMs);
  oneway void setOperationAuthenticateError(in int error);
  oneway void setOperationAuthenticateAcquired(in android.hardware.biometrics.fingerprint.AcquiredInfoAndVendorCode[] acquired);
  oneway void setOperationEnrollError(in int error);
  oneway void setOperationEnrollLatency(in int[] latencyMs);
  oneway void setOperationDetectInteractionLatency(in int[] latencyMs);
  oneway void setOperationDetectInteractionError(in int error);
  oneway void setOperationDetectInteractionDuration(in int durationMs);
  oneway void setOperationDetectInteractionAcquired(in android.hardware.biometrics.fingerprint.AcquiredInfoAndVendorCode[] acquired);
  oneway void setLockout(in boolean lockout);
  oneway void setLockoutEnable(in boolean enable);
  oneway void setLockoutTimedThreshold(in int threshold);
  oneway void setLockoutTimedDuration(in int durationMs);
  oneway void setLockoutPermanentThreshold(in int threshold);
  oneway void resetConfigurations();
  oneway void setType(in android.hardware.biometrics.fingerprint.FingerprintSensorType type);
  oneway void setSensorId(in int id);
  oneway void setSensorStrength(in android.hardware.biometrics.common.SensorStrength strength);
  oneway void setMaxEnrollmentPerUser(in int max);
  oneway void setSensorLocation(in android.hardware.biometrics.fingerprint.SensorLocation loc);
  oneway void setNavigationGuesture(in boolean v);
  oneway void setDetectInteraction(in boolean v);
  oneway void setDisplayTouch(in boolean v);
  oneway void setControlIllumination(in boolean v);
  const int STATUS_INVALID_PARAMETER = 1;
}
