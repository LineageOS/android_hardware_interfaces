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

import android.hardware.security.authgraph.PlainPubKey;
import android.hardware.security.authgraph.SignedPubKey;

/**
 * The enum type representing the public key of an asymmetric key pair.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
union PubKey {
    /**
     * Plain public key material encoded as a COSE_Key.
     */
    PlainPubKey plainKey;

    /**
     * Public key signed with the long term signing key of the party.
     */
    SignedPubKey signedKey;
}
