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
 * A DICE certificate chain following the Boot Certificate Chain (BCC) specification.
 * @hide
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
parcelable Bcc {
    /**
     * The DICE certificate chain CBOR encoded following the BCC specification. The CDDL
     * specification for BCC can be found here [1].
     *
     * @see <a
     *         href="https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/keymint/aidl/android/hardware/security/keymint/ProtectedData.aidl">
     *    BCC CDDL specification
     * </a>
     */
    byte[] data;
}
