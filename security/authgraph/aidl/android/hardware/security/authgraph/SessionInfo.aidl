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
import android.hardware.security.authgraph.SessionIdSignature;

/**
 * Session information returned as part of authenticated key exchange.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable SessionInfo {
    /**
     * The arcs that encrypt the two derived symmetric encryption keys (for two-way communication).
     * The encryption key is the party's per-boot key.
     */
    Arc[2] sharedKeys;

    /**
     * The value of the session id computed by the two parties during the authenticate key
     * exchange. Apart from the usage of the session id by the two peers, session id is also useful
     * to verify (by a third party) that the key exchange was successful.
     */
    byte[] sessionId;

    /**
     * The signature over the session id, created by the party who computed the session id.
     *
     * If there is one or more `DiceChainEntry` in the `ExplicitKeyDiceCertChain` of the party's
     * identity, the signature is verified with the public key in the leaf of the chain of
     * DiceChainEntries (i.e the public key in the last of the array of DiceChainEntries).
     * Otherwise, the signature is verified with the `DiceCertChainInitialPayload`.
     */
    SessionIdSignature signature;
}
