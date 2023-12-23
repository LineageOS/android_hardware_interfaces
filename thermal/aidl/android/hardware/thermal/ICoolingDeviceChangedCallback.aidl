/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.thermal.CoolingDevice;
import android.hardware.thermal.Temperature;

/**
 * ICoolingDeviceChangedCallback send cooling device change notification to clients.
 * @hide
 */
@VintfStability
interface ICoolingDeviceChangedCallback {
    /**
     * Send a cooling device change event to all ThermalHAL
     * cooling device event listeners.
     *
     * @param cooling_device The cooling device information associated with the
     *     change event.
     */
    oneway void notifyCoolingDeviceChanged(in CoolingDevice coolingDevice);
}
