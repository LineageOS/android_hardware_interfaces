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
 * Enum describing the different types of protected buffers. Protected buffers are named by its
 * corresponding use case and its underlaying implementation is platform dependant.
 */
@VintfStability
enum ProtectionId {
    /*
     * ProtectionID used by HwCrypto to enable Keys that can be used for Widevine video buffers.
     * These buffers should not be readable by non-trusted entities and HwCrypto should not allow
     * any read access to them through its interface.
     */
    WIDEVINE_OUTPUT_BUFFER = 1,
}
