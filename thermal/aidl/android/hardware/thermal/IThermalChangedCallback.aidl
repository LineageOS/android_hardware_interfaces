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

package android.hardware.thermal;

import android.hardware.thermal.Temperature;

/**
 * IThermalChangedCallback send throttling notification to clients.
 * @hide
 */
@VintfStability
interface IThermalChangedCallback {
    /**
     * Send a thermal throttling event to all ThermalHAL
     * thermal event listeners.
     *
     * @param temperature The temperature associated with the
     *    throttling event.
     */
    oneway void notifyThrottling(in Temperature temperature);
}
