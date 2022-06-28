/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.drm;
@VintfStability
interface IDrmPlugin {
  void closeSession(in byte[] sessionId);
  byte[] decrypt(in byte[] sessionId, in byte[] keyId, in byte[] input, in byte[] iv);
  byte[] encrypt(in byte[] sessionId, in byte[] keyId, in byte[] input, in byte[] iv);
  android.hardware.drm.HdcpLevels getHdcpLevels();
  android.hardware.drm.KeyRequest getKeyRequest(in byte[] scope, in byte[] initData, in String mimeType, in android.hardware.drm.KeyType keyType, in android.hardware.drm.KeyValue[] optionalParameters);
  List<android.hardware.drm.LogMessage> getLogMessages();
  List<android.hardware.drm.DrmMetricGroup> getMetrics();
  android.hardware.drm.NumberOfSessions getNumberOfSessions();
  List<android.hardware.drm.KeySetId> getOfflineLicenseKeySetIds();
  android.hardware.drm.OfflineLicenseState getOfflineLicenseState(in android.hardware.drm.KeySetId keySetId);
  byte[] getPropertyByteArray(in String propertyName);
  String getPropertyString(in String propertyName);
  android.hardware.drm.ProvisionRequest getProvisionRequest(in String certificateType, in String certificateAuthority);
  android.hardware.drm.SecureStop getSecureStop(in android.hardware.drm.SecureStopId secureStopId);
  List<android.hardware.drm.SecureStopId> getSecureStopIds();
  List<android.hardware.drm.SecureStop> getSecureStops();
  android.hardware.drm.SecurityLevel getSecurityLevel(in byte[] sessionId);
  byte[] openSession(in android.hardware.drm.SecurityLevel securityLevel);
  android.hardware.drm.KeySetId provideKeyResponse(in byte[] scope, in byte[] response);
  android.hardware.drm.ProvideProvisionResponseResult provideProvisionResponse(in byte[] response);
  List<android.hardware.drm.KeyValue> queryKeyStatus(in byte[] sessionId);
  void releaseAllSecureStops();
  void releaseSecureStop(in android.hardware.drm.SecureStopId secureStopId);
  void releaseSecureStops(in android.hardware.drm.OpaqueData ssRelease);
  void removeAllSecureStops();
  void removeKeys(in byte[] sessionId);
  void removeOfflineLicense(in android.hardware.drm.KeySetId keySetId);
  void removeSecureStop(in android.hardware.drm.SecureStopId secureStopId);
  boolean requiresSecureDecoder(in String mime, in android.hardware.drm.SecurityLevel level);
  void restoreKeys(in byte[] sessionId, in android.hardware.drm.KeySetId keySetId);
  void setCipherAlgorithm(in byte[] sessionId, in String algorithm);
  void setListener(in android.hardware.drm.IDrmPluginListener listener);
  void setMacAlgorithm(in byte[] sessionId, in String algorithm);
  void setPlaybackId(in byte[] sessionId, in String playbackId);
  void setPropertyByteArray(in String propertyName, in byte[] value);
  void setPropertyString(in String propertyName, in String value);
  byte[] sign(in byte[] sessionId, in byte[] keyId, in byte[] message);
  byte[] signRSA(in byte[] sessionId, in String algorithm, in byte[] message, in byte[] wrappedkey);
  boolean verify(in byte[] sessionId, in byte[] keyId, in byte[] message, in byte[] signature);
}
