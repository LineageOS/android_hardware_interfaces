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

package android.hardware.vibrator;

@VintfStability
@Backing(type="int")
enum Braking {
    /**
     * No braking mechanism used.
     * This is the default if the hardware does not support any braking mechanism.
     */
    NONE,
    /**
     * Closed-loop active braking.
     *
     * This effect should produce a sharp, crisp end to the waveform
     * Support is optional.
     */
    CLAB,
}
