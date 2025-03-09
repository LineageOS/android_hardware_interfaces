/*
 * Copyright (C) 2024 The Android Open Source Project
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
interface IVibrationSession {
    /**
     * Request the end of this session.
     *
     * This will cause this session to end once the ongoing vibration commands are completed in each
     * individual vibrator. The immediate end of this session can stll be trigged via abort().
     *
     * This should not block on the end of this session. The callback provided during the creation
     * of this session should be used to indicate the vibrations are done and the session has
     * ended. The session object can be safely destroyed after this is called, and the session
     * should end as expected.
     */
    void close();

    /**
     * Immediately end this session.
     *
     * This will cause this session to end immediately and stop any ongoing vibration. The vibrator
     * manager and each individual vibrator in this session will be reset and available when this
     * returns.
     */
    void abort();
}
