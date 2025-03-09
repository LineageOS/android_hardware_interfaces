/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.security.see.authmgr;

import android.hardware.security.see.authmgr.DiceChainEntry;
import android.hardware.security.see.authmgr.DicePolicy;

/**
 * This contains the DICE certificate and the DICE policy created for the client by the AuthMgr FE.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable DiceLeafArtifacts {
    DiceChainEntry diceLeaf;
    DicePolicy diceLeafPolicy;
}
