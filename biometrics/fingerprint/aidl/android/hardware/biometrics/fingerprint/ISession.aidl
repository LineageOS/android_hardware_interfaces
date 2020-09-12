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

import android.hardware.biometrics.fingerprint.ICancellationSignal;
import android.hardware.keymaster.HardwareAuthToken;

@VintfStability
interface ISession {
    /**
     * Methods applicable to any fingerprint type.
     */

    ICancellationSignal enroll(in int cookie, in HardwareAuthToken hat);

    ICancellationSignal authenticate(in int cookie, in long keystoreOperationId);

    ICancellationSignal detectInteraction(in int cookie);

    void enumerateEnrollments(in int cookie);

    void removeEnrollments(in int cookie, in int[] enrollmentIds);

    void getAuthenticatorId(in int cookie);

    void resetLockout(in int cookie, in HardwareAuthToken hat);


    /**
     * Methods for notifying the under-display fingerprint sensor about external events.
     */

    void onPointerDown(in int pointerId, in int x, in int y, in float minor, in float major);

    void onPointerUp(in int pointerId);

    void onUiReady();
}

