/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.security.dice;

/**
 * This parcelable represents a Signature. It is used as return value of IDiceNode::sign.
 *
 * @hide
 */
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
@VintfStability
parcelable Signature {
    /**
     * The RFC 8032 PureEd25519 signature.
     * @see <a href="https://datatracker.ietf.org/doc/html/rfc8032">RFC 8032</a>
     */
    byte[] data;
}
