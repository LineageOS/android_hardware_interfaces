/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.types.HmacOperationParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricAuthOperationParameters;
import android.hardware.security.see.hwcrypto.types.SymmetricOperationParameters;

/*
 * Type that describes the parameters for the different operations that can be performed.
 */
@VintfStability
union OperationParameters {
    /*
     * Parameters for authenticated symmetric cryptography (AES GCM).
     */
    SymmetricAuthOperationParameters symmetricAuthCrypto;

    /*
     * Parameters for non-authenticated symmetric cryptography (AES/TDES).
     */
    SymmetricOperationParameters symmetricCrypto;

    /*
     * Parameters for hash based message authenticated code operations.
     */
    HmacOperationParameters hmac;
}
