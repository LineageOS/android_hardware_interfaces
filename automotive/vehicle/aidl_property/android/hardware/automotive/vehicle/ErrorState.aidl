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
 * Used to enumerate the possible error states. For version 2 of this interface, ErrorState is used
 * by ADAS STATE properties, but its use may be expanded in future releases.
 */
@VintfStability
enum ErrorState {

    /**
     * This state is used as an alternative to any ErrorState value that is not defined in the
     * platform. Ideally, implementations of vehicle properties should not use this state. The
     * framework can use this field to remain backwards compatible if this enum is extended to
     * include additional states.
     */
    OTHER_ERROR_STATE = -1,
    NOT_AVAILABLE_DISABLED = -2,
    NOT_AVAILABLE_SPEED_LOW = -3,
    NOT_AVAILABLE_SPEED_HIGH = -4,
    NOT_AVAILABLE_SAFETY = -5,
}
