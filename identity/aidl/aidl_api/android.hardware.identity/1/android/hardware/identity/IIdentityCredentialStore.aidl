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

package android.hardware.identity;
@VintfStability
interface IIdentityCredentialStore {
  android.hardware.identity.HardwareInformation getHardwareInformation();
  android.hardware.identity.IWritableIdentityCredential createCredential(in @utf8InCpp String docType, in boolean testCredential);
  android.hardware.identity.IIdentityCredential getCredential(in android.hardware.identity.CipherSuite cipherSuite, in byte[] credentialData);
  const int STATUS_OK = 0;
  const int STATUS_FAILED = 1;
  const int STATUS_CIPHER_SUITE_NOT_SUPPORTED = 2;
  const int STATUS_INVALID_DATA = 3;
  const int STATUS_INVALID_AUTH_TOKEN = 4;
  const int STATUS_INVALID_ITEMS_REQUEST_MESSAGE = 5;
  const int STATUS_READER_SIGNATURE_CHECK_FAILED = 6;
  const int STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND = 7;
  const int STATUS_USER_AUTHENTICATION_FAILED = 8;
  const int STATUS_READER_AUTHENTICATION_FAILED = 9;
  const int STATUS_NO_ACCESS_CONTROL_PROFILES = 10;
  const int STATUS_NOT_IN_REQUEST_MESSAGE = 11;
  const int STATUS_SESSION_TRANSCRIPT_MISMATCH = 12;
}
