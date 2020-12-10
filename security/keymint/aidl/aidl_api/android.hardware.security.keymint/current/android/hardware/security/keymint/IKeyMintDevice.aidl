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

package android.hardware.security.keymint;
@VintfStability
interface IKeyMintDevice {
  android.hardware.security.keymint.KeyMintHardwareInfo getHardwareInfo();
  android.hardware.security.keymint.VerificationToken verifyAuthorization(in long challenge, in android.hardware.security.keymint.HardwareAuthToken token);
  void addRngEntropy(in byte[] data);
  void generateKey(in android.hardware.security.keymint.KeyParameter[] keyParams, out android.hardware.security.keymint.ByteArray generatedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics generatedKeyCharacteristics, out android.hardware.security.keymint.Certificate[] outCertChain);
  void importKey(in android.hardware.security.keymint.KeyParameter[] inKeyParams, in android.hardware.security.keymint.KeyFormat inKeyFormat, in byte[] inKeyData, out android.hardware.security.keymint.ByteArray outImportedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics outImportedKeyCharacteristics, out android.hardware.security.keymint.Certificate[] outCertChain);
  void importWrappedKey(in byte[] inWrappedKeyData, in byte[] inWrappingKeyBlob, in byte[] inMaskingKey, in android.hardware.security.keymint.KeyParameter[] inUnwrappingParams, in long inPasswordSid, in long inBiometricSid, out android.hardware.security.keymint.ByteArray outImportedKeyBlob, out android.hardware.security.keymint.KeyCharacteristics outImportedKeyCharacteristics);
  byte[] upgradeKey(in byte[] inKeyBlobToUpgrade, in android.hardware.security.keymint.KeyParameter[] inUpgradeParams);
  void deleteKey(in byte[] inKeyBlob);
  void deleteAllKeys();
  void destroyAttestationIds();
  android.hardware.security.keymint.BeginResult begin(in android.hardware.security.keymint.KeyPurpose inPurpose, in byte[] inKeyBlob, in android.hardware.security.keymint.KeyParameter[] inParams, in android.hardware.security.keymint.HardwareAuthToken inAuthToken);
  const int AUTH_TOKEN_MAC_LENGTH = 32;
}
