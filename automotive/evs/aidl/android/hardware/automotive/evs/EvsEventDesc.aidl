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

import android.hardware.automotive.evs.EvsEventType;

/**
 * Structure that describes informative events occurred during EVS is streaming
 */
@VintfStability
parcelable EvsEventDesc {
    /**
     * Type of an informative event
     */
    EvsEventType aType;
    /**
     * Device identifier
     */
    @utf8InCpp
    String deviceId;
    /**
     * Possible additional vendor information that is opaque to the EvsManager.
     * The size of the payload must not exceed 16-byte if the HIDL recipients are
     * expected to exist.
     */
    int[] payload;
}
