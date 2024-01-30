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

package android.hardware.automotive.vehicle;

/**
 * Test vendor properties used in reference VHAL implementation.
 */
@Backing(type="int")
enum TestVendorProperty {

    /**
     * Vendor version of CLUSTER_SWITCH_UI, used for the end-to-end testing of ClusterHomeService.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.INT32,
     */
    VENDOR_CLUSTER_SWITCH_UI = 0x0F34 + 0x20000000 + 0x01000000 + 0x00400000,

    /**
     * Vendor version of CLUSTER_DISPLAY_STATE, used for the end-to-end testing of
     * ClusterHomeService.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.INT32_VEC
     */
    VENDOR_CLUSTER_DISPLAY_STATE = 0x0F35 + 0x20000000 + 0x01000000 + 0x00410000,

    /**
     * Vendor version of CLUSTER_REPORT_STATE, used for the end-to-end testing of
     * ClusterHomeService.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyGroup.MIXED
     */
    VENDOR_CLUSTER_REPORT_STATE = 0x0F36 + 0x20000000 + 0x01000000 + 0x00E00000,

    /**
     * Vendor version of CLUSTER_REQUEST_DISPLAY, used for the end-to-end testing of
     * ClusterHomeService.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.INT32
     */
    VENDOR_CLUSTER_REQUEST_DISPLAY = 0x0F37 + 0x20000000 + 0x01000000 + 0x00400000,

    /**
     * Vendor version of CLUSTER_NAVIGATION_STATE, used for the end-to-end testing of
     * ClusterHomeService.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.BYTES
     */
    VENDOR_CLUSTER_NAVIGATION_STATE = 0x0F38 + 0x20000000 + 0x01000000 + 0x00700000,

    // These properties are placeholder properties for developers to test new features without
    // implementing a real property.

    /**
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.INT32
     */
    PLACEHOLDER_PROPERTY_INT = 0x2A11 + 0x20000000 + 0x01000000 + 0x00400000,

    /**
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.FLOAT
     */
    PLACEHOLDER_PROPERTY_FLOAT = 0x2A11 + 0x20000000 + 0x01000000 + 0x00600000,

    /**
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.BOOLEAN
     */
    PLACEHOLDER_PROPERTY_BOOLEAN = 0x2A11 + 0x20000000 + 0x01000000 + 0x00200000,

    /**
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.STRING
     */
    PLACEHOLDER_PROPERTY_STRING = 0x2A11 + 0x20000000 + 0x01000000 + 0x00100000,

    /**
     * This property is used for testing LargeParcelable marshalling/unmarhsalling end to end.
     * It acts as an regular property that stores the property value when setting and return the
     * value when getting, except that all the byteValues used in the setValue response would be
     * filled in the reverse order.
     *
     * This is used in {@code VehicleHalLargeParcelableTest}.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.BYTES
     *
     * 0x21702a12
     */
    ECHO_REVERSE_BYTES = 0x2A12 + 0x20000000 + 0x01000000 + 0x00700000,

    /**
     * This property is used for testing vendor error codes end to end.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.INT32
     *
     * 0x21402a13
     */
    VENDOR_PROPERTY_FOR_ERROR_CODE_TESTING = 0x2A13 + 0x20000000 + 0x01000000 + 0x00400000,

    /**
     * This property is used for test purpose. End to end tests use this property to test set and
     * get method for MIXED type properties.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyGroup.MIXED
     */
    MIXED_TYPE_PROPERTY_FOR_TEST = 0x1111 + 0x20000000 + 0x01000000 + 0x00E00000,

    /**
     * Property used for {@code CarVendorPropertyCustomPermissionTest}.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.DOOR | VehiclePropertyGroup.BOOLEAN
     */
    VENDOR_EXTENSION_BOOLEAN_PROPERTY = 0x0101 + 0x20000000 + 0x06000000 + 0x00200000,

    /**
     * Property used for {@code CarVendorPropertyCustomPermissionTest}.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.SEAT | VehiclePropertyGroup.FLOAT
     */
    VENDOR_EXTENSION_FLOAT_PROPERTY = 0x102 + 0x20000000 + 0x05000000 + 0x00600000,

    /**
     * Property used for {@code CarVendorPropertyCustomPermissionTest}.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.WINDOW | VehiclePropertyGroup.INT32
     */
    VENDOR_EXTENSION_INT_PROPERTY = 0x103 + 0x20000000 + 0x03000000 + 0x00400000,

    /**
     * Property used for {@code CarVendorPropertyCustomPermissionTest}.
     *
     * VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyGroup.STRING
     */
    VENDOR_EXTENSION_STRING_PROPERTY = 0x103 + 0x20000000 + 0x01000000 + 0x00100000,
}
