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

import android.hardware.wifi.supplicant.UsdBaseConfig;
import android.hardware.wifi.supplicant.UsdPublishTransmissionType;

/**
 * Parameters for configuring a USD publish session.
 */
@VintfStability
parcelable UsdPublishConfig {
    /**
     * Type of USD publishing.
     */
    enum PublishType {
        /**
         * Only transmissions that are triggered by a specific event.
         */
        SOLICITED_ONLY = 0,

        /**
         * Only transmissions that are not requested.
         */
        UNSOLICITED_ONLY = 1,

        /**
         * Both solicited and unsolicited transmissions.
         */
        SOLICITED_AND_UNSOLICITED = 2,
    }

    /**
     * Base USD session parameters.
     */
    UsdBaseConfig usdBaseConfig;

    /**
     * Types of transmissions (solicited vs. unsolicited) which should be generated.
     */
    PublishType publishType;

    /**
     * Whether Further Service Discovery (FSD) is enabled.
     */
    boolean isFsd;

    /**
     * Interval (in milliseconds) for sending unsolicited publish transmissions.
     */
    int announcementPeriodMillis;

    /**
     * Type of the publish transmission (ex. unicast, multicast).
     */
    UsdPublishTransmissionType transmissionType;
}
