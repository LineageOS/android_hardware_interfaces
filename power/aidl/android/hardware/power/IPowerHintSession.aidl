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

package android.hardware.power;

import android.hardware.power.WorkDuration;

@VintfStability
oneway interface IPowerHintSession {
    /**
     * Updates the desired duration of a previously-created thread group.
     *
     * See {@link IPowerHintSession#createHintSession} for more information on how
     * the desired duration will be used.
     *
     * @param targetDurationNanos the new desired duration in nanoseconds
     */
    void updateTargetWorkDuration(long targetDurationNanos);

    /**
     * Reports the actual duration of a thread group.
     *
     * The system will attempt to adjust the core placement of the threads within
     * the thread group and/or the frequency of the core on which they are run to bring
     * the actual duration close to the target duration.
     *
     * @param actualDurationMicros how long the thread group took to complete its
     *        last task in nanoseconds
     */
    void reportActualWorkDuration(in WorkDuration[] durations);

    /**
     * Pause the session when the application is not allowed to send hint in framework.
     */
    void pause();

    /**
     * Resume the session when the application is allowed to send hint in framework.
     */
    void resume();

    /**
     * Close the session to release resources.
     */
    void close();
}
