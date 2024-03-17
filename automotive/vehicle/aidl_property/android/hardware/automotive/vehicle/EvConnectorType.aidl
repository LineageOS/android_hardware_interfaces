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
    /**
     * IEC 62196 Type 1 connector
     *
     * Also known as the "Yazaki connector" or "J1772 connector".
     */
    IEC_TYPE_1_AC = 1,
    /**
     * IEC 62196 Type 2 connector
     *
     * Also known as the "Mennekes connector".
     */
    IEC_TYPE_2_AC = 2,
    /**
     * IEC 62196 Type 3 connector
     *
     * Also known as the "Scame connector".
     */
    IEC_TYPE_3_AC = 3,
    /**
     * IEC 62196 Type AA connector
     *
     * Also known as the "Chademo connector".
     */
    IEC_TYPE_4_DC = 4,
    /**
     * IEC 62196 Type EE connector
     *
     * Also known as the “CCS1 connector” or “Combo1 connector".
     */
    IEC_TYPE_1_CCS_DC = 5,
    /**
     * IEC 62196 Type EE connector
     *
     * Also known as the “CCS2 connector” or “Combo2 connector”.
     */
    IEC_TYPE_2_CCS_DC = 6,
    /**
     * DO NOT USE
     *
     * Connector of Tesla Roadster.
     */
    TESLA_ROADSTER = 7,
    /**
     * DO NOT USE
     * Use TESLA_SUPERCHARGER instead.
     *
     * High Power Wall Charger of Tesla.
     */
    TESLA_HPWC = 8,
    /**
     * SAE J3400 connector
     *
     * Also known as the "North American Charging Standard" (NACS)
     * or the "Tesla charging standard" connector.
     */
    TESLA_SUPERCHARGER = 9,
    /** GBT_AC Fast Charging Standard */
    GBT_AC = 10,
    /** GBT_DC Fast Charging Standard */
    GBT_DC = 11,
    /**
     * Connector type to use when no other types apply. Before using this
     * value, work with Google to see if the EvConnectorType enum can be
     * extended with an appropriate value.
     */
    OTHER = 101,
}
