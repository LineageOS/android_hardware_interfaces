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

/**
 * Parameters for configuring a USD subscribe session.
 */
@VintfStability
parcelable UsdSubscribeConfig {
    /**
     * Subscribe modes that this session can be configured in.
     */
    enum SubscribeType {
        /**
         * Subscribe function does not request transmission of any Subscribe messages, but checks
         * for matches in received Publish messages.
         */
        PASSIVE_MODE = 0,
        /**
         * Subscribe function additionally requests transmission of Subscribe messages and processes
         * Publish messages.
         */
        ACTIVE_MODE = 1,
    }

    /**
     * Base USD session parameters.
     */
    UsdBaseConfig usdBaseConfig;

    /**
     * Subscribe mode that this session should be configured in.
     */
    SubscribeType subscribeType;

    /**
     * Recommended periodicity (in milliseconds) of query transmissions for the session.
     */
    int queryPeriodMillis;
}
