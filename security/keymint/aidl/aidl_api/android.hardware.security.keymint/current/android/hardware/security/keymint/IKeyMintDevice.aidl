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
@VintfStability
interface IKeyMintDevice {
  android.hardware.security.keymint.KeyMintHardwareInfo getHardwareInfo();
  void addRngEntropy(in byte[] data);
  android.hardware.security.keymint.KeyCreationResult generateKey(in android.hardware.security.keymint.KeyParameter[] keyParams);
  android.hardware.security.keymint.KeyCreationResult importKey(in android.hardware.security.keymint.KeyParameter[] keyParams, in android.hardware.security.keymint.KeyFormat keyFormat, in byte[] keyData);
  android.hardware.security.keymint.KeyCreationResult importWrappedKey(in byte[] wrappedKeyData, in byte[] wrappingKeyBlob, in byte[] maskingKey, in android.hardware.security.keymint.KeyParameter[] unwrappingParams, in long passwordSid, in long biometricSid);
  byte[] upgradeKey(in byte[] inKeyBlobToUpgrade, in android.hardware.security.keymint.KeyParameter[] inUpgradeParams);
  void deleteKey(in byte[] inKeyBlob);
  void deleteAllKeys();
  void destroyAttestationIds();
  android.hardware.security.keymint.BeginResult begin(in android.hardware.security.keymint.KeyPurpose inPurpose, in byte[] inKeyBlob, in android.hardware.security.keymint.KeyParameter[] inParams, in android.hardware.security.keymint.HardwareAuthToken inAuthToken);
  const int AUTH_TOKEN_MAC_LENGTH = 32;
}
