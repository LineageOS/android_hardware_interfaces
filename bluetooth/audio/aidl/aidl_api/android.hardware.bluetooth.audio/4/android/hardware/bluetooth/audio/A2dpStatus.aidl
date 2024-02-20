/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;
@Backing(type="byte") @VintfStability
enum A2dpStatus {
  OK = 0,
  BAD_LENGTH = 0x11u8,
  BAD_PAYLOAD_FORMAT = 0x18u8,
  INVALID_CODEC_TYPE = 0xC1u8,
  NOT_SUPPORTED_CODEC_TYPE = 0xC2u8,
  INVALID_SAMPLING_FREQUENCY = 0xC3u8,
  NOT_SUPPORTED_SAMPLING_FREQUENCY = 0xC4u8,
  INVALID_CHANNEL_MODE = 0xC5u8,
  NOT_SUPPORTED_CHANNEL_MODE = 0xC6u8,
  INVALID_SUBBANDS = 0xC7u8,
  NOT_SUPPORTED_SUBBANDS = 0xC8u8,
  INVALID_ALLOCATION_METHOD = 0xC9u8,
  NOT_SUPPORTED_ALLOCATION_METHOD = 0xCAu8,
  INVALID_MINIMUM_BITPOOL_VALUE = 0xCBu8,
  NOT_SUPPORTED_MINIMUM_BITPOOL_VALUE = 0xCCu8,
  INVALID_MAXIMUM_BITPOOL_VALUE = 0xCDu8,
  NOT_SUPPORTED_MAXIMUM_BITPOOL_VALUE = 0xCEu8,
  NOT_SUPPORTED_VBR = 0xD3u8,
  NOT_SUPPORTED_BIT_RATE = 0xD5u8,
  INVALID_OBJECT_TYPE = 0xD6u8,
  NOT_SUPPORTED_OBJECT_TYPE = 0xD7u8,
  INVALID_CHANNELS = 0xD8u8,
  NOT_SUPPORTED_CHANNELS = 0xD9u8,
  INVALID_BLOCK_LENGTH = 0xDDu8,
  INVALID_CODEC_PARAMETER = 0xE2u8,
  NOT_SUPPORTED_CODEC_PARAMETER = 0xE3u8,
  INVALID_DRC = 0xE4u8,
  NOT_SUPPORTED_DRC = 0xE5u8,
}
