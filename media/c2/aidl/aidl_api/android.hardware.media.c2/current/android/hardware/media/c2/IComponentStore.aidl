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

package android.hardware.media.c2;
@VintfStability
interface IComponentStore {
  void copyBuffer(in android.hardware.media.c2.Buffer src, in android.hardware.media.c2.Buffer dst);
  android.hardware.media.c2.IComponent createComponent(in String name, in android.hardware.media.c2.IComponentListener listener, in android.hardware.media.bufferpool2.IClientManager pool);
  android.hardware.media.c2.IComponentInterface createInterface(in String name);
  android.hardware.media.c2.IConfigurable getConfigurable();
  android.hardware.media.bufferpool2.IClientManager getPoolClientManager();
  android.hardware.media.c2.StructDescriptor[] getStructDescriptors(in int[] indices);
  android.hardware.media.c2.IComponentStore.ComponentTraits[] listComponents();
  @VintfStability
  parcelable ComponentTraits {
    String name;
    android.hardware.media.c2.IComponentStore.ComponentTraits.Domain domain;
    android.hardware.media.c2.IComponentStore.ComponentTraits.Kind kind;
    int rank;
    String mediaType;
    String[] aliases;
    @Backing(type="int") @VintfStability
    enum Kind {
      OTHER = 0,
      DECODER,
      ENCODER,
    }
    @Backing(type="int") @VintfStability
    enum Domain {
      OTHER = 0,
      VIDEO,
      AUDIO,
      IMAGE,
    }
  }
}
