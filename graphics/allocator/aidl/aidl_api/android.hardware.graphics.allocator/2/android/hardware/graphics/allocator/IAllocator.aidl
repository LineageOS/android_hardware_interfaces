/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.graphics.allocator;
@VintfStability
interface IAllocator {
  /**
   * @deprecated As of android.hardware.graphics.allocator-V2 in combination with AIMAPPER_VERSION_5 this is deprecated & replaced with allocate2. If android.hardware.graphics.mapper@4 is still in use, however, this is still required to be implemented.
   */
  android.hardware.graphics.allocator.AllocationResult allocate(in byte[] descriptor, in int count);
  android.hardware.graphics.allocator.AllocationResult allocate2(in android.hardware.graphics.allocator.BufferDescriptorInfo descriptor, in int count);
  boolean isSupported(in android.hardware.graphics.allocator.BufferDescriptorInfo descriptor);
  String getIMapperLibrarySuffix();
}
