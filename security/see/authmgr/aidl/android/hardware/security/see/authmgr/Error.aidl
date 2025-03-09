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

package android.hardware.security.see.authmgr;

/**
 * AuthMgr error codes. Aidl will return these error codes as service specific errors in
 * EX_SERVICE_SPECIFIC.
 */
@VintfStability
@Backing(type="int")
enum Error {
    /** Success */
    OK = 0,

    /** Duplicated attempt to start authentication from the same transport ID */
    AUTHENTICATION_ALREADY_STARTED = -1,

    /** Duplicated authenticated attempt with the same instance ID */
    INSTANCE_ALREADY_AUTHENTICATED = -2,

    /** Invalid DICE certificate chain of the AuthMgr FE */
    INVALID_DICE_CERT_CHAIN = -3,

    /** Invalid DICE leaf of the client */
    INVALID_DICE_LEAF = -4,

    /** Invalid DICE policy */
    INVALID_DICE_POLICY = -5,

    /** The DICE chain to policy matching failed */
    DICE_POLICY_MATCHING_FAILED = -6,

    /** Invalid signature */
    SIGNATURE_VERIFICATION_FAILED = -7,

    /** Failed to handover the connection to the trusted service */
    CONNECTION_HANDOVER_FAILED = -8,

    /**
     * An authentication required request (e.g. phase 2) is invoked on a non-authenticated
     * connection
     */
    CONNECTION_NOT_AUTHENTICATED = -9,

    /** There is no pending connection with a matching token to authorize in phase 2 */
    NO_CONNECTION_TO_AUTHORIZE = -10,

    /** Invalid instance identifier */
    INVALID_INSTANCE_IDENTIFIER = -11,

    /** Failed to allocate memory */
    MEMORY_ALLOCATION_FAILED = -12,

    /** An instance which is pending deletion is trying to authenticate */
    INSTANCE_PENDING_DELETION = -13,

    /** A client which is pending deletion is trying to authorize */
    CLIENT_PENDING_DELETION = -14,

    /** Trying to complete authentication for an instance for which authentication is not started */
    AUTHENTICATION_NOT_STARTED = -15,

    /** Creation of the pVM instance's context in the secure storage is not allowed */
    INSTANCE_CONTEXT_CREATION_DENIED = -16,
}
