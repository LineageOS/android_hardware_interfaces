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

package android.hardware.input.processor;

@VintfStability
interface IInputProcessor {
    /**
     * Upon receiving the current motion event, return the classification based on the current
     * sequence of motion events.
     * Once the classification has been determined, it should not change until a new gesture is
     * started.
     */
    android.hardware.input.common.Classification classify(
            in android.hardware.input.common.MotionEvent event);

    /**
     * Reset the HAL internal state. The reset may be called to prevent an inconsistent
     * stream of events to be sent to the HAL.
     */
    void reset();

    /**
     * Called when an input device has been reset.
     */
    void resetDevice(in int deviceId);
}
