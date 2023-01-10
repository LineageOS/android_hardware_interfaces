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

package android.hardware.tv.hdmi.earc;

/**
 * Result enum for return values. Used by the HDMI related AIDL.
 */
@VintfStability
enum Result {
    /**
     * The eARC enabled setting was set successfully.
     */
    SUCCESS = 0,

    /**
     * The eARC enabled setting could not be set because of an unknown failure.
     */
    FAILURE_UNKNOWN = 1,

    /**
     * The eARC enabled setting could not be set because the arguments were invalid.
     */
    FAILURE_INVALID_ARGS = 2,

    /**
     * The eARC enabled setting could not be set because eARC feature is not supported.
     */
    FAILURE_NOT_SUPPORTED = 4,
}
