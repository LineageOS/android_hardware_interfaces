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
import android.hardware.security.authgraph.PubKey;

/**
 * The type that encapsulates a key. Key can be either a symmetric key or an asymmetric key.
 * If it is an asymmetric key, it is used for key exchange.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable Key {
    /**
     * If the Key is an asymmetric key, public key should be present.
     */
    @nullable PubKey pubKey;

    /**
     * Arc from the per-boot key to the payload key. The payload key is either the symmetric key
     * or the private key of an asymmetric key, based on the type of the key being created.
     * This is marked as optional because there are instances where only the public key is returned,
     * e.g. `init` method in the key exchange protocol.
     */
    @nullable Arc arcFromPBK;
}
