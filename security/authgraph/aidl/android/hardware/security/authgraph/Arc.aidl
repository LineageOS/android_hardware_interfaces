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
 * This is the definition of the data format of an Arc.
 * @hide
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable Arc {
    /**
     * The messages exchanged between the domains in the AuthGraph protocol are called Arcs.
     * An arc is simply AES-GCM. Encryption of a payload P with a key K and additional
     * authentication data (AAD) D: (i.e. Arc = Enc(K, P, D)).
     *
     * Data is CBOR-encoded according to the `Arc` CDDL definition in Arc.cddl.
     */
    byte[] arc;
}
