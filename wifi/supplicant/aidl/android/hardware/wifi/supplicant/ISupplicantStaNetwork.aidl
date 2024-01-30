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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.AuthAlgMask;
import android.hardware.wifi.supplicant.DppConnectionKeys;
import android.hardware.wifi.supplicant.EapMethod;
import android.hardware.wifi.supplicant.EapPhase2Method;
import android.hardware.wifi.supplicant.GroupCipherMask;
import android.hardware.wifi.supplicant.GroupMgmtCipherMask;
import android.hardware.wifi.supplicant.ISupplicantStaNetworkCallback;
import android.hardware.wifi.supplicant.IfaceType;
import android.hardware.wifi.supplicant.KeyMgmtMask;
import android.hardware.wifi.supplicant.NetworkResponseEapSimGsmAuthParams;
import android.hardware.wifi.supplicant.NetworkResponseEapSimUmtsAuthParams;
import android.hardware.wifi.supplicant.OcspType;
import android.hardware.wifi.supplicant.PairwiseCipherMask;
import android.hardware.wifi.supplicant.ProtoMask;
import android.hardware.wifi.supplicant.SaeH2eMode;
import android.hardware.wifi.supplicant.TlsVersion;

/**
 * Interface exposed by the supplicant for each station mode network
 * configuration it controls.
 */
@VintfStability
interface ISupplicantStaNetwork {
    /**
     * Max length of SSID param.
     */
    const int SSID_MAX_LEN_IN_BYTES = 32;

    /**
     * Min length of PSK passphrase param.
     */
    const int PSK_PASSPHRASE_MIN_LEN_IN_BYTES = 8;

    /**
     * Max length of PSK passphrase param.
     */
    const int PSK_PASSPHRASE_MAX_LEN_IN_BYTES = 63;

    /**
     * Max number of WEP keys param.
     */
    const int WEP_KEYS_MAX_NUM = 4;

    /**
     * Length of each WEP40 keys param.
     */
    const int WEP40_KEY_LEN_IN_BYTES = 5;

    /**
     * Length of each WEP104 keys param.
     */
    const int WEP104_KEY_LEN_IN_BYTES = 13;

    /**
     * Disable the network for connection purposes.
     *
     * This must trigger a disconnection from the network, if currently
     * connected to this one.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void disable();

    /**
     * Enable the network for connection purposes.
     *
     * This must trigger a connection to the network if:
     * a) |noConnect| is false, and
     * b) This is the only network configured, and
     * c) Is visible in the current scan results.
     *
     * @param noConnect Only enable the network, don't trigger a connect.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void enable(in boolean noConnect);

    /**
     * Set whether to enable SAE PK (Public Key) only mode to enable public AP validation.
     * When enabled, only SAE PK network is allowed; otherwise PK is optional.
     * If this API is not called before connecting to an SAE
     * network, SAE PK mode depends on SAE PK config in wpa_supplicant configuration.
     * If SAE PK config of wpa_supplicant configuration is not set,
     * the default mode is optional (support for both PK and standard mode).
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void enableSaePkOnlyMode(in boolean enable);

    /**
     * Set EAP OpenSSL Suite-B-192 ciphers for WPA3-Enterprise
     *        Supported option:
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void enableSuiteBEapOpenSslCiphers();

    /**
     * Enable TLS Suite-B in EAP Phase1
     *
     * @param enable Set to true to enable TLS Suite-B in EAP phase1
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void enableTlsSuiteBEapPhase1Param(in boolean enable);

    /**
     * Get the auth alg mask set for the network.
     *
     * @return Combination of |AuthAlgMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    AuthAlgMask getAuthAlg();

    /**
     * Get the BSSID set for this network.
     *
     * @return bssid Value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getBssid();

    /**
     * Get EAP Alt subject match set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapAltSubjectMatch();

    /**
     * Get EAP Anonymous Identity set for this network.
     *
     * @return identity value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getEapAnonymousIdentity();

    /**
     * Get EAP CA certificate file path set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapCACert();

    /**
     * Get EAP CA certificate directory path set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapCAPath();

    /**
     * Get EAP Client certificate file path set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapClientCert();

    /**
     * Get EAP Domain suffix match set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapDomainSuffixMatch();

    /**
     * Get whether EAP Open SSL Engine is enabled for this network.
     *
     * @return true if set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean getEapEngine();

    /**
     * Get EAP Open SSL Engine ID set for this network.
     *
     * @return value set.
     * throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapEngineId();

    /**
     * Get EAP Identity set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getEapIdentity();

    /**
     * Get EAP Method set for this network.
     *
     * @return value set.
     *        Must be one of |EapMethod| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    EapMethod getEapMethod();

    /**
     * Get EAP Password set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getEapPassword();

    /**
     * Get EAP Phase2 Method set for this network.
     *
     * @return value set.
     *        Must be one of |EapPhase2Method| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    EapPhase2Method getEapPhase2Method();

    /**
     * Get EAP private key Id set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|)
     */
    String getEapPrivateKeyId();

    /**
     * Get EAP subject match set for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getEapSubjectMatch();

    /**
     * Get whether enhanced directional multi-gigabit (802.11ay EDMG) is enabled for this network.
     *
     * @return true if set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean getEdmg();

    /**
     * Get the group cipher mask set for the network.
     *
     * @return Combination of |GroupCipherMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    GroupCipherMask getGroupCipher();

    /**
     * Get the group management cipher mask set for the network.
     *
     * @return Combination of |GroupMgmtCipherMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    GroupMgmtCipherMask getGroupMgmtCipher();

    /**
     * Retrieves the ID allocated to this network by the supplicant.
     *
     * This is not the |SSID| of the network, but an internal identifier for
     * this network used by the supplicant.
     *
     * @return Network ID.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    int getId();

    /**
     * Get ID string set for this network.
     * Network identifier string for external scripts.
     *
     * @return ID string set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getIdStr();

    /**
     * Retrieves the name of the interface this network belongs to.
     *
     * @return Name of the network interface, e.g., wlan0
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getInterfaceName();

    /**
     * Get the key mgmt mask set for the network.
     *
     * @return Combination of |KeyMgmtMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    KeyMgmtMask getKeyMgmt();

    /**
     * Get OCSP (Online Certificate Status Protocol) type for this network.
     *
     * @return ocsp type.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    OcspType getOcsp();

    /**
     * Get the pairwise cipher mask set for the network.
     *
     * @return Combination of |PairwiseCipherMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    PairwiseCipherMask getPairwiseCipher();

    /**
     * Get the proto mask set for the network.
     *
     * @return Combination of |ProtoMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    ProtoMask getProto();

    /**
     * Get raw psk for WPA_PSK network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getPsk();

    /**
     * Get passphrase for WPA_PSK network.
     * Must return a failure if network has no passphrase set (use |getPsk| if
     * network was configured with raw psk instead).
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getPskPassphrase();

    /**
     * Get whether RequirePmf is enabled for this network.
     *
     * @return true if set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean getRequirePmf();

    /**
     * Get SAE password for WPA3-Personal
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getSaePassword();

    /**
     * Get SAE password ID for WPA3-Personal
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getSaePasswordId();

    /**
     * Get whether Probe Requests are being sent for this network (hidden).
     *
     * @return true if set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean getScanSsid();

    /**
     *
     * Getters for the various network params.
     *
     */

    /**
     * Get SSID for this network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getSsid();

    /**
     * Retrieves the type of the interface this network belongs to.
     *
     * @return Type of the network interface, e.g., STA.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    IfaceType getType();

    /**
     * Get WAPI certificate suite name set for this network.
     *
     * @return The name of a suite.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    String getWapiCertSuite();

    /**
     * Get WEP key for WEP network.
     *
     * @param keyIdx Index of wep key to be fetched.
     *        Max of |WEP_KEYS_MAX_NUM|.
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getWepKey(in int keyIdx);

    /**
     * Get default Tx key index for WEP network.
     *
     * @return value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    int getWepTxKeyIdx();

    /**
     * Retrieves a WPS-NFC configuration token for this network.
     *
     * @return Bytes representing WPS-NFC configuration token.
     *         This is a dump of all the WPS atrributes of the AP configuration
     *         as specified in the Wi-Fi Protected Setup Specification.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getWpsNfcConfigurationToken();

    /**
     * Register for callbacks from this network.
     *
     * These callbacks are invoked for events that are specific to this network.
     * Registration of multiple callback objects is supported. These objects must
     * be automatically deleted when the corresponding client process is dead or
     * if this network is removed.
     *
     * @param callback An instance of the |ISupplicantStaNetworkCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void registerCallback(in ISupplicantStaNetworkCallback callback);

    /**
     * Initiate connection to this network.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void select();

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapIdentityRequest| request.
     *
     * @param identity Identity string containing the IMSI.
     * @param encryptedIdentity Identity string containing the encrypted IMSI.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapIdentityResponse(in byte[] identity, in byte[] encryptedIdentity);

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapSimGsmAuthRequest| request.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapSimGsmAuthFailure();

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapSimGsmAuthRequest| request.
     *
     * @param params Params to be used for EAP GSM authentication.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapSimGsmAuthResponse(in NetworkResponseEapSimGsmAuthParams[] params);

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapSimUmtsAuthRequest| request.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapSimUmtsAuthFailure();

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapSimUmtsAuthRequest| request.
     *
     * @param params Params to be used for EAP UMTS authentication.
     * @throws ServiceSpecificException with one of the following values:

     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapSimUmtsAuthResponse(in NetworkResponseEapSimUmtsAuthParams params);

    /**
     * Used to send a response to the
     * |ISupplicantNetworkCallback.onNetworkEapSimUmtsAuthRequest| request.
     *
     * @param auts Params to be used for EAP UMTS authentication.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void sendNetworkEapSimUmtsAutsResponse(in byte[] auts);

    /**
     * Set auth alg mask for the network.
     *
     * @param authAlgMask value to set.
     *        Combination of |ProtoMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setAuthAlg(in AuthAlgMask authAlgMask);

    /**
     * Set the network to only connect to an AP with provided BSSID.
     *
     * @param bssid value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setBssid(in byte[] bssid);

    /**
     * Set DPP keys for network which supports DPP AKM.
     *
     * @param keys connection keys needed to make DPP
     * AKM based network connection.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setDppKeys(in DppConnectionKeys keys);

    /**
     * Set EAP Alt subject match for this network.
     *
     * @param match value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapAltSubjectMatch(in String match);

    /**
     * Set EAP Anonymous Identity for this network.
     *
     * @param identity value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapAnonymousIdentity(in byte[] identity);

    /**
     * Set EAP CA certificate file path for this network.
     *
     * @param path value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapCACert(in String path);

    /**
     * Set EAP CA certificate directory path for this network.
     *
     * @param path value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapCAPath(in String path);

    /**
     * Set EAP Client certificate file path for this network.
     *
     * @param path value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapClientCert(in String path);

    /**
     * Set EAP Domain suffix match for this network.
     *
     * @param match value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapDomainSuffixMatch(in String match);

    /**
     * Set EAP encrypted IMSI Identity for this network.
     *
     * @param identity Identity string built from the encrypted IMSI.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapEncryptedImsiIdentity(in byte[] identity);

    /**
     * Enable EAP Open SSL Engine for this network.
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapEngine(in boolean enable);

    /**
     * Set EAP Open SSL Engine ID for this network.
     *
     * @param id value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapEngineID(in String id);

    /**
     * Enable Extensible Authentication (EAP) - Re-authentication Protocol (ERP) for this network.
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapErp(in boolean enable);

    /**
     * Set EAP Identity for this network.
     *
     * @param identity value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapIdentity(in byte[] identity);

    /**
     * Set EAP Method for this network.
     *
     * @param method value to be set.
     *        Must be one of |EapMethod| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapMethod(in EapMethod method);

    /**
     * Set EAP Password for this network.
     *
     * @param password value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapPassword(in byte[] password);

    /**
     * Set EAP Phase2 Method for this network.
     *
     * EAP method needs to be set for this to work.
     *
     * @param method value to set.
     *        Must be one of |EapPhase2Method| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapPhase2Method(in EapPhase2Method method);

    /**
     * Set EAP private key Id for this network.
     * This is used if private key operations for EAP-TLS are performed
     * using a smartcard.
     *
     * @param id value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapPrivateKeyId(in String id);

    /**
     * Set EAP subject match for this network.
     *
     * @param match value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEapSubjectMatch(in String match);

    /**
     * Set whether to enable enhanced directional multi-gigabit (802.11ay EDMG).
     * Only allowed if hw mode is |HOSTAPD_MODE_IEEE80211AD|
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEdmg(in boolean enable);

    /**
     * Set group cipher mask for the network.
     *
     * @param groupCipherMask value to set.
     *        Combination of |ProtoMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setGroupCipher(in GroupCipherMask groupCipherMask);

    /**
     * Set group management cipher mask for the network.
     *
     * @param groupMgmtCipherMask value to set.
     *        Combination of |GroupMgmtCipherMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setGroupMgmtCipher(in GroupMgmtCipherMask groupMgmtCipherMask);

    /**
     * Set ID string for this network.
     * Network identifier string for external scripts.
     *
     * @param idStr ID string value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setIdStr(in String idStr);

    /**
     * Set key management mask for the network.
     *
     * @param keyMgmtMask value to set.
     *        Combination of |KeyMgmtMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setKeyMgmt(in KeyMgmtMask keyMgmtMask);

    /**
     * Set OCSP (Online Certificate Status Protocol) type for this network.
     *
     * @param ocspType value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setOcsp(in OcspType ocspType);

    /**
     * Set pairwise cipher mask for the network.
     *
     * @param pairwiseCipherMask value to set.
     *        Combination of |ProtoMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setPairwiseCipher(in PairwiseCipherMask pairwiseCipherMask);

    /**
     * Add a pairwise master key (PMK) into supplicant PMK cache.
     *
     * @param serializedEntry is serialized PMK cache entry, the content is
     *              opaque for the framework and depends on the native implementation.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setPmkCache(in byte[] serializedEntry);

    /**
     * This field can be used to enable proactive key caching which is also
     * known as opportunistic PMKSA caching for WPA2. This is disabled (0)
     * by default unless default value is changed with the global okc=1
     * parameter.
     *
     * Proactive key caching is used to make supplicant assume that the APs
     * are using the same PMK and generate PMKSA cache entries without
     * doing RSN pre-authentication. This requires support from the AP side
     * and is normally used with wireless switches that co-locate the
     * authenticator.
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setProactiveKeyCaching(in boolean enable);

    /**
     * Set proto mask for the network.
     *
     * @param protoMask value to set.
     *        Combination of |ProtoMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setProto(in ProtoMask protoMask);

    /**
     * Set raw psk for WPA_PSK network.
     *
     * @param psk value to set as specified in IEEE 802.11i-2004 standard.
     *        This is the calculated using 'wpa_passphrase <ssid> [passphrase]'
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setPsk(in byte[] psk);

    /**
     * Set passphrase for WPA_PSK network.
     *
     * @param psk value to set.
     *        Length of value must be between
     *        |PSK_PASSPHRASE_MIN_LEN_IN_BYTES| and
     *        |PSK_PASSPHRASE_MAX_LEN_IN_BYTES|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setPskPassphrase(in String psk);

    /**
     * Set whether RequirePmf is enabled for this network.
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setRequirePmf(in boolean enable);

    /**
     * Set SAE H2E (Hash-to-Element) mode.
     *
     * @param mode SAE H2E supporting mode.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setSaeH2eMode(in SaeH2eMode mode);

    /**
     * Set SAE password for WPA3-Personal
     *
     * @param saePassword string with the above option
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setSaePassword(in String saePassword);

    /**
     * Set SAE password ID for WPA3-Personal
     *
     * @param sae_password_id string with the above option
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setSaePasswordId(in String saePasswordId);

    /**
     * Set whether to send probe requests for this network (hidden).
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setScanSsid(in boolean enable);

    /**
     *
     * Setters for the various network params.
     * These correspond to elements of |wpa_sssid| struct used internally by
     * the supplicant to represent each network.
     *
     */

    /**
     * Set SSID for this network.
     *
     * @param ssid value to set.
     *        Max length of |SSID_MAX_LEN_IN_BYTES|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setSsid(in byte[] ssid);

    /**
     * Set PPS MO ID for this network.
     * (Hotspot 2.0 PerProviderSubscription/UpdateIdentifier)
     *
     * @param id ID value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setUpdateIdentifier(in int id);

    /**
     * Set WAPI certificate suite name for this network.
     *
     * @param suite value to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWapiCertSuite(in String suite);

    /**
     * Set WEP key for WEP network.
     *
     * @param keyIdx Index of wep key to set.
     *        Max of |WEP_KEYS_MAX_NUM|.
     * @param wepKey value to set.
     *        Length of each key must be either
     *        |WEP40_KEY_LEN_IN_BYTES| or
     *        |WEP104_KEY_LEN_IN_BYTES|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setWepKey(in int keyIdx, in byte[] wepKey);

    /**
     * Set default Tx key index for WEP network.
     *
     * @param keyIdx Value to set.
     *        Max of |WEP_KEYS_MAX_NUM|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setWepTxKeyIdx(in int keyIdx);

    /**
     * Set the roaming consortium selection.
     *
     * @param selectedRcoi Indicates the roaming consortium selection. This is a
     *            3 or 5-octet long byte array that indicates the selected RCOI
     *            used for a Passpoint connection.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setRoamingConsortiumSelection(in byte[] selectedRcoi);

    /**
     * Set the minimum TLS version for EAP phase1 param.
     *
     * @param tlsVersion the TLS version
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setMinimumTlsVersionEapPhase1Param(TlsVersion tlsVersion);

    /**
     * Enable the strict conservative peer mode for EAP-SIM/AKA/AKA'
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setStrictConservativePeerMode(in boolean enable);

    /**
     * Disables Extremely High Throughput (EHT) mode, aka Wi-Fi 7 support, for the network. When
     * EHT is disabled, the device ceases to transmit EHT related Information Elements (IEs),
     * including multi-link IEs and EHT capability, in subsequent messages such as (Re)Association
     * requests to the Access Point (AP).
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void disableEht();

    /**
     * Set additional vendor-provided configuration data.
     *
     * @param vendorData List of |OuiKeyedData| containing the vendor-provided
     *         configuration data.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setVendorData(in OuiKeyedData[] vendorData);
}
