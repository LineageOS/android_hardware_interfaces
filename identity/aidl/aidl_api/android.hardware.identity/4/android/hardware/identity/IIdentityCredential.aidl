/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;
@VintfStability
interface IIdentityCredential {
  /**
   * @deprecated use deleteCredentalWithChallenge() instead.
   */
  byte[] deleteCredential();
  byte[] createEphemeralKeyPair();
  void setReaderEphemeralPublicKey(in byte[] publicKey);
  long createAuthChallenge();
  void startRetrieval(in android.hardware.identity.SecureAccessControlProfile[] accessControlProfiles, in android.hardware.keymaster.HardwareAuthToken authToken, in byte[] itemsRequest, in byte[] signingKeyBlob, in byte[] sessionTranscript, in byte[] readerSignature, in int[] requestCounts);
  void startRetrieveEntryValue(in @utf8InCpp String nameSpace, in @utf8InCpp String name, in int entrySize, in int[] accessControlProfileIds);
  byte[] retrieveEntryValue(in byte[] encryptedContent);
  @SuppressWarnings(value={"out-array"}) void finishRetrieval(out byte[] mac, out byte[] deviceNameSpaces);
  @SuppressWarnings(value={"out-array"}) android.hardware.identity.Certificate generateSigningKeyPair(out byte[] signingKeyBlob);
  void setRequestedNamespaces(in android.hardware.identity.RequestNamespace[] requestNamespaces);
  void setVerificationToken(in android.hardware.keymaster.VerificationToken verificationToken);
  byte[] deleteCredentialWithChallenge(in byte[] challenge);
  byte[] proveOwnership(in byte[] challenge);
  android.hardware.identity.IWritableIdentityCredential updateCredential();
}
