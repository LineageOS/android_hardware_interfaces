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

package android.hardware.tv.hdmi.connection;

/**
 * Result enum for return values. Used by the HDMI related AIDL.
 */
@VintfStability
enum Result {
    /**
     * The HPD Signal type was set successfully.
     */
    SUCCESS = 0,

    /**
     * The HPD Signal type could not be set because of an unknown failure.
     */
    FAILURE_UNKNOWN = 1,

    /**
     * The HPD Signal type could not be set because the arguments were invalid.
     */
    FAILURE_INVALID_ARGS = 2,

    /**
     * The HPD Signal type could not be set because the HAL is in an invalid state.
     */
    FAILURE_INVALID_STATE = 3,

    /**
     * The HPD Signal type could not be set as the signal type is not supported.
     */
    FAILURE_NOT_SUPPORTED = 4,
}
