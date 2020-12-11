///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.keymint;
@VintfStability
interface IKeyMintDevice {
  android.hardware.keymint.KeyMintHardwareInfo getHardwareInfo();
  android.hardware.keymint.VerificationToken verifyAuthorization(in long challenge, in android.hardware.keymint.HardwareAuthToken token);
  void addRngEntropy(in byte[] data);
  void generateKey(in android.hardware.keymint.KeyParameter[] keyParams, out android.hardware.keymint.ByteArray generatedKeyBlob, out android.hardware.keymint.KeyCharacteristics generatedKeyCharacteristics, out android.hardware.keymint.Certificate[] outCertChain);
  void importKey(in android.hardware.keymint.KeyParameter[] inKeyParams, in android.hardware.keymint.KeyFormat inKeyFormat, in byte[] inKeyData, out android.hardware.keymint.ByteArray outImportedKeyBlob, out android.hardware.keymint.KeyCharacteristics outImportedKeyCharacteristics, out android.hardware.keymint.Certificate[] outCertChain);
  void importWrappedKey(in byte[] inWrappedKeyData, in byte[] inWrappingKeyBlob, in byte[] inMaskingKey, in android.hardware.keymint.KeyParameter[] inUnwrappingParams, in long inPasswordSid, in long inBiometricSid, out android.hardware.keymint.ByteArray outImportedKeyBlob, out android.hardware.keymint.KeyCharacteristics outImportedKeyCharacteristics);
  byte[] upgradeKey(in byte[] inKeyBlobToUpgrade, in android.hardware.keymint.KeyParameter[] inUpgradeParams);
  void deleteKey(in byte[] inKeyBlob);
  void deleteAllKeys();
  void destroyAttestationIds();
  android.hardware.keymint.BeginResult begin(in android.hardware.keymint.KeyPurpose inPurpose, in byte[] inKeyBlob, in android.hardware.keymint.KeyParameter[] inParams, in android.hardware.keymint.HardwareAuthToken inAuthToken);
  const int AUTH_TOKEN_MAC_LENGTH = 32;
}
