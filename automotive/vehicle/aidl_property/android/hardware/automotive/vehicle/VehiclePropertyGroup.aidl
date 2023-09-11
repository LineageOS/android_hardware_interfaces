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

@VintfStability
@Backing(type="int")
enum VehiclePropertyGroup {
    /**
     * Properties declared in AOSP must use this flag.
     */
    SYSTEM = 0x10000000,

    /**
     * Properties declared by vendors must use this flag.
     */
    VENDOR = 0x20000000,

    /**
     * Group reserved for backporting system properties introduced in a newer Android
     * release to an older Android release.
     *
     * It is recommended to map the system property ID to a backported property ID by replacing the
     * VehiclePropertyGroup, e.g. backported PERF_VEHICLE_SPEED(0x11600207) would be 0x31600207.
     *
     * When updated to a newer Android release where the property is defined as system properties,
     * the backported properties must be migrated to system properties.
     *
     * In Android system, the backported property is treated the same as a vendor defined property
     * with the same vendor permission model, a.k.a. Default required permission is
     * `android.car.Car.PERMISSION_VENDOR_EXTENSION`, or customized by
     * `SUPPORT_CUSTOMIZE_VENDOR_PERMISSION` VHAL property.
     *
     * Only applications with vendor permissions may access these backported properties.
     *
     * Vendors must also make sure this property's behavior is consistent with what is expected for
     * the backported system property, e.g. the access mode, the change mode and the config array
     * must be correct.
     *
     * When vendors define custom properties, they must use {@code VENDOR} flag, instead of
     * {@code BACKPORTED}
     */
    BACKPORTED = 0x30000000,

    /**
     * The bit mask for {@code VehiclePropertyGroup}. This is not a group by itself.
     */
    MASK = 0xf0000000,
}
