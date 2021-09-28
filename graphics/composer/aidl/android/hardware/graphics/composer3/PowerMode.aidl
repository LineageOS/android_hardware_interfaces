/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

import android.hardware.graphics.composer3.PowerMode;

@VintfStability
@Backing(type="int")
enum PowerMode {
    /**
     * The display is fully off (blanked).
     */
    OFF = 0,
    /**
     * These are optional low power modes. getDozeSupport may be called to
     * determine whether a given display supports these modes.
     *
     *
     * The display is turned on and configured in a low power state that
     * is suitable for presenting ambient information to the user,
     * possibly with lower fidelity than ON, but with greater efficiency.
     */
    DOZE = 1,
    /**
     * The display is configured as in DOZE but may stop applying display
     * updates from the client. This is effectively a hint to the device
     * that drawing to the display has been suspended and that the
     * device must remain on in a low power state and continue
     * displaying its current contents indefinitely until the power mode
     * changes.
     *
     * This mode may also be used as a signal to enable hardware-based
     * doze functionality. In this case, the device is free to take over
     * the display and manage it autonomously to implement a low power
     * always-on display.
     */
    DOZE_SUSPEND = 3,
    /**
     * The display is fully on.
     */
    ON = 2,
    /**
     * The display is configured as in ON but may stop applying display
     * updates from the client. This is effectively a hint to the device
     * that drawing to the display has been suspended and that the
     * device must remain on and continue displaying its current contents
     * indefinitely until the power mode changes.
     *
     * This mode may also be used as a signal to enable hardware-based
     * functionality to take over the display and manage it autonomously
     * to implement a low power always-on display.
     */
    ON_SUSPEND = 4,
}
