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

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.Identity;
import android.hardware.security.authgraph.Key;

/**
 * Session initiation information returned as part of authenticated key exchange.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable SessionInitiationInfo {
    /**
     * An ephemeral EC key created for the Elliptic-curve Diffie-Hellman (ECDH) process.
     */
    Key key;

    /**
     * The identity of the party who creates this `SessionInitiationInfo`.
     */
    Identity identity;

    /**
     * Nonce (a cryptographic random number of 16 bytes) specific to this session.
     * The nonce serves three purposes:
     * 1. freshness of key exchange
     * 2. creating a session id (a publicly known value related to the exchanged keys)
     * 3. usage as salt into the HKDF-EXTRACT function during key derivation from the Diffie-Hellman
     *    shared secret
     */
    byte[] nonce;

    /**
     * The protocol version (i.e. AIDL version) - This is used to prevent version downgrade attacks
     * as follows:
     * 1. In `create`, the source advertises the latest protocol version supported by the source,
     *    which is given as input to the `init` call on the sink in the input parameter:
     *    `peerVersion`.
     * 2. In `init`, the sink includes the `peerVersion` in the inputs to the derivation of the
     *    shared keys. Then the sink returns the latest protocol version supported by the sink,
     *    which is given as input to the `finish` call on the source in the input parameter:
     *    `peerVersion`.
     * 3. In `finish`, the source first checks whether the sink's version is equal or less than the
     *    source's version and includes in the source's version in the inputs to the derivation of
     *    the shared keys.
     * Analysis: if an attacker-in-the-middle wanted the two parties to use an older (vulnerable)
     * version of the protocol, they can invoke `init` with a version that is lower than the version
     * advertised by the source in `create`. However, since both parties include the source's
     * version in the inputs to the derivation of the shared keys, the two parties won't end up with
     * the same shared keys in the presence of such an attack. This is detected when checking the
     * signature on the session id in `finish`, at which point the protocol aborts. Therefore,
     * an attacker cannot successfully launch a version downgrade attack on this protocol.
     */
    int version;
}
