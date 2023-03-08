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

package android.hardware.audio.core;

import android.media.audio.common.Boolean;
import android.media.audio.common.Float;
import android.media.audio.common.Int;

/**
 * An instance of IBluetooth manages settings for the Hands-Free Profile (HFP)
 * and the SCO Link. This interface is optional to implement and provide by the
 * vendor. It needs to be provided only if the device actually supports BT SCO
 * or HFP.
 *
 * Each of IBluetooth* interfaces is independent of each other. The HAL module
 * can provide any combination of them.
 */
@VintfStability
interface IBluetooth {
    @JavaDerive(equals=true, toString=true)
    @VintfStability
    parcelable ScoConfig {
        /**
         * Whether BT SCO is enabled. The client might need to disable it
         * when another profile (for example, A2DP) is activated.
         */
        @nullable Boolean isEnabled;
        /**
         * Whether BT SCO Noise Reduction and Echo Cancellation are enabled.
         */
        @nullable Boolean isNrecEnabled;
        @VintfStability enum Mode { UNSPECIFIED, SCO, SCO_WB, SCO_SWB }
        /**
         * If set, specifies the SCO mode to use:
         *   regular, wide band (WB), or super wide band (SWB).
         */
        Mode mode = Mode.UNSPECIFIED;
        /**
         * The name of the BT SCO headset used for debugging purposes. Can be empty.
         */
        @nullable @utf8InCpp String debugName;
    }

    /**
     * Set the configuration of Bluetooth SCO.
     *
     * In the provided parcelable, the client sets zero, one or more parameters
     * which have to be updated on the HAL side. The parameters that are left
     * unset must retain their current values. It is allowed to change
     * parameters while the SCO profile is disabled (isEnabled.value == false).
     *
     * In the returned parcelable, all parameter fields known to the HAL module
     * must be populated to their current values. If the SCO profile is
     * currently disabled (isEnabled.value == false), the parameters must
     * reflect the last values that were in use.
     *
     * The client can pass an uninitialized parcelable in order to retrieve the
     * current configuration.
     *
     * @return The current configuration (after update). All fields known to
     *         the HAL must be populated.
     * @param config The configuration to set. Any number of fields may be left
     *               uninitialized.
     * @throws EX_UNSUPPORTED_OPERATION If BT SCO is not supported.
     * @throws EX_ILLEGAL_ARGUMENT If the requested combination of parameter
     *                             values is invalid.
     */
    ScoConfig setScoConfig(in ScoConfig config);

    @JavaDerive(equals=true, toString=true)
    @VintfStability
    parcelable HfpConfig {
        /**
         * Whether BT HFP is enabled.
         */
        @nullable Boolean isEnabled;
        /**
         * The sample rate of BT HFP, in Hertz. Must be a positive number.
         */
        @nullable Int sampleRate;

        const int VOLUME_MIN = 0;
        const int VOLUME_MAX = 1;
        /**
         * The output volume of BT HFP. 1.0f means unity gain, 0.0f is muted,
         * see VOLUME_* constants;
         */
        @nullable Float volume;
    }

    /**
     * Set the configuration of Bluetooth HFP.
     *
     * In the provided parcelable, the client sets zero, one or more parameters
     * which have to be updated on the HAL side. The parameters that are left
     * unset must retain their current values. It is allowed to change
     * parameters while the HFP profile is disabled (isEnabled.value == false).
     *
     * In the returned parcelable, all parameter fields known to the HAL module
     * must be populated to their current values. If the HFP profile is
     * currently disabled (isEnabled.value == false), the parameters must
     * reflect the last values that were in use.
     *
     * The client can pass an uninitialized parcelable in order to retrieve the
     * current configuration.
     *
     * @return The current configuration (after update). All fields known to
     *         the HAL must be populated.
     * @param config The configuration to set. Any number of fields may be left
     *               uninitialized.
     * @throws EX_UNSUPPORTED_OPERATION If BT HFP is not supported.
     * @throws EX_ILLEGAL_ARGUMENT If the requested combination of parameter
     *                             values is invalid.
     */
    HfpConfig setHfpConfig(in HfpConfig config);
}
