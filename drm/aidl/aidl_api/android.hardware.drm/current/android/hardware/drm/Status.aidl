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

package android.hardware.drm;
@Backing(type="int") @VintfStability
enum Status {
  OK = 0,
  ERROR_DRM_NO_LICENSE = 1,
  ERROR_DRM_LICENSE_EXPIRED = 2,
  ERROR_DRM_SESSION_NOT_OPENED = 3,
  ERROR_DRM_CANNOT_HANDLE = 4,
  ERROR_DRM_INVALID_STATE = 5,
  BAD_VALUE = 6,
  ERROR_DRM_NOT_PROVISIONED = 7,
  ERROR_DRM_RESOURCE_BUSY = 8,
  ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION = 9,
  ERROR_DRM_DEVICE_REVOKED = 10,
  ERROR_DRM_DECRYPT = 11,
  ERROR_DRM_UNKNOWN = 12,
  ERROR_DRM_INSUFFICIENT_SECURITY = 13,
  ERROR_DRM_FRAME_TOO_LARGE = 14,
  ERROR_DRM_SESSION_LOST_STATE = 15,
  ERROR_DRM_RESOURCE_CONTENTION = 16,
  CANNOT_DECRYPT_ZERO_SUBSAMPLES = 17,
  CRYPTO_LIBRARY_ERROR = 18,
  GENERAL_OEM_ERROR = 19,
  GENERAL_PLUGIN_ERROR = 20,
  INIT_DATA_INVALID = 21,
  KEY_NOT_LOADED = 22,
  LICENSE_PARSE_ERROR = 23,
  LICENSE_POLICY_ERROR = 24,
  LICENSE_RELEASE_ERROR = 25,
  LICENSE_REQUEST_REJECTED = 26,
  LICENSE_RESTORE_ERROR = 27,
  LICENSE_STATE_ERROR = 28,
  MALFORMED_CERTIFICATE = 29,
  MEDIA_FRAMEWORK_ERROR = 30,
  MISSING_CERTIFICATE = 31,
  PROVISIONING_CERTIFICATE_ERROR = 32,
  PROVISIONING_CONFIGURATION_ERROR = 33,
  PROVISIONING_PARSE_ERROR = 34,
  PROVISIONING_REQUEST_REJECTED = 35,
  RETRYABLE_PROVISIONING_ERROR = 36,
  SECURE_STOP_RELEASE_ERROR = 37,
  STORAGE_READ_FAILURE = 38,
  STORAGE_WRITE_FAILURE = 39,
}
