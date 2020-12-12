/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.security.keymint;

import android.hardware.security.keymint.KeyParameter;

/**
 * KeyCharacteristics defines the attributes of a key, including cryptographic parameters, and usage
 * restrictions.  It consits of two vectors of KeyParameters, one for "softwareEnforced" attributes
 * and one for "hardwareEnforced" attributes.
 *
 * KeyCharacteristics objects are returned by generateKey, importKey, importWrappedKey and
 * getKeyCharacteristics.  The IKeyMintDevice secure environment is responsible for allocating the
 * parameters, all of which are Tags with associated values, to the correct vector.  The
 * hardwareEnforced vector must contain only those attributes which are enforced by secure hardware.
 * All others should be in the softwareEnforced vector.  See the definitions of individual Tag enums
 * for specification of which must be hardware-enforced, which may be software-enforced and which
 * must never appear in KeyCharacteristics.
 */
@VintfStability
parcelable KeyCharacteristics {
    /* TODO(seleneh) get rid of the software enforced in keymint.  replace hardware enforced with
     * tee enforced and strongbox enforced.
     */
    KeyParameter[] softwareEnforced;
    KeyParameter[] hardwareEnforced;
}
