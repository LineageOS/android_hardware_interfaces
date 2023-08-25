/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.security.keymint;
/* @hide */
@Backing(type="int") @VintfStability
enum Tag {
  INVALID = 0,
  PURPOSE = (android.hardware.security.keymint.TagType.ENUM_REP | 1) /* 536870913 */,
  ALGORITHM = (android.hardware.security.keymint.TagType.ENUM | 2) /* 268435458 */,
  KEY_SIZE = (android.hardware.security.keymint.TagType.UINT | 3) /* 805306371 */,
  BLOCK_MODE = (android.hardware.security.keymint.TagType.ENUM_REP | 4) /* 536870916 */,
  DIGEST = (android.hardware.security.keymint.TagType.ENUM_REP | 5) /* 536870917 */,
  PADDING = (android.hardware.security.keymint.TagType.ENUM_REP | 6) /* 536870918 */,
  CALLER_NONCE = (android.hardware.security.keymint.TagType.BOOL | 7) /* 1879048199 */,
  MIN_MAC_LENGTH = (android.hardware.security.keymint.TagType.UINT | 8) /* 805306376 */,
  EC_CURVE = (android.hardware.security.keymint.TagType.ENUM | 10) /* 268435466 */,
  RSA_PUBLIC_EXPONENT = (android.hardware.security.keymint.TagType.ULONG | 200) /* 1342177480 */,
  INCLUDE_UNIQUE_ID = (android.hardware.security.keymint.TagType.BOOL | 202) /* 1879048394 */,
  RSA_OAEP_MGF_DIGEST = (android.hardware.security.keymint.TagType.ENUM_REP | 203) /* 536871115 */,
  BOOTLOADER_ONLY = (android.hardware.security.keymint.TagType.BOOL | 302) /* 1879048494 */,
  ROLLBACK_RESISTANCE = (android.hardware.security.keymint.TagType.BOOL | 303) /* 1879048495 */,
  HARDWARE_TYPE = (android.hardware.security.keymint.TagType.ENUM | 304) /* 268435760 */,
  EARLY_BOOT_ONLY = (android.hardware.security.keymint.TagType.BOOL | 305) /* 1879048497 */,
  ACTIVE_DATETIME = (android.hardware.security.keymint.TagType.DATE | 400) /* 1610613136 */,
  ORIGINATION_EXPIRE_DATETIME = (android.hardware.security.keymint.TagType.DATE | 401) /* 1610613137 */,
  USAGE_EXPIRE_DATETIME = (android.hardware.security.keymint.TagType.DATE | 402) /* 1610613138 */,
  MIN_SECONDS_BETWEEN_OPS = (android.hardware.security.keymint.TagType.UINT | 403) /* 805306771 */,
  MAX_USES_PER_BOOT = (android.hardware.security.keymint.TagType.UINT | 404) /* 805306772 */,
  USAGE_COUNT_LIMIT = (android.hardware.security.keymint.TagType.UINT | 405) /* 805306773 */,
  USER_ID = (android.hardware.security.keymint.TagType.UINT | 501) /* 805306869 */,
  USER_SECURE_ID = (android.hardware.security.keymint.TagType.ULONG_REP | 502) /* -1610612234 */,
  NO_AUTH_REQUIRED = (android.hardware.security.keymint.TagType.BOOL | 503) /* 1879048695 */,
  USER_AUTH_TYPE = (android.hardware.security.keymint.TagType.ENUM | 504) /* 268435960 */,
  AUTH_TIMEOUT = (android.hardware.security.keymint.TagType.UINT | 505) /* 805306873 */,
  ALLOW_WHILE_ON_BODY = (android.hardware.security.keymint.TagType.BOOL | 506) /* 1879048698 */,
  TRUSTED_USER_PRESENCE_REQUIRED = (android.hardware.security.keymint.TagType.BOOL | 507) /* 1879048699 */,
  TRUSTED_CONFIRMATION_REQUIRED = (android.hardware.security.keymint.TagType.BOOL | 508) /* 1879048700 */,
  UNLOCKED_DEVICE_REQUIRED = (android.hardware.security.keymint.TagType.BOOL | 509) /* 1879048701 */,
  APPLICATION_ID = (android.hardware.security.keymint.TagType.BYTES | 601) /* -1879047591 */,
  APPLICATION_DATA = (android.hardware.security.keymint.TagType.BYTES | 700) /* -1879047492 */,
  CREATION_DATETIME = (android.hardware.security.keymint.TagType.DATE | 701) /* 1610613437 */,
  ORIGIN = (android.hardware.security.keymint.TagType.ENUM | 702) /* 268436158 */,
  ROOT_OF_TRUST = (android.hardware.security.keymint.TagType.BYTES | 704) /* -1879047488 */,
  OS_VERSION = (android.hardware.security.keymint.TagType.UINT | 705) /* 805307073 */,
  OS_PATCHLEVEL = (android.hardware.security.keymint.TagType.UINT | 706) /* 805307074 */,
  UNIQUE_ID = (android.hardware.security.keymint.TagType.BYTES | 707) /* -1879047485 */,
  ATTESTATION_CHALLENGE = (android.hardware.security.keymint.TagType.BYTES | 708) /* -1879047484 */,
  ATTESTATION_APPLICATION_ID = (android.hardware.security.keymint.TagType.BYTES | 709) /* -1879047483 */,
  ATTESTATION_ID_BRAND = (android.hardware.security.keymint.TagType.BYTES | 710) /* -1879047482 */,
  ATTESTATION_ID_DEVICE = (android.hardware.security.keymint.TagType.BYTES | 711) /* -1879047481 */,
  ATTESTATION_ID_PRODUCT = (android.hardware.security.keymint.TagType.BYTES | 712) /* -1879047480 */,
  ATTESTATION_ID_SERIAL = (android.hardware.security.keymint.TagType.BYTES | 713) /* -1879047479 */,
  ATTESTATION_ID_IMEI = (android.hardware.security.keymint.TagType.BYTES | 714) /* -1879047478 */,
  ATTESTATION_ID_MEID = (android.hardware.security.keymint.TagType.BYTES | 715) /* -1879047477 */,
  ATTESTATION_ID_MANUFACTURER = (android.hardware.security.keymint.TagType.BYTES | 716) /* -1879047476 */,
  ATTESTATION_ID_MODEL = (android.hardware.security.keymint.TagType.BYTES | 717) /* -1879047475 */,
  VENDOR_PATCHLEVEL = (android.hardware.security.keymint.TagType.UINT | 718) /* 805307086 */,
  BOOT_PATCHLEVEL = (android.hardware.security.keymint.TagType.UINT | 719) /* 805307087 */,
  DEVICE_UNIQUE_ATTESTATION = (android.hardware.security.keymint.TagType.BOOL | 720) /* 1879048912 */,
  IDENTITY_CREDENTIAL_KEY = (android.hardware.security.keymint.TagType.BOOL | 721) /* 1879048913 */,
  STORAGE_KEY = (android.hardware.security.keymint.TagType.BOOL | 722) /* 1879048914 */,
  ATTESTATION_ID_SECOND_IMEI = (android.hardware.security.keymint.TagType.BYTES | 723) /* -1879047469 */,
  ASSOCIATED_DATA = (android.hardware.security.keymint.TagType.BYTES | 1000) /* -1879047192 */,
  NONCE = (android.hardware.security.keymint.TagType.BYTES | 1001) /* -1879047191 */,
  MAC_LENGTH = (android.hardware.security.keymint.TagType.UINT | 1003) /* 805307371 */,
  RESET_SINCE_ID_ROTATION = (android.hardware.security.keymint.TagType.BOOL | 1004) /* 1879049196 */,
  CONFIRMATION_TOKEN = (android.hardware.security.keymint.TagType.BYTES | 1005) /* -1879047187 */,
  CERTIFICATE_SERIAL = (android.hardware.security.keymint.TagType.BIGNUM | 1006) /* -2147482642 */,
  CERTIFICATE_SUBJECT = (android.hardware.security.keymint.TagType.BYTES | 1007) /* -1879047185 */,
  CERTIFICATE_NOT_BEFORE = (android.hardware.security.keymint.TagType.DATE | 1008) /* 1610613744 */,
  CERTIFICATE_NOT_AFTER = (android.hardware.security.keymint.TagType.DATE | 1009) /* 1610613745 */,
  MAX_BOOT_LEVEL = (android.hardware.security.keymint.TagType.UINT | 1010) /* 805307378 */,
}
