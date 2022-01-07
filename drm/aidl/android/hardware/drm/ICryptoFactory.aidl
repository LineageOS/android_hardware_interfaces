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

import android.hardware.drm.Uuid;

/**
 * ICryptoFactory is the main entry point for interacting with a vendor's
 * crypto HAL to create crypto plugins.

 * Crypto plugins create crypto sessions which are used by a codec to decrypt
 * protected video content.
 */
@VintfStability
interface ICryptoFactory {
    /**
     * Create a crypto plugin for the specified uuid and scheme-specific
     * initialization data.
     *
     * @param uuid uniquely identifies the drm scheme. See
     *     http://dashif.org/identifiers/protection for uuid assignments
     *
     * @param initData scheme-specific init data.
     *
     * @return A crypto plugin instance if successful, or null if not created.
     */
    @nullable android.hardware.drm.ICryptoPlugin createPlugin(
            in Uuid uuid, in byte[] initData);

    /**
     * Determine if a crypto scheme is supported by this HAL.
     *
     * @param uuid identifies the crypto scheme in question
     * @return must be true only if the scheme is supported
     */
    boolean isCryptoSchemeSupported(in Uuid uuid);
}
