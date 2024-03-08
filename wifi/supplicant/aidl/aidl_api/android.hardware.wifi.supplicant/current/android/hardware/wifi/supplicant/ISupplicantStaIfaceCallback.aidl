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
interface ISupplicantStaIfaceCallback {
  oneway void onAnqpQueryDone(in byte[] bssid, in android.hardware.wifi.supplicant.AnqpData data, in android.hardware.wifi.supplicant.Hs20AnqpData hs20Data);
  oneway void onAssociationRejected(in android.hardware.wifi.supplicant.AssociationRejectionData assocRejectData);
  oneway void onAuthenticationTimeout(in byte[] bssid);
  oneway void onAuxiliarySupplicantEvent(in android.hardware.wifi.supplicant.AuxiliarySupplicantEventCode eventCode, in byte[] bssid, in String reasonString);
  oneway void onBssTmHandlingDone(in android.hardware.wifi.supplicant.BssTmData tmData);
  oneway void onBssidChanged(in android.hardware.wifi.supplicant.BssidChangeReason reason, in byte[] bssid);
  oneway void onDisconnected(in byte[] bssid, in boolean locallyGenerated, in android.hardware.wifi.supplicant.StaIfaceReasonCode reasonCode);
  oneway void onDppFailure(in android.hardware.wifi.supplicant.DppFailureCode code, in String ssid, in String channelList, in char[] bandList);
  oneway void onDppProgress(in android.hardware.wifi.supplicant.DppProgressCode code);
  oneway void onDppSuccess(in android.hardware.wifi.supplicant.DppEventType event);
  /**
   * @deprecated This callback is deprecated from AIDL v2, newer HAL should call onDppConfigReceived.
   */
  oneway void onDppSuccessConfigReceived(in byte[] ssid, in String password, in byte[] psk, in android.hardware.wifi.supplicant.DppAkm securityAkm, in android.hardware.wifi.supplicant.DppConnectionKeys dppConnectionKeys);
  oneway void onDppSuccessConfigSent();
  oneway void onEapFailure(in byte[] bssid, in int errorCode);
  oneway void onExtRadioWorkStart(in int id);
  oneway void onExtRadioWorkTimeout(in int id);
  oneway void onHs20DeauthImminentNotice(in byte[] bssid, in int reasonCode, in int reAuthDelayInSec, in String url);
  /**
   * @deprecated No longer in use.
   */
  oneway void onHs20IconQueryDone(in byte[] bssid, in String fileName, in byte[] data);
  oneway void onHs20SubscriptionRemediation(in byte[] bssid, in android.hardware.wifi.supplicant.OsuMethod osuMethod, in String url);
  oneway void onHs20TermsAndConditionsAcceptanceRequestedNotification(in byte[] bssid, in String url);
  oneway void onNetworkAdded(in int id);
  oneway void onNetworkNotFound(in byte[] ssid);
  oneway void onNetworkRemoved(in int id);
  /**
   * @deprecated use onPmkSaCacheAdded() instead.
   */
  oneway void onPmkCacheAdded(in long expirationTimeInSec, in byte[] serializedEntry);
  /**
   * @deprecated This callback is deprecated from AIDL v2, newer HAL should call onSupplicantStateChanged()
   */
  oneway void onStateChanged(in android.hardware.wifi.supplicant.StaIfaceCallbackState newState, in byte[] bssid, in int id, in byte[] ssid, in boolean filsHlpSent);
  oneway void onWpsEventFail(in byte[] bssid, in android.hardware.wifi.supplicant.WpsConfigError configError, in android.hardware.wifi.supplicant.WpsErrorIndication errorInd);
  oneway void onWpsEventPbcOverlap();
  oneway void onWpsEventSuccess();
  oneway void onQosPolicyReset();
  oneway void onQosPolicyRequest(in int qosPolicyRequestId, in android.hardware.wifi.supplicant.QosPolicyData[] qosPolicyData);
  oneway void onMloLinksInfoChanged(in android.hardware.wifi.supplicant.ISupplicantStaIfaceCallback.MloLinkInfoChangeReason reason);
  oneway void onDppConfigReceived(in android.hardware.wifi.supplicant.DppConfigurationData configData);
  oneway void onDppConnectionStatusResultSent(in android.hardware.wifi.supplicant.DppStatusErrorCode code);
  oneway void onBssFrequencyChanged(in int frequencyMhz);
  oneway void onSupplicantStateChanged(in android.hardware.wifi.supplicant.SupplicantStateChangeData stateChangeData);
  oneway void onQosPolicyResponseForScs(in android.hardware.wifi.supplicant.QosPolicyScsResponseStatus[] qosPolicyScsResponseStatus);
  oneway void onPmkSaCacheAdded(in android.hardware.wifi.supplicant.PmkSaCacheData pmkSaData);
  @Backing(type="int") @VintfStability
  enum MloLinkInfoChangeReason {
    TID_TO_LINK_MAP = 0,
    MULTI_LINK_RECONFIG_AP_REMOVAL = 1,
  }
}
