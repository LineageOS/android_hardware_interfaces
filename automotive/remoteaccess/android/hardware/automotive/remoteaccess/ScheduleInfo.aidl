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

package android.hardware.automotive.remoteaccess;

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable ScheduleInfo {
    /**
     * The ID used to identify the client this schedule is for. This must be one of the
     * preconfigured remote access serverless client ID defined in car service resource
     * {@code R.xml.remote_access_serverless_client_map}.
     */
    String clientId;
    /**
     * A unique scheduling ID (among the same client). Adding a new schedule info with a duplicate
     * scheduleId will return {@code EX_ILLEGAL_ARGUMENT}.
     */
    String scheduleId;
    /**
     * The opaque task data that will be sent back to the remote task client app when the task is
     * executed. It is not interpreted/parsed by the Android system.
     */
    byte[] taskData;
    /**
     * How many times this task will be executed. 0 means infinite.
     *
     * <p>This must be >= 0.
     */
    int count;
    /**
     * The start time in epoch seconds.
     *
     * <p>The external device issuing remote task must have a clock synced with the
     * {@code System.currentTimeMillis()} used in Android system.
     *
     * <p>Optionally, the VHAL property {@code EPOCH_TIME} can be used to sync the time.
     *
     * <p>This must be >= 0.
     */
    long startTimeInEpochSeconds;
    /**
     * The interval (in seconds) between scheduled task execution.
     *
     * <p>This must be >=0. This is not useful when {@code count} is 1. If this is 0,
     * The tasks will be delivered multiple times with no interval in between.
     */
    long periodicInSeconds;
}
