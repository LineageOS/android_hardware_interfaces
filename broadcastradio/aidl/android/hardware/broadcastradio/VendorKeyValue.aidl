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

/**
 * A key-value pair for vendor-specific information to be passed as-is through
 * Android framework to the front-end application.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable VendorKeyValue {
    /**
     * Key must start with unique vendor Java-style namespace,
     * eg. 'com.somecompany.parameter1'.
     */
    String key;

    /**
     * Value must be passed through the framework without any changes.
     * Format of this string can vary across vendors.
     */
    String value;
}
