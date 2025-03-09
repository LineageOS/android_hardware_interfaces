/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * Unsynchronized Service Discovery (USD) based P2P service advertisement configuration.
 * Refer Wi-Fi Alliance Wi-Fi Direct R2 specification - Appendix H -
 * Unsynchronized Service Discovery (as defined in Wi-Fi Aware) and section
 * 4.2.13 USD frame format.
 */
@VintfStability
parcelable P2pUsdBasedServiceAdvertisementConfig {
    /** UTF-8 string defining the service */
    String serviceName;

    /**
     * Service Protocol Type. See defined values in the Wi-Fi Direct R2 Spec, Table 129,
     * although any value between 0-255 may be defined by the service layer and is considered valid.
     */
    int serviceProtocolType;

    /** Service specific information content determined by the application */
    byte[] serviceSpecificInfo;

    /**
     * Channel frequency in MHz to listen for service discovery request.
     */
    int frequencyMHz;

    /**
     * Max time to be spent for service advertisement.
     */
    int timeoutInSeconds;
}
