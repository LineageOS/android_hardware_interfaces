/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * Used by CAMERA_SERVICE_CURRENT_STATE to describe current state of CarEvsService types.
 *
 * This is consistent with CarEvsManager.SERVICE_STATE_* constants.
 */
@VintfStability
@Backing(type="int")
enum CameraServiceState {
    /**
     * State that a corresponding service type is not available.
     */
    UNAVAILABLE = 0,

    /**
     * State that a corresponding service type is inactive; it's available but not used
     * by any clients yet.
     */
    INACTIVE = 1,

    /**
     * State that CarEvsService requested launching a registered activity; the service is waiting
     * for a video request from it.
     */
    REQUESTED = 2,

    /**
     * State that a corresponding service type is actively being used.
     */
    ACTIVE = 3,
}
