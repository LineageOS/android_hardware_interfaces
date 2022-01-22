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

package android.hardware.nfc;

import android.hardware.nfc.PresenceCheckAlgorithm;
import android.hardware.nfc.ProtocolDiscoveryConfig;

/**
 * Define Nfc related configurations based on:
 * NFC Controller Interface (NCI) Technical Specification
 * https://nfc-forum.org/our-work/specification-releases/
 */
@VintfStability
parcelable NfcConfig {
    /**
     * If true, NFCC is using bail out mode for either Type A or Type B poll
     * based on: Nfc-Forum Activity Technical Specification
     * https://nfc-forum.org/our-work/specification-releases/
     */
    boolean nfaPollBailOutMode;
    PresenceCheckAlgorithm presenceCheckAlgorithm;
    ProtocolDiscoveryConfig nfaProprietaryCfg;
    /**
     * Default off-host route. 0x00 if there aren't any. Refer to NCI spec.
     */
    byte defaultOffHostRoute;
    /**
     * Default off-host route for Felica. 0x00 if there aren't any. Refer to
     * NCI spec.
     */
    byte defaultOffHostRouteFelica;
    /**
     * Default system code route. 0x00 if there aren't any. Refer NCI spec.
     */
    byte defaultSystemCodeRoute;
    /**
     * Default power state for system code route. 0x00 if there aren't any.
     * Refer to NCI spec.
     */
    byte defaultSystemCodePowerState;
    /**
     * Default route for all remaining protocols and technology which haven't
     * been configured.
     * Device Host(0x00) is the default. Refer to NCI spec.
     *
     */
    byte defaultRoute;
    /**
     * Pipe ID for eSE. 0x00 if there aren't any.
     */
    byte offHostESEPipeId;
    /**
     * Pipe ID for UICC. 0x00 if there aren't any.
     */
    byte offHostSIMPipeId;
    /**
     * Extended APDU length for ISO_DEP. If not supported default length is 261
     */
    int maxIsoDepTransceiveLength;
    /**
     * list of allowed host ids, as per ETSI TS 102 622
     * https://www.etsi.org/
     */
    byte[] hostAllowlist;
    /**
     * NFCEE ID for offhost UICC & eSE secure element.
     * 0x00 if there aren't any. Refer to NCI spec.
     */
    byte[] offHostRouteUicc;
    byte[] offHostRouteEse;
    /**
     * Default IsoDep route. 0x00 if there aren't any. Refer to NCI spec.
     */
    byte defaultIsoDepRoute;
}
