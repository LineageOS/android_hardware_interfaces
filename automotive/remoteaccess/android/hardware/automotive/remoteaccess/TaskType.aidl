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
@Backing(type="int")
enum TaskType {
    /**
     * A custom task that is opaque to anyone other than the remote task client app.
     *
     * <p>The opaque task data in the {@code ScheduleInfo} will be sent back to the app when the
     * task is to be executed.
     */
    CUSTOM = 0,
    /**
     * Enters the garage mode if allowed.
     *
     * <p>Make the Android system enters garage mode if vehicle is currently not in use and
     * entering garage mode is allowed (e.g. battery level is high enough).
     *
     * <p>This is based on best-effort and it is not guaranteed.
     *
     * <p>If allowed, the external system should set {@code AP_POWER_BOOTUP_REASON} to
     * {@code SYSTEM_ENTER_GARAGE_MODE} and then boot up (or resume) the head unit.
     */
    ENTER_GARAGE_MODE = 1,
}
