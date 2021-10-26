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

/**
 * Various states of the interface reported by |onStateChanged|.
 */
@VintfStability
@Backing(type="int")
enum StaIfaceCallbackState {
    /**
     * This state indicates that client is not associated, but is likely to
     * start looking for an access point. This state is entered when a
     * connection is lost.
     */
    DISCONNECTED = 0,
    /**
     * This state is entered if the network interface is disabled, e.g.,
     * due to rfkill. the supplicant refuses any new operations that would
     * use the radio until the interface has been enabled.
     */
    IFACE_DISABLED = 1,
    /**
     * This state is entered if there are no enabled networks in the
     * configuration. the supplicant is not trying to associate with a new
     * network and external interaction (e.g., ctrl_iface call to add or
     * enable a network) is needed to start association.
     */
    INACTIVE = 2,
    /**
     * This state is entered when the supplicant starts scanning for a
     * network.
     */
    SCANNING = 3,
    /**
     * This state is entered when the supplicant has found a suitable BSS
     * to authenticate with and the driver is configured to try to
     * authenticate with this BSS. This state is used only with drivers
     * that use the supplicant as the SME.
     */
    AUTHENTICATING = 4,
    /**
     * This state is entered when the supplicant has found a suitable BSS
     * to associate with and the driver is configured to try to associate
     * with this BSS in ap_scan=1 mode. When using ap_scan=2 mode, this
     * state is entered when the driver is configured to try to associate
     * with a network using the configured SSID and security policy.
     */
    ASSOCIATING = 5,
    /**
     * This state is entered when the driver reports that association has
     * been successfully completed with an AP. If IEEE 802.1X is used
     * (with or without WPA/WPA2), the supplicant remains in this state
     * until the IEEE 802.1X/EAPOL authentication has been completed.
     */
    ASSOCIATED = 6,
    /**
     * This state is entered when WPA/WPA2 4-Way Handshake is started. In
     * case of WPA-PSK, this happens when receiving the first EAPOL-Key
     * frame after association. In case of WPA-EAP, this state is entered
     * when the IEEE 802.1X/EAPOL authentication has been completed.
     */
    FOURWAY_HANDSHAKE = 7,
    /**
     * This state is entered when 4-Way Key Handshake has been completed
     * (i.e., when the supplicant sends out message 4/4) and when Group
     * Key rekeying is started by the AP (i.e., when supplicant receives
     * message 1/2).
     */
    GROUP_HANDSHAKE = 8,
    /**
     * This state is entered when the full authentication process is
     * completed. In case of WPA2, this happens when the 4-Way Handshake is
     * successfully completed. With WPA, this state is entered after the
     * Group Key Handshake; with IEEE 802.1X (non-WPA) connection is
     * completed after dynamic keys are received (or if not used, after
     * the EAP authentication has been completed). With static WEP keys and
     * plaintext connections, this state is entered when an association
     * has been completed.
     *
     * This state indicates that the supplicant has completed its
     * processing for the association phase and that data connection is
     * fully configured.
     */
    COMPLETED = 9,
}
