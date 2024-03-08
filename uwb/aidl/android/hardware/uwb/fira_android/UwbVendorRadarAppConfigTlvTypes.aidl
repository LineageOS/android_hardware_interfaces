/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb.fira_android;

/**
 * Android specific radar app params set/expected in UCI command:
 * GID: 1100b (Android specific Group)
 * OID: 010001b (RADAR_SET_APP_CONFIG_CMD)
 * OID: 010010b (RADAR_GET_APP_CONFIG_CMD)
 */
@VintfStability
@Backing(type="int")
enum UwbVendorRadarAppConfigTlvTypes {
    /**
     * 7 byte data
     * Radar frame timing parameters:
     * Octet [3:0] - BURST_PERIOD
     *   Duration between the start of two consecutive Radar bursts in ms.
     * Octet [5:4] - SWEEP_PERIOD
     *   Duration between the start times of two consecutive Radar sweeps in
     *   RSTU.
     * Octet [6] - SWEEPS_PER_BURST
     *   Number of Radar sweeps within the Radar burst.
     */
    RADAR_TIMING_PARAMS = 0x0,
    /**
     * 1 byte data
     * The number of samples captured for each radar sweep. (default = 64)
     */
    SAMPLES_PER_SWEEP = 0x1,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config.
     * (default = 9)
     */
    RADAR_CHANNEL_NUMBER = 0x2,
    /**
     * 2 byte data
     * Defines the start offset with respect to 0cm distance to limit the sweep
     * range. Signed value and unit in samples.
     * (default = 0)
     */
    SWEEP_OFFSET = 0x3,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config.
     * (default = 0x0)
     */
    RADAR_RFRAME_CONFIG = 0x4,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config, but extended to 0xA.
     * (default = 0x2 : 128 symbols)
     */
    RADAR_PREAMBLE_DURATION = 0x5,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config, but extended to 127.
     * (default = 25)
     */
    RADAR_PREAMBLE_CODE_INDEX = 0x6,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config.
     * (default = 50)
     */
    RADAR_SESSION_PRIORITY = 0x7,
    /**
     * 1 byte data
     * Bits per sample in the radar sweep.
     * 0x00 = 32 bits per sample (default)
     * 0x01 = 48 bits per sample
     * 0x02 = 64 bits per sample
     */
    BITS_PER_SAMPLE = 0x8,
    /**
     * 1 byte data
     * Same as in FiRa UCI Session App Config.
     * (default = 0x1)
     */
    RADAR_PRF_MODE = 0x9,
    /**
     * 2 byte data
     * Maximum number of Radar bursts to be executed in the session. The
     * session is stopped and moved to SESSION_STATE_IDLE Session State when
     * configured radar bursts are elapsed.
     * 0x00 = Unlimited (default)
     */
    NUMBER_OF_BURSTS = 0xA,
    /**
     * 2 byte data
     * Type of radar data to be reported.
     * 0x00: Radar Sweep Samples. Reported in RADAR_DATA_NTF. (default)
     */
    RADAR_DATA_TYPE = 0xB,
}
