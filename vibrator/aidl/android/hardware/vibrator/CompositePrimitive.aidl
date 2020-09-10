/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.vibrator;

@VintfStability
@Backing(type="int")
enum CompositePrimitive {
    /**
     * No haptic effect. Used to generate extended delays between primitives.
     *
     * Support is required.
     */
    NOOP,
    /**
     * This effect should produce a sharp, crisp click sensation.
     *
     * Support is required.
     */
    CLICK,
    /**
     * A haptic effect that simulates downwards movement with gravity. Often
     * followed by extra energy of hitting and reverberation to augment
     * physicality.
     *
     * Support is optional.
     */
    THUD,
    /**
     * A haptic effect that simulates spinning momentum.
     *
     * Support is optional.
     */
    SPIN,
    /**
     * A haptic effect that simulates quick upward movement against gravity.
     *
     * Support is required.
     */
    QUICK_RISE,
    /**
     * A haptic effect that simulates slow upward movement against gravity.
     *
     * Support is required.
     */
    SLOW_RISE,
    /**
     * A haptic effect that simulates quick downwards movement with gravity.
     *
     * Support is required.
     */
    QUICK_FALL,
    /**
     * This very short effect should produce a light crisp sensation intended
     * to be used repetitively for dynamic feedback.
     *
     * Support is required.
     */
    LIGHT_TICK,
}
