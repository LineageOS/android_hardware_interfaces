/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * Used by INFO_EV_CONNECTOR_TYPE to enumerate the type of connectors
 * available to charge the vehicle.
 */
@VintfStability
@Backing(type="int")
enum EvConnectorType {
    /**
     * Default type if the vehicle does not know or report the EV connector
     * type.
     */
    UNKNOWN = 0,
    IEC_TYPE_1_AC = 1,
    IEC_TYPE_2_AC = 2,
    IEC_TYPE_3_AC = 3,
    IEC_TYPE_4_DC = 4,
    IEC_TYPE_1_CCS_DC = 5,
    IEC_TYPE_2_CCS_DC = 6,
    TESLA_ROADSTER = 7,
    TESLA_HPWC = 8,
    TESLA_SUPERCHARGER = 9,
    GBT_AC = 10,
    GBT_DC = 11,
    /**
     * Connector type to use when no other types apply. Before using this
     * value, work with Google to see if the EvConnectorType enum can be
     * extended with an appropriate value.
     */
    OTHER = 101,
}
