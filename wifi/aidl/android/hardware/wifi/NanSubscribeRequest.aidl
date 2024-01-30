/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.wifi;

import android.hardware.wifi.MacAddress;
import android.hardware.wifi.NanDiscoveryCommonConfig;
import android.hardware.wifi.NanPairingConfig;
import android.hardware.wifi.NanSrfType;
import android.hardware.wifi.NanSubscribeType;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * Subscribe request. Specifies a subscribe discovery operation.
 */
@VintfStability
parcelable NanSubscribeRequest {
    /**
     * Common configuration of discovery sessions.
     */
    NanDiscoveryCommonConfig baseConfigs;
    /**
     * The type of the subscribe discovery session.
     */
    NanSubscribeType subscribeType;
    /**
     * For |NanSubscribeType.ACTIVE| subscribe discovery sessions, specifies how the Service
     * Response Filter (SRF) attribute is populated. Relevant only if |shouldUseSrf| is set to true.
     * NAN Spec: Service Descriptor Attribute (SDA) / Service Response Filter / SRF Control / SRF
     * Type
     */
    NanSrfType srfType;
    /**
     * Configure whether inclusion of an address in |intfAddr| indicates that those devices should
     * respond or the reverse. Relevant only if |shouldUseSrf| is set to true and |srfType| is set
     * to |NanSrfType.PARTIAL_MAC_ADDR|. NAN Spec: Service Descriptor Attribute (SDA) / Service
     * Response Filter / SRF Control / Include
     */
    boolean srfRespondIfInAddressSet;
    /**
     * Control whether the Service Response Filter (SRF) is used.
     * NAN Spec: Service Descriptor Attribute (SDA) / Service Control /
     *           Service Response Filter Present
     */
    boolean shouldUseSrf;
    /**
     * Control whether the presence of |NanDiscoveryCommonConfig.serviceSpecificInfo| data is needed
     * in the publisher in order to trigger service discovery, i.e. a
     * |IWifiNanIfaceEventCallback.eventMatch|. The test is for presence of data - not for the
     * specific contents of the data.
     */
    boolean isSsiRequiredForMatch;
    /**
     * NAN Interface Addresses constituting the Service Response Filter (SRF).
     * Max length (number of addresses): |NanCapabilities.maxSubscribeInterfaceAddresses|.
     * NAN Spec: Service Descriptor Attribute (SDA) / Service Response Filter / Address Set
     */
    MacAddress[] intfAddr;
    /**
     * Security config used for the pairing
     */
    NanPairingConfig pairingConfig;
    /**
     * The Identity key for pairing, will generate NIRA for verification by the peer
     */
    byte[16] identityKey;
    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
