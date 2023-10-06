/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.automotive.ivn;

/**
 * Hardware Identifiers for an Android device.
 *
 * <p>These identifiers are embedded in the ID attestation certificate and are
 * used to restrict what devices this device can connect to. All fields are
 * optional but at least one of the fields must be specified.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable HardwareIdentifiers {
    /**
     * Optional brand name, as returned by {@code Build.BRAND} in Android.
     *
     * If unspecified, we assume the other device has the same brand name as this device.
     */
    @nullable String brandName;
    /**
     * Optional brand name, as returned by {@code Build.DEVICE} in Android.
     *
     * If unspecified, we assume the other device has the same device name as this device.
     */
    @nullable String deviceName;
    /**
     * Optional model name, as returned by {@code Build.PRODUCT} in Android.
     *
     * If unspecified, we assume the other device has the same product name as this device.
     */
    @nullable String productName;
    /**
     * Optional manufacturer name, as returned by {@code Build.MANUFACTURER} in Android.
     *
     * If unspecified, we assume the other device has the same manufacturer name as this device.
     */
    @nullable String manufacturerName;
    /**
     * Optional model name, as returned by {@code Build.MODEL} in Android.
     *
     * If unspecified, we assume the other device has the same model name as this device.
     */
    @nullable String modelName;
    /**
     * Optional serial number.
     *
     * If unspecified, we allow the endpoint to have any serial number.
     */
    @nullable String serialNumber;
}
