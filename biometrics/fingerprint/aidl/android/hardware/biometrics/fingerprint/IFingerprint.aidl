/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.biometrics.fingerprint;

import android.hardware.biometrics.fingerprint.ISession;
import android.hardware.biometrics.fingerprint.ISessionCallback;
import android.hardware.biometrics.fingerprint.SensorProps;

@VintfStability
interface IFingerprint {
    /**
     * getSensorProps:
     *
     * @return A list of properties for all sensors that an instance of the HAL supports.
     */
    SensorProps[] getSensorProps();

    /**
     * createSession:
     *
     * Creates a session which can then be used by the framework to perform operations such as
     * enroll, authenticate, etc for the given sensorId and userId.
     *
     * Calling this method while there is an active session is considered an error. If the
     * framework is in a bad state and for some reason cannot close its session, it should use
     * the reset method below.
     *
     * A physical sensor identified by sensorId typically supports only a single in-flight session
     * at a time. As such, if a session is currently in a state other than SessionState::IDLING, the
     * HAL MUST finish or cancel the current operation and return to SessionState::IDLING before the
     * new session is created. For example:
     *   1) If a session for sensorId=0, userId=0 is currently in a cancellable state (see
     *      ICancellationSignal) such as SessionState::AUTHENTICATING and the framework requests a
     *      new session for sensorId=0, userId=10, the HAL must end the current session with
     *      Error::CANCELED, invoke ISessionCallback#onStateChanged with SessionState::IDLING, and
     *      then return a new session for sensorId=0, userId=10.
     *   2) If a session for sensorId=0, userId=0 is currently in a non-cancellable state such as
     *      SessionState::REMOVING_ENROLLMENTS, and the framework requests a new session for
     *      sensorId=0, userId=10, the HAL must finish the current operation before invoking
     *      ISessionCallback#onStateChanged with SessionState::IDLING, and return a new session for
     *      sensorId=0, userId=10.
     *
     * Implementations must store user-specific state or metadata in /data/vendor_de/<user>/fpdata
     * as specified by the SeLinux policy. This directory is created/removed by vold (see
     * vold_prepare_subdirs.cpp). Implementations may store additional user-specific data, such as
     * embeddings or templates in StrongBox.
     *
     * @param sensorId The sensor with which this session is being created.
     * @param userId The userId with which this session is being created.
     * @param cb Used to notify the framework.
     * @return A new session
     */
    ISession createSession(in int sensorId, in int userId, in ISessionCallback cb);

    /**
     * Resets the HAL into a clean state, forcing it to cancel all of the pending operations, close
     * its current session, and release all of the acquired resources.
     *
     * This should be used as a last resort to recover the HAL if the current session becomes
     * unresponsive. The implementation might choose to restart the HAL process to get back into a
     * good state.
     */
    void reset();
}
