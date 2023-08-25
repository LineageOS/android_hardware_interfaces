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
 * Used to set a UserIdentificationAssociationType with an Android user.
 */
@VintfStability
@Backing(type="int")
enum UserIdentificationAssociationSetValue {
    INVALID = 0,
    /**
     * Associate the identification type with the current foreground Android user.
     */
    ASSOCIATE_CURRENT_USER = 1,
    /**
     * Disassociate the identification type from the current foreground Android user.
     */
    DISASSOCIATE_CURRENT_USER = 2,
    /**
     * Disassociate the identification type from all Android users.
     */
    DISASSOCIATE_ALL_USERS = 3,
}
