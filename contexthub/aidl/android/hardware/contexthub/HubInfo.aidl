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

package android.hardware.contexthub;

import android.hardware.contexthub.ContextHubInfo;
import android.hardware.contexthub.VendorHubInfo;

@VintfStability
parcelable HubInfo {
    /**
     * Invalid hub ID.
     */
    const long HUB_ID_INVALID = 0;

    /**
     * Reserved hub ID.
     */
    const long HUB_ID_RESERVED = -1;

    /**
     * Hub ID (depending on type). This is a globally unique identifier.
     *
     * HUB_ID_INVALID(0) is an invalid id and should never be used.
     * HUB_ID_RESERVED(-1) is reserved for future use.
     */
    long hubId;

    /**
     * A hub can be either a ContextHub or a VendorHub.
     */
    union HubDetails {
        ContextHubInfo contextHubInfo;
        VendorHubInfo vendorHubInfo;
    }

    /**
     * Detail information about the hub.
     */
    HubDetails hubDetails;
}
