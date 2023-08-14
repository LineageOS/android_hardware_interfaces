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

import android.hardware.power.SessionHint;
import android.hardware.power.SessionMode;
import android.hardware.power.WorkDuration;

@VintfStability
interface IPowerHintSession {
    /**
     * Updates the desired duration of a previously-created thread group.
     *
     * See {@link IPowerHintSession#createHintSession} for more information on how
     * the desired duration will be used.
     *
     * @param targetDurationNanos the new desired duration in nanoseconds
     */
    oneway void updateTargetWorkDuration(long targetDurationNanos);

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
    oneway void reportActualWorkDuration(in WorkDuration[] durations);

    /**
     * Pause the session when the application is not allowed to send hint in framework.
     */
    oneway void pause();

    /**
     * Resume the session when the application is allowed to send hint in framework.
     */
    oneway void resume();

    /**
     * Close the session to release resources.
     */
    oneway void close();

    /**
     * Gives information to the PowerHintSession about upcoming or unexpected
     * changes in load to supplement the normal updateTarget/reportActual cycle.
     *
     * @param hint The hint to provide to the PowerHintSession
     */
    oneway void sendHint(SessionHint hint);

    /**
     * Sets a list of threads to the power hint session. This operation will replace
     * the current list of threads with the given list of threads. If there's already
     * boost for the replaced threads, a reset must be performed for the replaced
     * threads. Note that this is not an oneway method.
     *
     * @param threadIds The list of threads to be associated
     * with this session.
     *
     * @throws ScopedAStatus Status of the operation. If status code is not
     *    STATUS_OK, getMessage() must be populated with the human-readable
     *    error message. If the list of thread ids is empty, EX_ILLEGAL_ARGUMENT
     *    must be thrown.
     */
    void setThreads(in int[] threadIds);

    /**
     * Called to enable or disable special modes for the hint session, which may
     * adjust the power or performance of the session.
     *
     * @param type The mode being set
     * @param enabled True to enable the mode, false to disable it
     */
    oneway void setMode(SessionMode type, boolean enabled);
}
