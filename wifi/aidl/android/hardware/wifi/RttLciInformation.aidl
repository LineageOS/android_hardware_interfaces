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

import android.hardware.wifi.RttMotionPattern;

/**
 * Movement pattern unknown.
 */
@VintfStability
parcelable RttLciInformation {
    /**
     * Latitude in degrees * 2^25 , 2's complement.
     */
    long latitude;
    /**
     * Longitude in degrees * 2^25 , 2's complement.
     */
    long longitude;
    /**
     * Altitude in units of 1/256 m.
     */
    int altitude;
    /**
     * As defined in Section 2.3.2 of IETF RFC 6225.
     */
    byte latitudeUnc;
    /**
     * As defined in Section 2.3.2 of IETF RFC 6225.
     */
    byte longitudeUnc;
    /**
     * As defined in Section 2.4.5 from IETF RFC 6225.
     */
    byte altitudeUnc;
    /**
     * The following elements are for configuring the Z subelement.
     *
     *
     * Motion pattern type.
     */
    RttMotionPattern motionPattern;
    /**
     * Floor in units of 1/16th of floor. 0x80000000 if unknown.
     */
    int floor;
    /**
     * In units of 1/64 m.
     */
    int heightAboveFloor;
    /**
     * In units of 1/64 m. 0 if unknown.
     */
    int heightUnc;
}
