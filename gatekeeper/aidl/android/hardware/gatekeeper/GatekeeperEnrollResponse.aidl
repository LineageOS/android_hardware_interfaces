/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.gatekeeper;

/**
 * Gatekeeper response to enroll requests has this structure as mandatory part
 */
@VintfStability
parcelable GatekeeperEnrollResponse {
    /**
     * Request completion status. The status code can be IGatekeeper::STATUS_OK
     * or IGatekeeper::ERROR_RETRY_TIMEOUT.
     */
    int statusCode;
    /**
     * Retry timeout in ms, if code == IGatekeeper::ERROR_RETRY_TIMEOUT
     * otherwise unused (0)
     */
    int timeoutMs;
    /**
     * secure user id.
     */
    long secureUserId;
    /**
     * optional crypto blob. Opaque to Android system.
     */
    byte[] data;
}
