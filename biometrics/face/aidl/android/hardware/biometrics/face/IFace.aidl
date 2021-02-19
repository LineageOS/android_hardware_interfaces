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

package android.hardware.biometrics.face;

import android.hardware.biometrics.face.ISession;
import android.hardware.biometrics.face.ISessionCallback;
import android.hardware.biometrics.face.SensorProps;

@VintfStability
interface IFace {
    /**
     * getSensorProps:
     *
     * @return A list of properties for all face sensors available to the HAL.
     */
    SensorProps[] getSensorProps();

    /**
     * createSession:
     *
     * Creates a session that can be used by the framework to perform operations such as
     * enroll, authenticate, etc. for the given sensorId and userId.
     *
     * Calling this method while there is an active session is considered an error. If the
     * framework is in a bad state and for some reason cannot close its session, it should use
     * the reset method below.
     *
     * Implementations must store user-specific state or metadata in /data/vendor_de/<user>/facedata
     * as specified by the SELinux policy. The directory /data/vendor_de is managed by vold (see
     * vold_prepare_subdirs.cpp). Implementations may store additional user-specific data, such as
     * embeddings or templates in StrongBox.
     *
     * @param sensorId The sensorId with which this session is being created.
     * @param userId The userId with which this session is being created.
     * @param cb A callback to notify the framework about the session's results and events.
     * @return A new session.
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
