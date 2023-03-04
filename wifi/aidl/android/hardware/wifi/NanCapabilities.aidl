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

/**
 * NDP Capabilities response.
 */
@VintfStability
parcelable NanCapabilities {
    /**
     * Maximum number of clusters which the device can join concurrently.
     */
    int maxConcurrentClusters;
    /**
     * Maximum number of concurrent publish discovery sessions.
     */
    int maxPublishes;
    /**
     * Maximum number of concurrent subscribe discovery sessions.
     */
    int maxSubscribes;
    /**
     * Maximum length (in bytes) of service name.
     */
    int maxServiceNameLen;
    /**
     * Maximum length (in bytes) of individual match filters.
     */
    int maxMatchFilterLen;
    /**
     * Maximum length (in bytes) of aggregate match filters across all active sessions.
     */
    int maxTotalMatchFilterLen;
    /**
     * Maximum length (in bytes) of the service specific info field.
     */
    int maxServiceSpecificInfoLen;
    /**
     * Maximum length (in bytes) of the extended service specific info field.
     */
    int maxExtendedServiceSpecificInfoLen;
    /**
     * Maximum number of data interfaces (NDI) which can be created concurrently on the device.
     */
    int maxNdiInterfaces;
    /**
     * Maximum number of data paths (NDP) which can be created concurrently on the device, across
     * all data interfaces (NDI).
     */
    int maxNdpSessions;
    /**
     * Maximum length (in bytes) of application info field (used in data-path negotiations).
     */
    int maxAppInfoLen;
    /**
     * Maximum number of transmitted followup messages which can be queued by the firmware.
     */
    int maxQueuedTransmitFollowupMsgs;
    /**
     * Maximum number MAC interface addresses which can be specified to a subscribe discovery
     * session.
     */
    int maxSubscribeInterfaceAddresses;
    /**
     * Bitmap of |NanCipherSuiteType| values indicating the set of supported cipher suites.
     */
    int supportedCipherSuites;
    /**
     * Flag to indicate if instant communication mode is supported.
     */
    boolean instantCommunicationModeSupportFlag;
    /**
     * Flag to indicate if 6 GHz is supported.
     */
    boolean supports6g;
    /**
     * Flag to indicate if High Efficiency is supported.
     */
    boolean supportsHe;
    /**
     * Flag to indicate if NAN pairing is supported.
     */
    boolean supportsPairing;
    /**
     * Flag to indicate if setting NAN cluster ID is supported.
     */
    boolean supportsSetClusterId;
    /**
     * Flag to indicate if NAN suspension is supported.
     */
    boolean supportsSuspension;
}
