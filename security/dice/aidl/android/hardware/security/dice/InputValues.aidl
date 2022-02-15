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

import android.hardware.security.dice.Config;
import android.hardware.security.dice.Mode;

/**
 * DICE input values for certificate and CDI generation.
 *
 * @see <a
 *         href="https://pigweed.googlesource.com/open-dice/+/refs/heads/main/docs/specification.md#input-values">
 *     Open-dice input-values
 * </a>
 * @hide
 */
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
@VintfStability
parcelable InputValues {
    /**
     * The target code hash. Must be exactly 64 bytes.
     */
    byte[64] codeHash;
    /**
     * The configuration data.
     */
    Config config;
    /**
     * The authority hash. Must be exactly 64 bytes. Must be all zero if unused.
     */
    byte[64] authorityHash;
    /**
     * Optional free form authorityDescriptor.
     */
    @nullable byte[] authorityDescriptor;
    /**
     * The mode of operation. Normal, Debug, Maintenance, or not initialized.
     */
    Mode mode = Mode.NOT_INITIALIZED;
    /**
     * Optional hidden values. Must be exactly 64 bytes. Must be all zero if unused.
     */
    byte[64] hidden;
}
