/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

package android.hardware.graphics.composer3;
@Backing(type="int") @VintfStability
enum Command {
  LENGTH_MASK = 65535,
  OPCODE_SHIFT = 16,
  OPCODE_MASK = -65536,
  SELECT_DISPLAY = 0,
  SELECT_LAYER = 65536,
  SET_ERROR = 16777216,
  SET_CHANGED_COMPOSITION_TYPES = 16842752,
  SET_DISPLAY_REQUESTS = 16908288,
  SET_PRESENT_FENCE = 16973824,
  SET_RELEASE_FENCES = 17039360,
  SET_COLOR_TRANSFORM = 33554432,
  SET_CLIENT_TARGET = 33619968,
  SET_OUTPUT_BUFFER = 33685504,
  VALIDATE_DISPLAY = 33751040,
  ACCEPT_DISPLAY_CHANGES = 33816576,
  PRESENT_DISPLAY = 33882112,
  PRESENT_OR_VALIDATE_DISPLAY = 33947648,
  SET_LAYER_CURSOR_POSITION = 50331648,
  SET_LAYER_BUFFER = 50397184,
  SET_LAYER_SURFACE_DAMAGE = 50462720,
  SET_LAYER_BLEND_MODE = 67108864,
  SET_LAYER_COLOR = 67174400,
  SET_LAYER_COMPOSITION_TYPE = 67239936,
  SET_LAYER_DATASPACE = 67305472,
  SET_LAYER_DISPLAY_FRAME = 67371008,
  SET_LAYER_PLANE_ALPHA = 67436544,
  SET_LAYER_SIDEBAND_STREAM = 67502080,
  SET_LAYER_SOURCE_CROP = 67567616,
  SET_LAYER_TRANSFORM = 67633152,
  SET_LAYER_VISIBLE_REGION = 67698688,
  SET_LAYER_Z_ORDER = 67764224,
  SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT = 67829760,
  SET_LAYER_PER_FRAME_METADATA = 50528256,
  SET_LAYER_FLOAT_COLOR = 67895296,
  SET_LAYER_COLOR_TRANSFORM = 67960832,
  SET_LAYER_PER_FRAME_METADATA_BLOBS = 50593792,
  SET_CLIENT_TARGET_PROPERTY = 17104896,
  SET_LAYER_GENERIC_METADATA = 68026368,
}
