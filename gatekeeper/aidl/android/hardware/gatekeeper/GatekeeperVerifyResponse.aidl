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

import android.hardware.security.keymint.HardwareAuthToken;

/**
 * Gatekeeper response to verify requests has this structure as mandatory part
 */
@VintfStability
parcelable GatekeeperVerifyResponse {
    /**
     * Request completion status. The status code can be IGatekeeper::STATUS_OK
     * or IGatekeeper::ERROR_RETRY_TIMEOUT or IGatekeeper::STATUS_REENROLL.
     */
    int statusCode;
    /**
     * Retry timeout in ms, if code == IGatekeeper::ERROR_RETRY_TIMEOUT
     * otherwise unused (0)
     */
    int timeoutMs;
    /**
     * On successful verification of the password,
     * IGatekeeper implementations must return hardware auth token
     * in the response.
     */
    HardwareAuthToken hardwareAuthToken;
}
