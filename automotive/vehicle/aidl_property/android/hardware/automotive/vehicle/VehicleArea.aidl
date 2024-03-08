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
 * List of different supported area types for vehicle properties.
 * Used to construct property IDs in the VehicleProperty enum.
 *
 * Some properties may be associated with particular areas in the vehicle. For example,
 * VehicleProperty#DOOR_LOCK property must be associated with a particular door, thus this property
 * must be of the VehicleArea#DOOR area type.
 *
 * Other properties may not be associated with a particular area in the vehicle. These kinds of
 * properties must be of the VehicleArea#GLOBAL area type.
 *
 * Note: This is not the same as areaId used in VehicleAreaConfig. E.g. for a global property, the
 * property ID is of the VehicleArea#GLOBAL area type, however, the area ID must be 0.
 */
@VintfStability
@Backing(type="int")
// A better name would be VehicleAreaType
enum VehicleArea {
    /**
     * A global property is a property that applies to the entire vehicle and is not associated with
     * a specific area. For example, FUEL_LEVEL, HVAC_STEERING_WHEEL_HEAT are global properties.
     */
    GLOBAL = 0x01000000,
    /** WINDOW maps to enum VehicleAreaWindow */
    WINDOW = 0x03000000,
    /** MIRROR maps to enum VehicleAreaMirror */
    MIRROR = 0x04000000,
    /** SEAT maps to enum VehicleAreaSeat */
    SEAT = 0x05000000,
    /** DOOR maps to enum VehicleAreaDoor */
    DOOR = 0x06000000,
    /** WHEEL maps to enum VehicleAreaWheel */
    WHEEL = 0x07000000,
    /**
     * A property with the VENDOR vehicle area contains area IDs that are vendor defined. Each area
     * ID within this area type must be unique with no overlapping bits set.
     */
    VENDOR = 0x08000000,

    MASK = 0x0f000000,
}
