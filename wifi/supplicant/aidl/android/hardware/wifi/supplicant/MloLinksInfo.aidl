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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.MloLink;

/**
 * Multi-Link Operation (MLO) Links info.
 * The information for MLO links needed by 802.11be standard.
 */
@VintfStability
parcelable MloLinksInfo {
    /**
     * List of MLO links
     */
    MloLink[] links;
    /**
     * The MLO link-id for the access point. It is the link-id used for association.
     */
    int apMloLinkId;
    /**
     * AP MLD MAC address.
     */
    @nullable byte[6] apMldMacAddress;
}
