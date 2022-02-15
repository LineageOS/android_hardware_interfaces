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

import android.hardware.drm.CryptoSchemes;
import android.hardware.drm.SecurityLevel;
import android.hardware.drm.Uuid;

/**
 * IDrmFactory is the main entry point for interacting with a vendor's
 * drm HAL to create drm plugin instances. A drm plugin instance
 * creates drm sessions which are used to obtain keys for a crypto
 * session so it can decrypt protected video content.
 */
@VintfStability
interface IDrmFactory {
    /**
     * Create a drm plugin instance for the specified uuid and
     * scheme-specific initialization data.
     *
     * @param uuid uniquely identifies the drm scheme. See
     *     http://dashif.org/identifiers/protection for uuid assignments
     * @param appPackageName identifies the package name of the calling
     *     application.
     *
     * @return A DRM plugin instance if successful, or null if not created.
     *     Implicit error codes:
     *       + ERROR_DRM_CANNOT_HANDLE if the plugin cannot be created.
     */
    @nullable android.hardware.drm.IDrmPlugin createDrmPlugin(
            in Uuid uuid, in String appPackageName);

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
    @nullable android.hardware.drm.ICryptoPlugin createCryptoPlugin(
            in Uuid uuid, in byte[] initData);

    /**
     * Return vector of uuids identifying crypto schemes supported by
     * this HAL.
     *
     * @return List of uuids for which isCryptoSchemeSupported is true;
     *      each uuid can be used as input to createPlugin.
     */
    CryptoSchemes getSupportedCryptoSchemes();

}
