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

package android.hardware.automotive.ivn;

import android.hardware.automotive.ivn.EndpointInfo;
import android.hardware.automotive.ivn.OccupantZoneInfo;

/**
 * Interface for In-Vehicle Network(IVN) Android devices.
 *
 * This is used in a multi-zone multi-SoC environment where there are multiple
 * SoCs running Android in a vehicle. Each support one or multiple occupant
 * zones. E.g., one SoC for front passenger, one SoC for backseat left-zone
 * and middle/right zone passengers.
 */
@VintfStability
interface IIvnAndroidDevice {
    /**
     * Returns the unique ID for this Android device.
     *
     * <p>This ID has to be unique among all the android devices in the whole vehicle. It is usually
     * a hard-coded value, e.g. serial number.
     *
     * @return an ID representing this device.
     */
    int getMyDeviceId();

    /**
     * Returns a list of unique IDs for other IVN Android devices.
     *
     * The returned list does not contain the current Android device ID. This list is usually
     * pre-configured for this HAL, either hard-coded or read from configuration file.
     *
     * @return A list of IDs representing connected Android devices.
     */
    int[] getOtherDeviceIds();

    /**
     * Returns the Android device ID for a specified occupant zone.
     *
     * @pararm zoneID the occupant zone ID returned from {@link android.car.CarOccupantZoneManager}.
     * @return an ID representing an Android device.
     */
    int getDeviceIdForOccupantZone(int zoneId);

    /**
     * Returns all the occupant zones supported for a specified IVN Android device.
     *
     * @param androidDeviceId the android device ID.
     * @return A list of supported occupant zone info.
     */
    OccupantZoneInfo[] getOccupantZonesForDevice(int androidDeviceId);

    /**
     * Returns the connection endpoint info for this android device.
     *
     * @return The endpoint info.
     */
    EndpointInfo getMyEndpointInfo();

    /**
     * Returns the connection endpoint info for the specified IVN Android device.
     *
     * @return The endpoint info.
     */
    EndpointInfo getEndpointInfoForDevice(int androidDeviceId);
}
