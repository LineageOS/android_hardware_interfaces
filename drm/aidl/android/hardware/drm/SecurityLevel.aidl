/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.drm;

@VintfStability
@Backing(type="int")
enum SecurityLevel {
    /**
     * Unable to determine the security level
     */
    UNKNOWN,
    /**
     * Software-based whitebox crypto
     */
    SW_SECURE_CRYPTO,
    /**
     * Software-based whitebox crypto and an obfuscated decoder
     */
    SW_SECURE_DECODE,
    /**
     * DRM key management and crypto operations are performed within a
     * hardware backed trusted execution environment
     */
    HW_SECURE_CRYPTO,
    /**
     * DRM key management, crypto operations and decoding of content
     * are performed within a hardware backed trusted execution environment
     */
    HW_SECURE_DECODE,
    /**
     * DRM key management, crypto operations, decoding of content and all
     * handling of the media (compressed and uncompressed) is handled within
     * a hardware backed trusted execution environment.
     */
    HW_SECURE_ALL,
    /**
     * The default security level is defined as the highest security level
     * supported on the device.
     */
    DEFAULT,
}
