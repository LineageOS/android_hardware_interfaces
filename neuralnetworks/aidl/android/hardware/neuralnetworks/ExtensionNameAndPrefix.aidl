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

package android.hardware.neuralnetworks;

/**
 * The mapping between extension names and prefixes of operand and operation type values.
 *
 * An operand or operation whose numeric type value is above {@link IDevice::OPERAND_TYPE_BASE_MAX}
 * or {@link IDevice::OPERATION_TYPE_BASE_MAX} respectively should be interpreted as an extension
 * operand/operation. The low {@link IDevice::EXTENSION_TYPE_LOW_BITS_TYPE} bits of the value
 * correspond to the type ID within the extension and the high
 * {@link IDevice::EXTENSION_TYPE_HIGH_BITS_PREFIX} bits encode the "prefix", which maps uniquely to
 * the extension name. The sign bit is always 0.
 *
 * For example, if a model contains an operation whose value is 0x7AAABBBB and extensionNameToPrefix
 * contains an entry with prefix=0x7AAA and name="vendor.test.test_extension", then the operation
 * should be interpreted as the operation 0xBBBB of the extension named vendor.test.test_extension.
 *
 * This is a one-to-one correspondence. That is, there must be at most one prefix corresponding to
 * each extension name and at most one extension name corresponding to each prefix.
 */
@VintfStability
parcelable ExtensionNameAndPrefix {
    /**
     * The extension name.
     *
     * See {@link Extension::name} for the format specification.
     */
    String name;
    /**
     * The extension prefix. Only the lowest 15 bits are used, so the value must be less than 32768.
     */
    char prefix;
}
