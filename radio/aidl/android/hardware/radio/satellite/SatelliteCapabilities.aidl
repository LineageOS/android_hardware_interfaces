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

package android.hardware.radio.satellite;

import android.hardware.radio.satellite.NTRadioTechnology;
import android.hardware.radio.satellite.SatelliteFeature;

@VintfStability
@JavaDerive(toString=true)
parcelable SatelliteCapabilities {
    /**
     * List of technologies supported by the satellite modem.
     */
    NTRadioTechnology[] supportedRadioTechnologies;

    /**
     * Whether satellite mode is always on (this indicates the power impact of keeping it on is
     * very minimal).
     */
    boolean isAlwaysOn;

    /**
     * Whether UE needs to point to a satellite to send and receive data.
     */
    boolean needsPointingToSatellite;

    /**
     * List of features supported by the satellite modem.
     */
    SatelliteFeature[] supportedFeatures;

    /**
     * Whether UE needs a separate SIM profile to communicate with satellite network.
     */
    boolean needsSeparateSimProfile;
}
