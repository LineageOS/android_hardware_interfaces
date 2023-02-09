/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
1 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.thermal;

import android.hardware.thermal.CoolingType;

/* @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CoolingDevice {
    /**
     * This cooling device type, CPU, GPU, BATTERY, and etc.
     */
    CoolingType type;
    /**
     * Name of this cooling device.
     * All cooling devices of the same "type" must have a different "name".
     * The name is usually defined in kernel device tree, and this is for client
     * logging purpose.
     */
    String name;
    /**
     * Current throttle state of the cooling device. The value can any unsigned integer
     * numbers between 0 and max_state defined in its driver, usually representing the
     * associated device's power state. 0 means device is not in throttling, higher value
     * means deeper throttling.
     */
    long value;
}
