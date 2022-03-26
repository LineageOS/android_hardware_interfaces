/**
 * Copyright (c) 2022, The Android Open Source Project
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

@VintfStability
@Backing(type="int")
enum DimmingStage {
    /**
     * Corresponds to if the dimming stage is not meaningful for a scene, such as
     * if client composition is not requesting dimming.
     */
    NONE = 0,
    /**
     * Dimming operations must be applied in linear optical space
     */
    LINEAR = 1,
    /**
     * Dimming operations must be applied in gamma space, after OETF has been applied.
     * Note that for this dimming operation to be perceptually correct it must also be gamma
     * corrected. The framework will assume that it is able to use the gamma 2.2
     * power function for gamma correcting the dimming matrix, for simplicity of
     * implementation and the fact that gamma 2.2 is close enough to typical SDR
     * transfer functions that would be used for the client target.
     */
    GAMMA_OETF = 2,
}
