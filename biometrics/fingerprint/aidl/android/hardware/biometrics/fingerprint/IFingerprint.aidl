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
     * Creates a instance of ISession which can be used by the framework to perform operations
     * such as ISession#enroll, ISession#authenticate, etc. for the given sensorId and userId.
     *
     * Calling this method while there is an active session is considered an error. If the framework
     * wants to create a new session when it already has an active session, it must first cancel the
     * current operation if it's cancellable, or wait until it completes. Then, the framework must
     * explicitly close the session with ISession#close. Once the framework receives
     * ISessionCallback#onSessionClosed, a new session can be created.
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
}
