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

package android.hardware.neuralnetworks;

/**
 * A type that is used to represent a token / byte array data pair.
 */
@VintfStability
parcelable TokenValuePair {
    /**
     * A 32bit integer token. The token is created by combining the
     * extension prefix and enum defined within the extension.
     * The low {@link IDevice::EXTENSION_TYPE_LOW_BITS_TYPE} bits of the value
     * correspond to the hint within the extension and the high
     * {@link IDevice::EXTENSION_TYPE_HIGH_BITS_PREFIX} bits encode the "prefix", which maps
     * uniquely to the extension name. The sign bit is always 0.
     *
     * For example, if a token value is 0x7AAA000B and the corresponding
     * {@link ExtensionNameAndPrefix} contains an entry with prefix=0x7AAA and
     * name="vendor.test.test_extension", then the token should be interpreted as the hint
     * 0x000B of the extension named vendor.test.test_extension.
     */
    int token;
    /**
     * A byte array containing the raw data.
     */
    byte[] value;
}
