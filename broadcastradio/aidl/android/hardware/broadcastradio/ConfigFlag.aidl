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

package android.hardware.broadcastradio;

/**
 * Configuration flags to be used with isConfigFlagSet and setConfigFlag methods
 * of IBroadcastRadio.
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum ConfigFlag {
    /**
     * Forces mono audio stream reception.
     *
     * Analog broadcasts can recover poor reception conditions by jointing
     * stereo channels into one. Mainly for, but not limited to AM/FM.
     */
    FORCE_MONO = 1,

    /**
     * Forces the analog playback for the supporting radio technology.
     *
     * User may disable digital playback for FM HD Radio or hybrid FM/DAB with
     * this option. This is purely user choice, i.e. does not reflect digital-
     * analog handover state managed from the HAL implementation side.
     *
     * Some radio technologies may not support this, i.e. DAB.
     *
     * @deprecated Use {link #FORCE_ANALOG_FM} instead
     */
    FORCE_ANALOG,

    /**
     * Forces the digital playback for the supporting radio technology.
     *
     * User may disable digital-analog handover that happens with poor
     * reception conditions. With digital forced, the radio will remain silent
     * instead of switching to analog channel if it's available. This is purely
     * user choice, it does not reflect the actual state of handover.
     */
    FORCE_DIGITAL,

    /**
     * RDS Alternative Frequencies.
     *
     * If set and the currently tuned RDS station broadcasts on multiple
     * channels, radio tuner automatically switches to the best available
     * alternative.
     */
    RDS_AF,

    /**
     * RDS region-specific program lock-down.
     *
     * Allows user to lock to the current region as they move into the
     * other region.
     */
    RDS_REG,

    /**
     * Enables DAB-DAB hard- and implicit-linking (the same content).
     */
    DAB_DAB_LINKING,

    /**
     * Enables DAB-FM hard- and implicit-linking (the same content).
     */
    DAB_FM_LINKING,

    /**
     * Enables DAB-DAB soft-linking (related content).
     */
    DAB_DAB_SOFT_LINKING,

    /**
     * Enables DAB-FM soft-linking (related content).
     */
    DAB_FM_SOFT_LINKING,

    /**
     * Forces the FM analog playback for the supporting radio technology.
     *
     * User may disable FM digital playback for FM HD Radio or hybrid FM/DAB
     * with this option. This is purely user choice, i.e. does not reflect
     * digital-analog handover state managed from the HAL implementation side.
     *
     * Some radio technologies may not support this, i.e. DAB.
     */
    FORCE_ANALOG_FM,

    /**
     * Forces the AM analog playback for the supporting radio technology.
     *
     * User may disable AM digital playback for AM HD Radio or hybrid AM/DAB
     * with this option. This is purely user choice, i.e. does not reflect
     * digital-analog handover state managed from the HAL implementation side.
     *
     * Some radio technologies may not support this, i.e. DAB.
     */
    FORCE_ANALOG_AM,
}
