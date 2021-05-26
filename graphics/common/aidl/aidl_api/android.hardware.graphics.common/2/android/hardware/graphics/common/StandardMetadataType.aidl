/**
 * Copyright (c) 2019,libgralloctypes_helper The Android Open Source Project
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

package android.hardware.graphics.common;
@Backing(type="long") @VintfStability
enum StandardMetadataType {
  INVALID = 0,
  BUFFER_ID = 1,
  NAME = 2,
  WIDTH = 3,
  HEIGHT = 4,
  LAYER_COUNT = 5,
  PIXEL_FORMAT_REQUESTED = 6,
  PIXEL_FORMAT_FOURCC = 7,
  PIXEL_FORMAT_MODIFIER = 8,
  USAGE = 9,
  ALLOCATION_SIZE = 10,
  PROTECTED_CONTENT = 11,
  COMPRESSION = 12,
  INTERLACED = 13,
  CHROMA_SITING = 14,
  PLANE_LAYOUTS = 15,
  CROP = 16,
  DATASPACE = 17,
  BLEND_MODE = 18,
  SMPTE2086 = 19,
  CTA861_3 = 20,
  SMPTE2094_40 = 21,
}
