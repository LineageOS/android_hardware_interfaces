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

package android.hardware.security.see.hwcrypto;
interface IHwCryptoKey {
  android.hardware.security.see.hwcrypto.IHwCryptoKey.DiceCurrentBoundKeyResult deriveCurrentDicePolicyBoundKey(in android.hardware.security.see.hwcrypto.IHwCryptoKey.DiceBoundDerivationKey derivationKey);
  android.hardware.security.see.hwcrypto.IHwCryptoKey.DiceBoundKeyResult deriveDicePolicyBoundKey(in android.hardware.security.see.hwcrypto.IHwCryptoKey.DiceBoundDerivationKey derivationKey, in byte[] dicePolicyForKeyVersion);
  android.hardware.security.see.hwcrypto.IHwCryptoKey.DerivedKey deriveKey(in android.hardware.security.see.hwcrypto.IHwCryptoKey.DerivedKeyParameters parameters);
  enum DeviceKeyId {
    DEVICE_BOUND_KEY,
    BATCH_KEY,
  }
  union DiceBoundDerivationKey {
    android.hardware.security.see.hwcrypto.IOpaqueKey opaqueKey;
    android.hardware.security.see.hwcrypto.IHwCryptoKey.DeviceKeyId keyId;
  }
  parcelable DiceCurrentBoundKeyResult {
    android.hardware.security.see.hwcrypto.IOpaqueKey diceBoundKey;
    byte[] dicePolicyForKeyVersion;
  }
  parcelable DiceBoundKeyResult {
    android.hardware.security.see.hwcrypto.IOpaqueKey diceBoundKey;
    boolean dicePolicyWasCurrent;
  }
  parcelable ClearKeyPolicy {
    int keySizeBytes;
  }
  union DerivedKeyPolicy {
    android.hardware.security.see.hwcrypto.KeyPolicy opaqueKey;
    android.hardware.security.see.hwcrypto.IHwCryptoKey.ClearKeyPolicy clearKey;
  }
  parcelable DerivedKeyParameters {
    android.hardware.security.see.hwcrypto.IOpaqueKey derivationKey;
    android.hardware.security.see.hwcrypto.IHwCryptoKey.DerivedKeyPolicy keyPolicy;
    byte[] context;
  }
  union DerivedKey {
    byte[] explicitKey = {};
    android.hardware.security.see.hwcrypto.IOpaqueKey opaque;
  }
}
