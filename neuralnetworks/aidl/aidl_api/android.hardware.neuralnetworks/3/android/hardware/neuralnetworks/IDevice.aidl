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

package android.hardware.neuralnetworks;
@VintfStability
interface IDevice {
  android.hardware.neuralnetworks.DeviceBuffer allocate(in android.hardware.neuralnetworks.BufferDesc desc, in android.hardware.neuralnetworks.IPreparedModelParcel[] preparedModels, in android.hardware.neuralnetworks.BufferRole[] inputRoles, in android.hardware.neuralnetworks.BufferRole[] outputRoles);
  android.hardware.neuralnetworks.Capabilities getCapabilities();
  android.hardware.neuralnetworks.NumberOfCacheFiles getNumberOfCacheFilesNeeded();
  android.hardware.neuralnetworks.Extension[] getSupportedExtensions();
  boolean[] getSupportedOperations(in android.hardware.neuralnetworks.Model model);
  android.hardware.neuralnetworks.DeviceType getType();
  String getVersionString();
  void prepareModel(in android.hardware.neuralnetworks.Model model, in android.hardware.neuralnetworks.ExecutionPreference preference, in android.hardware.neuralnetworks.Priority priority, in long deadlineNs, in ParcelFileDescriptor[] modelCache, in ParcelFileDescriptor[] dataCache, in byte[] token, in android.hardware.neuralnetworks.IPreparedModelCallback callback);
  void prepareModelFromCache(in long deadlineNs, in ParcelFileDescriptor[] modelCache, in ParcelFileDescriptor[] dataCache, in byte[] token, in android.hardware.neuralnetworks.IPreparedModelCallback callback);
  const int BYTE_SIZE_OF_CACHE_TOKEN = 32;
  const int MAX_NUMBER_OF_CACHE_FILES = 32;
  const int EXTENSION_TYPE_HIGH_BITS_PREFIX = 15;
  const int EXTENSION_TYPE_LOW_BITS_TYPE = 16;
  const int OPERAND_TYPE_BASE_MAX = 65535;
  const int OPERATION_TYPE_BASE_MAX = 65535;
}
