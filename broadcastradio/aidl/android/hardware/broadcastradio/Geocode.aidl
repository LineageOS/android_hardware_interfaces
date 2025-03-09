/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * Geographic code reprensenting location in alert message.
 *
 * <p>Geocode is mainly for information display instead of parsing on radio application side. See
 * ITU-T X.1303 bis for more info.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable Geocode {
    /**
     * Value name of a geographic code.
     *
     * <p>Value name are acronyms should be represented in all capital
     * letters without periods (e.g., SAME, FIPS, ZIP).
     */
    String valueName;

    /**
     * Value of a geographic code.
     */
    String value;
}
