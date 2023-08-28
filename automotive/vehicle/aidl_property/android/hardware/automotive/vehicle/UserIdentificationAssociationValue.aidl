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
 * Whether a UserIdentificationAssociationType is associate with an Android user.
 */
@VintfStability
@Backing(type="int")
enum UserIdentificationAssociationValue {
    /**
     * Used when the status of an association could not be determined.
     *
     * For example, in a set() request, it would indicate a failure to set the given type.
     */
    UNKNOWN = 1,
    /**
     * The identification type is associated with the current foreground Android user.
     */
    ASSOCIATED_CURRENT_USER = 2,
    /**
     * The identification type is associated with another Android user.
     */
    ASSOCIATED_ANOTHER_USER = 3,
    /**
     * The identification type is not associated with any Android user.
     */
    NOT_ASSOCIATED_ANY_USER = 4,
}
