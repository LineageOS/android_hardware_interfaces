/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.bluetooth.finder;

import android.hardware.bluetooth.finder.Eid;

@VintfStability
interface IBluetoothFinder {
    /**
     * API to set the EIDs to the Bluetooth Controller
     *
     * @param eids array of 20 bytes EID to the Bluetooth
     * controller
     */
    void sendEids(in Eid[] eids);

    /**
     * API to enable the powered-off finder feature, which allows the Bluetooth controller to send
     * beacons after the device is powered off.
     *
     * @param enable true to enable; false to disable
     */
    void setPoweredOffFinderMode(in boolean enable);

    /**
     * API for retrieving feature enablement status
     *
     * @return the value last set by setPoweredOffFinderMode, false if setPoweredOffFinderMode was
     *         never been invoked since boot.
     */
    boolean getPoweredOffFinderMode();
}
