/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

/**
 * AuthGraph error codes. Aidl will return these error codes as service specific errors in
 * EX_SERVICE_SPECIFIC.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum Error {
    /* Success */
    OK = 0,
    /* Invalid peer nonce for key exchange */
    INVALID_PEER_NONCE = -1,
    /* Invalid key exchange public key by the peer */
    INVALID_PEER_KE_KEY = -2,
    /* Invalid identity of the peer */
    INVALID_IDENTITY = -3,
    /* Invalid certificate chain in the identity of the peer */
    INVALID_CERT_CHAIN = -4,
    /* Invalid signature by the peer */
    INVALID_SIGNATURE = -5,
    /* Invalid key exchange key created by a particular party themselves to be used as a handle */
    INVALID_KE_KEY = -6,
    /* Invalid public key in the `Key` struct */
    INVALID_PUB_KEY_IN_KEY = -7,
    /* Invalid private key arc in the `Key` struct */
    INVALID_PRIV_KEY_ARC_IN_KEY = -8,
    /* Invalid shared key arcs */
    INVALID_SHARED_KEY_ARCS = -9,
    /* Memory allocation failed */
    MEMORY_ALLOCATION_FAILED = -10,
    /* The protocol version negotiated with the sink is incompatible */
    INCOMPATIBLE_PROTOCOL_VERSION = -11,
}
