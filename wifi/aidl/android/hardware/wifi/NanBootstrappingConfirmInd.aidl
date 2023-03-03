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

import android.hardware.wifi.NanBootstrappingResponseCode;
import android.hardware.wifi.NanStatus;

/**
 * See Wi-Fi Aware R4.0 section 9.5.21.7
 */
@VintfStability
parcelable NanBootstrappingConfirmInd {
    /**
     * Id of the bootstrapping session. Obtained as part of earlier
     |IWifiNanIface.initiateBootstrappingRequest| success notification.
     */
    int bootstrappingInstanceId;

    /**
     * Indicate whether the bootstrapping method negotiation accept or not
     */
    NanBootstrappingResponseCode responseCode;

    /**
     * Failure reason if |acceptRequest| is false.
     */
    NanStatus reasonCode;

    /**
     * The delay of bootstrapping in seconds for the follow up request.
     */
    int comeBackDelay;

    /**
     * Cookie received from peer with |comeBackDelay| for follow up |NanBootstrappingRequest|
     */
    byte[] cookie;
}
