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

package android.hardware.automotive.evs;

/**
 * Structure describing the basic properties of an EVS camera.
 *
 * The HAL is responsible for filling out this structure for each
 * EVS camera in the system.
 */
@VintfStability
parcelable CameraDesc {
    /**
     * Unique identifier for camera devices.  This may be a path to detected
     * camera device; for example, "/dev/video0".
     */
    @utf8InCpp
    String id;
    /**
     * Opaque value from driver.  Vendor may use this field to store additional
     * information; for example, sensor and bridge chip id.
     */
    int vendorFlags;
    /**
     * Store camera metadata such as lens characteristics.
     */
    byte[] metadata;
}
