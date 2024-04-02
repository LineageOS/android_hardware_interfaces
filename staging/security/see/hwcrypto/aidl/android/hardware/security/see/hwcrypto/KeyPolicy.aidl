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

import android.hardware.security.see.hwcrypto.types.KeyLifetime;
import android.hardware.security.see.hwcrypto.types.KeyPermissions;
import android.hardware.security.see.hwcrypto.types.KeyType;
import android.hardware.security.see.hwcrypto.types.KeyUse;

/*
 * Parcelable that specified how a key can be used.
 */
parcelable KeyPolicy {
    /*
     * Enum specifying the operations the key can perform (encryption, decryption, etc.).
     */
    KeyUse usage;

    /*
     * Enum that describes the key lifetime characteristics. See the docstring on
     * <code>KeyLifetime</code> for more details.
     */
    KeyLifetime keyLifetime = KeyLifetime.EPHEMERAL;

    /*
     * Additional permissions of the key (e.g. key types allowed to wrap the key, boot binding,
     * etc.). See the docstring on <code>KeyPermissions</code> for more details.
     */
    KeyPermissions[] keyPermissions;

    /*
     * Key can be used to wrap or derive other keys.
     */
    boolean keyManagementKey;

    /*
     * Enum that specifies the key type.
     */
    KeyType keyType = KeyType.AES_256_GCM;
}
