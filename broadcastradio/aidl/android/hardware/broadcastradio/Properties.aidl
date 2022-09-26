/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.IdentifierType;
import android.hardware.broadcastradio.VendorKeyValue;

/**
 * Properties of a given broadcast radio module.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable Properties {
    /**
     * A company name who made the radio module. Must be a valid, registered
     * name of the company itself.
     *
     * It must be opaque to the Android framework.
     */
    String maker;

    /**
     * A product name. Must be unique within the company.
     *
     * It must be opaque to the Android framework.
     */
    String product;

    /**
     * Version of the hardware module.
     *
     * It must be opaque to the Android framework.
     */
    String version;

    /**
     * Hardware serial number (for subscription services).
     *
     * It must be opaque to the Android framework.
     */
    String serial;

    /**
     * A list of supported {@link IdentifierType} values.
     *
     * If an identifier is supported by radio module, it means it can use it for
     * tuning to ProgramSelector with either primary or secondary Identifier of
     * a given type.
     *
     * Support for VENDOR identifier type does not guarantee compatibility, as
     * other module properties (implementor, product, version) must be checked.
     */
    IdentifierType[] supportedIdentifierTypes;

    /**
     * Vendor-specific information.
     *
     * It may be used for extra features, not supported by the platform,
     * for example: com.me.preset-slots=6; com.me.ultra-hd-capable={@code false}.
     */
    VendorKeyValue[] vendorInfo;
}
