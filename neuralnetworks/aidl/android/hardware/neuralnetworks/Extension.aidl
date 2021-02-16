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

import android.hardware.neuralnetworks.ExtensionOperandTypeInformation;

/**
 * Information about an extension.
 */
@VintfStability
parcelable Extension {
    /**
     * The extension name.
     *
     * The name must consist of lowercase latin letters, numbers, periods, and underscore signs. The
     * name must contain at least one period.
     *
     * The name must start with the reverse domain name of the vendor.
     *
     * Example: com.google.test_extension
     */
    String name;
    /**
     * Information about operand types defined by the extension.
     */
    ExtensionOperandTypeInformation[] operandTypes;
}
