/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.powerstats;

import android.hardware.powerstats.PowerEntityType;

/**
 * PowerEntityInfo contains information, such as the ID, name, and type of a
 * given PowerEntity.
 */
@VintfStability
parcelable PowerEntityInfo {
    /**
     * Unique ID corresponding to the PowerEntity
     */
    int powerEntityId;
    /**
     * Name of the PowerEntity (opaque to the framework)
     */
    String powerEntityName;
    /**
     * Type of the PowerEntity
     */
    PowerEntityType type;
}