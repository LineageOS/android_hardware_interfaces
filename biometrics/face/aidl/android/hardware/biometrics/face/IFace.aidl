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

/**
 * @hide
 */
@VintfStability
interface IFace {
    /**
     * getSensorProps:
     *
     * @return A list of properties for all of the face sensors supported by the HAL.
     */
    SensorProps[] getSensorProps();

    /**
     * createSession:
     *
     * Creates an instance of ISession that can be used by the framework to perform operations such
     * as ISession#enroll, ISession#authenticate, etc. for the given sensorId and userId.
     *
     * Calling this method while there is an active session is considered an error. If the framework
     * wants to create a new session when it already has an active session, it must first cancel the
     * current operation if it's cancellable or wait until it completes. Then, the framework must
     * explicitly close the session with ISession#close. Once the framework receives
     * ISessionCallback#onSessionClosed, a new session can be created.
     *
     * Implementations must store user-specific state or metadata in /data/vendor_de/<user>/facedata
     * as specified by the SELinux policy. The directory /data/vendor_de is managed by vold (see
     * vold_prepare_subdirs.cpp). Implementations may store additional user-specific data, such as
     * embeddings or templates, in StrongBox.
     *
     * During create session it is expected that the HAL will call linkToDeath with the callee's
     * binder token. The recommended implementation is to close this session if the callee dies,
     * to prevent subsequent createSession calls from failing.
     *
     * @param sensorId The sensorId for which this session is being created.
     * @param userId The userId for which this session is being created.
     * @param cb A callback to notify the framework about the session's events.
     * @return A new session.
     */
    ISession createSession(in int sensorId, in int userId, in ISessionCallback cb);
}
