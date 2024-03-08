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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * Indicates the potential non-compliance reasons for the
 * connected USB Type-C port partner which could be a power
 * source, accessory or cable. Applicable for USB-C receptacles
 * in Android devices.
 */
enum ComplianceWarning {
    /**
     * Used to indicate Type-C sources/cables/accessories/ports
     * whose issue is not listed below but do not meet
     * specification requirements from including but not limited to
     * USB Type-C Cable and Connector Specification, Universal Serial Bus
     * Power Delivery Specification, and Universal Serial Bus
     * 1.x/2.0/3.x/4.0.
     */
    OTHER = 1,
    /**
     * Used to indicate Type-C port partner
     * (cable/accessory/source) that identifies itself as debug
     * accessory source as defined in USB Type-C Cable and
     * Connector Specification. However, the specification
     * states that this is meant for debug only and shall not
     * be used for with commercial products.
     */
    DEBUG_ACCESSORY = 2,
    /**
     * Used to indicate Type-C port partner that does not
     * identify itself as one of the charging port types
     * (SDP/CDP/DCP etc) as defined by Battery Charging v1.2
     * Specification.
     */
    BC_1_2 = 3,
    /**
     * Used to indicate Type-C sources/cables that are missing
     * pull up resistors on the CC pins as required by USB
     * Type-C Cable and Connector Specification.
     */
    MISSING_RP = 4,
    /**
     * Used to indicate the charging setups on the USB ports are unable to
     * deliver negotiated power.
     */
    INPUT_POWER_LIMITED = 5,
    /**
     * Used to indicate the cable/connector on the USB ports are missing
     * the required wires on the data pins to make data transfer.
     */
    MISSING_DATA_LINES = 6,
    /**
     * Used to indicate enumeration failures on the USB ports, potentially due to
     * signal integrity issues or other causes.
     */
    ENUMERATION_FAIL = 7,
    /**
     * Used to indicate unexpected data disconnection on the USB ports,
     * potentially due to signal integrity issues or other causes.
     */
    FLAKY_CONNECTION = 8,
    /**
     * Used to indicate unreliable or slow data transfer on the USB ports,
     * potentially due to signal integrity issues or other causes.
     */
    UNRELIABLE_IO = 9,
}
