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

package android.hardware.audio.core;

import android.hardware.audio.core.VendorParameter;

/**
 * An instance of IBluetoothA2dp manages settings for the A2DP (Advanced Audio
 * Distribution Profile) profiles. This interface is optional to implement by
 * the vendor. It needs to be provided only if the device actually supports BT
 * A2DP.
 *
 * Each of IBluetooth* interfaces is independent of each other. The HAL module
 * can provide any combination of them.
 */
@VintfStability
interface IBluetoothA2dp {
    /**
     * Whether BT A2DP is enabled.
     *
     * Returns the current state of A2DP support. The client might need to
     * disable (suspend) A2DP when another profile (for example, SCO) is
     * activated.
     *
     * @return Whether BT A2DP is enabled.
     */
    boolean isEnabled();

    /**
     * Enable or disable A2DP.
     *
     * Sets the current state of A2DP support. The client might need to
     * disable (suspend) A2DP when another profile (for example, SCO) is
     * activated.
     *
     * @param enabled Whether BT A2DP must be enabled or suspended.
     * @throws EX_ILLEGAL_STATE If there was an error performing the operation.
     */
    void setEnabled(boolean enabled);

    /**
     * Indicates whether the module supports reconfiguration of offloaded codecs.
     *
     * Offloaded codec implementations may need to be reconfigured when the
     * active A2DP device changes. This method indicates whether the HAL module
     * supports the reconfiguration event. The result returned from this method
     * must not change over time.
     *
     * @return Whether reconfiguration offload of offloaded codecs is supported.
     */
    boolean supportsOffloadReconfiguration();

    /**
     * Instructs the HAL module to reconfigure offloaded codec.
     *
     * Offloaded codec implementations may need to be reconfigured when the
     * active A2DP device changes. This method is a notification for the HAL
     * module to commence reconfiguration.
     *
     * Note that 'EX_UNSUPPORTED_OPERATION' must be thrown if and only if
     * 'supportsOffloadReconfiguration' returns 'false'.
     *
     * @param parameter Optional vendor-specific parameters, can be left empty.
     * @throws EX_ILLEGAL_STATE If there was an error performing the operation,
     *                          or the operation can not be commenced in the current state.
     * @throws EX_UNSUPPORTED_OPERATION If the module does not support codec reconfiguration.
     */
    void reconfigureOffload(in VendorParameter[] parameters);
}
