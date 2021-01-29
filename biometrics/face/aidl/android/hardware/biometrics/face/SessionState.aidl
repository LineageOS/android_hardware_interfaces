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

@VintfStability
@Backing(type="byte")
enum SessionState {
    /**
     * The HAL is not processing any session requests.
     */
    IDLING,

    /**
     * The session has been terminated by the HAL.
     */
    TERMINATED,

    /**
     * The HAL is processing the ISession#generateChallenge request.
     */
    GENERATING_CHALLENGE,

    /**
     * The HAL is processing the ISession#revokeChallenge request.
     */
    REVOKING_CHALLENGE,

    /**
     * The HAL is processing the ISession#enroll request.
     */
    ENROLLING,

    /**
     * The HAL is processing the ISession#authenticate request.
     */
    AUTHENTICATING,

    /**
     * The HAL is processing the ISession#detectInteraction request.
     */
    DETECTING_INTERACTION,

    /**
     * The HAL is processing the ISession#enumerateEnrollments request.
     */
    ENUMERATING_ENROLLMENTS,

    /**
     * The HAL is processing the ISession#removeEnrollments request.
     */
    REMOVING_ENROLLMENTS,

    /**
     * The HAL is processing the ISession#getFeatures request.
     */
    GETTING_FEATURES,

    /**
     * The HAL is processing the ISession#setFeature request.
     */
    SETTING_FEATURE,

    /**
     * The HAL is processing the ISession#getAuthenticatorId request.
     */
    GETTING_AUTHENTICATOR_ID,

    /**
     * The HAL is processing the ISession#invalidateAuthenticatorId request.
     */
    INVALIDATING_AUTHENTICATOR_ID,

    /**
     * The HAL is processing the ISession#resetLockout request.
     */
    RESETTING_LOCKOUT
}

