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
 * DPP bootstrap info generated for responder mode operation
 */
@VintfStability
parcelable DppResponderBootstrapInfo {
    /**
     * Generated bootstrap identifier
     */
    int bootstrapId;
    /**
     * The Wi-Fi channel that the DPP responder is listening on.
     */
    int listenChannel;
    /**
     * Bootstrapping URI per DPP specification, "section 5.2 Bootstrapping
     * information", may contain listen channel, MAC address, public key, or other information.
     */
    String uri;
}
