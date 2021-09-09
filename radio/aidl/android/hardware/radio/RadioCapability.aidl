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

import android.hardware.radio.RadioAccessFamily;
import android.hardware.radio.RadioCapabilityPhase;
import android.hardware.radio.RadioCapabilityStatus;

@VintfStability
parcelable RadioCapability {
    /**
     * Unique session value defined by framework returned in all "responses/unslo".
     */
    int session;
    RadioCapabilityPhase phase;
    /**
     * 32-bit bitmap of RadioAccessFamily.
     */
    RadioAccessFamily raf;
    /**
     * A UUID typically "com.xxxx.lmX" where X is the logical modem.
     * RadioConst:MAX_UUID_LENGTH is the max length.
     */
    String logicalModemUuid;
    RadioCapabilityStatus status;
}
