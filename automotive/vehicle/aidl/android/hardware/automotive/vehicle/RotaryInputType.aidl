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

package android.hardware.automotive.vehicle;

/**
 * A rotary control which can rotate without limits. These controls use HW_ROTARY_INPUT to report
 * relative clockwise or counterclockwise motion. They have no absolute position.
 */
@VintfStability
@Backing(type="int")
enum RotaryInputType {
    /**
     * Main rotary control, typically in the center console, used to navigate the user interface.
     */
    ROTARY_INPUT_TYPE_SYSTEM_NAVIGATION = 0,
    /**
     * Volume control for adjusting audio volume.
     */
    ROTARY_INPUT_TYPE_AUDIO_VOLUME = 1,
}
