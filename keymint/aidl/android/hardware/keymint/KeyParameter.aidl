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

package android.hardware.keymint;


import android.hardware.keymint.Algorithm;
import android.hardware.keymint.BlockMode;
import android.hardware.keymint.Digest;
import android.hardware.keymint.EcCurve;
import android.hardware.keymint.HardwareAuthenticatorType;
import android.hardware.keymint.KeyDerivationFunction;
import android.hardware.keymint.KeyOrigin;
import android.hardware.keymint.KeyPurpose;
import android.hardware.keymint.PaddingMode;
import android.hardware.keymint.SecurityLevel;
import android.hardware.keymint.Tag;


/**
 * Identifies the key authorization parameters to be used with keyMint.  This is usually
 * provided as an array of KeyParameters to IKeyMintDevice or Operation.
 *
 * TODO(seleneh): Union was not supported in aidl when this cl is first drafted.  So we just had
 * the Tags, and bool, int, long, int[], and we will cast to the appropate types base on the
 * Tag value.  We need to update this defination to distingish Algorithm, BlockMode,
 * PaddingMode, KeyOrigin...etc later, as union support is recently added to aidl.
 * b/173253030
 */
@VintfStability
parcelable KeyParameter {
    /**
     * Identify what type of key parameter this parcelable actually holds, and based on the type
     * of tag is int, long, bool, or byte[], one of the fields below will be referenced.
     */
    Tag tag;

    boolean boolValue;
    int integer;
    long longInteger;
    // TODO: change this to nullable.
    byte[] blob;
}
