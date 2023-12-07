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

package android.hardware.wifi.supplicant;
@VintfStability
interface ISupplicantStaNetwork {
  void disable();
  void enable(in boolean noConnect);
  void enableSaePkOnlyMode(in boolean enable);
  void enableSuiteBEapOpenSslCiphers();
  void enableTlsSuiteBEapPhase1Param(in boolean enable);
  android.hardware.wifi.supplicant.AuthAlgMask getAuthAlg();
  byte[] getBssid();
  String getEapAltSubjectMatch();
  byte[] getEapAnonymousIdentity();
  String getEapCACert();
  String getEapCAPath();
  String getEapClientCert();
  String getEapDomainSuffixMatch();
  boolean getEapEngine();
  String getEapEngineId();
  byte[] getEapIdentity();
  android.hardware.wifi.supplicant.EapMethod getEapMethod();
  byte[] getEapPassword();
  android.hardware.wifi.supplicant.EapPhase2Method getEapPhase2Method();
  String getEapPrivateKeyId();
  String getEapSubjectMatch();
  boolean getEdmg();
  android.hardware.wifi.supplicant.GroupCipherMask getGroupCipher();
  android.hardware.wifi.supplicant.GroupMgmtCipherMask getGroupMgmtCipher();
  int getId();
  String getIdStr();
  String getInterfaceName();
  android.hardware.wifi.supplicant.KeyMgmtMask getKeyMgmt();
  android.hardware.wifi.supplicant.OcspType getOcsp();
  android.hardware.wifi.supplicant.PairwiseCipherMask getPairwiseCipher();
  android.hardware.wifi.supplicant.ProtoMask getProto();
  byte[] getPsk();
  String getPskPassphrase();
  boolean getRequirePmf();
  String getSaePassword();
  String getSaePasswordId();
  boolean getScanSsid();
  byte[] getSsid();
  android.hardware.wifi.supplicant.IfaceType getType();
  String getWapiCertSuite();
  byte[] getWepKey(in int keyIdx);
  int getWepTxKeyIdx();
  byte[] getWpsNfcConfigurationToken();
  void registerCallback(in android.hardware.wifi.supplicant.ISupplicantStaNetworkCallback callback);
  void select();
  void sendNetworkEapIdentityResponse(in byte[] identity, in byte[] encryptedIdentity);
  void sendNetworkEapSimGsmAuthFailure();
  void sendNetworkEapSimGsmAuthResponse(in android.hardware.wifi.supplicant.NetworkResponseEapSimGsmAuthParams[] params);
  void sendNetworkEapSimUmtsAuthFailure();
  void sendNetworkEapSimUmtsAuthResponse(in android.hardware.wifi.supplicant.NetworkResponseEapSimUmtsAuthParams params);
  void sendNetworkEapSimUmtsAutsResponse(in byte[] auts);
  void setAuthAlg(in android.hardware.wifi.supplicant.AuthAlgMask authAlgMask);
  void setBssid(in byte[] bssid);
  void setDppKeys(in android.hardware.wifi.supplicant.DppConnectionKeys keys);
  void setEapAltSubjectMatch(in String match);
  void setEapAnonymousIdentity(in byte[] identity);
  void setEapCACert(in String path);
  void setEapCAPath(in String path);
  void setEapClientCert(in String path);
  void setEapDomainSuffixMatch(in String match);
  void setEapEncryptedImsiIdentity(in byte[] identity);
  void setEapEngine(in boolean enable);
  void setEapEngineID(in String id);
  void setEapErp(in boolean enable);
  void setEapIdentity(in byte[] identity);
  void setEapMethod(in android.hardware.wifi.supplicant.EapMethod method);
  void setEapPassword(in byte[] password);
  void setEapPhase2Method(in android.hardware.wifi.supplicant.EapPhase2Method method);
  void setEapPrivateKeyId(in String id);
  void setEapSubjectMatch(in String match);
  void setEdmg(in boolean enable);
  void setGroupCipher(in android.hardware.wifi.supplicant.GroupCipherMask groupCipherMask);
  void setGroupMgmtCipher(in android.hardware.wifi.supplicant.GroupMgmtCipherMask groupMgmtCipherMask);
  void setIdStr(in String idStr);
  void setKeyMgmt(in android.hardware.wifi.supplicant.KeyMgmtMask keyMgmtMask);
  void setOcsp(in android.hardware.wifi.supplicant.OcspType ocspType);
  void setPairwiseCipher(in android.hardware.wifi.supplicant.PairwiseCipherMask pairwiseCipherMask);
  void setPmkCache(in byte[] serializedEntry);
  void setProactiveKeyCaching(in boolean enable);
  void setProto(in android.hardware.wifi.supplicant.ProtoMask protoMask);
  void setPsk(in byte[] psk);
  void setPskPassphrase(in String psk);
  void setRequirePmf(in boolean enable);
  void setSaeH2eMode(in android.hardware.wifi.supplicant.SaeH2eMode mode);
  void setSaePassword(in String saePassword);
  void setSaePasswordId(in String saePasswordId);
  void setScanSsid(in boolean enable);
  void setSsid(in byte[] ssid);
  void setUpdateIdentifier(in int id);
  void setWapiCertSuite(in String suite);
  void setWepKey(in int keyIdx, in byte[] wepKey);
  void setWepTxKeyIdx(in int keyIdx);
  void setRoamingConsortiumSelection(in byte[] selectedRcoi);
  void setMinimumTlsVersionEapPhase1Param(android.hardware.wifi.supplicant.TlsVersion tlsVersion);
  void setStrictConservativePeerMode(in boolean enable);
  void disableEht();
  void setVendorData(in android.hardware.wifi.common.OuiKeyedData[] vendorData);
  const int SSID_MAX_LEN_IN_BYTES = 32;
  const int PSK_PASSPHRASE_MIN_LEN_IN_BYTES = 8;
  const int PSK_PASSPHRASE_MAX_LEN_IN_BYTES = 63;
  const int WEP_KEYS_MAX_NUM = 4;
  const int WEP40_KEY_LEN_IN_BYTES = 5;
  const int WEP104_KEY_LEN_IN_BYTES = 13;
}
