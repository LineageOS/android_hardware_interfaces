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
package android.hardware.security.see.hwcrypto.types;

/*
 * Additional characteristics and permissions of the key.
 */
enum KeyPermissions {
    /*
     * Key can be wrapped by an ephemeral key.
     */
    ALLOW_EPHEMERAL_KEY_WRAPPING,

    /*
     * Key can be wrapped by a hardware key. Notice that ephemeral keys cannot be wrapped by
     * hardware keys.
     */
    ALLOW_HARDWARE_KEY_WRAPPING,

    /*
     * Key can be wrapped by a portable key. Notice that neither ephemeral keys nor hardware keys
     * can be wrapped by portable keys.
     */
    ALLOW_PORTABLE_KEY_WRAPPING,
}
