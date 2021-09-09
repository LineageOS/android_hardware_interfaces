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

package android.hardware.radio;

/**
 * Data connection active status
 */
@VintfStability
@Backing(type="int")
enum DataConnActiveStatus {
    /**
     * Indicates the data connection is inactive.
     */
    INACTIVE,
    /**
     * Indicates the data connection is active with physical link dormant.
     */
    DORMANT,
    /**
     * Indicates the data connection is active with physical link up.
     */
    ACTIVE,
}
