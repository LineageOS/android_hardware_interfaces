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

package android.hardware.contexthub;

@VintfStability
parcelable ContextHubInfo {
    /** Descriptive name of the Context Hub */
    String name;

    /** The vendor e.g. "Google" */
    String vendor;

    /** Toolchain that describes the binary architecture eg: "gcc ARM" */
    String toolchain;

    /** A unique ID for this Context Hub */
    int id;

    /** Peak MIPs this platform can deliver */
    float peakMips;

    /**
     * The maximum length in bytes of a message sent to the Context Hub.
     */
    int maxSupportedMessageLengthBytes;

    /**
     * Machine-readable CHRE platform ID, returned to nanoapps in the CHRE API
     * function call chreGetPlatformId(). This field pairs with
     * chreApiMajorVersion, chreApiMinorVersion, and chrePatchVersion to fully
     * specify the CHRE implementation version. See also the CHRE API header
     * file chre/version.h.
     */
    long chrePlatformId;

    /**
     * The version of the CHRE implementation returned to nanoApps in the CHRE
     *  API function call chreGetVersion(). The major and minor version specify
     * the implemented version of the CHRE API, while the patch version
     * describes the implementation version within the scope of the platform
     * ID. See also the CHRE API header file chre/version.h.
     */
    byte chreApiMajorVersion;
    byte chreApiMinorVersion;
    char chrePatchVersion;

    /**
     * A list of Android permissions this Context Hub support for nanoapps to enforce host endpoints
     * are granted in order to communicate with them.
     */
    String[] supportedPermissions;

    /**
     * True if the Context Hub supports reliable messages. False otherwise, in which case
     * ContextHubMessage.isReliable must always be set to false. See
     * ContextHubMessage.isReliable for more information.
     */
    boolean supportsReliableMessages;
}
