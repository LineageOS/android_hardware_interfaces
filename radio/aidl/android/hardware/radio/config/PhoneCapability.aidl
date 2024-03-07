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

package android.hardware.radio.config;

/**
 * Phone capability which describes the data connection capability of modem.
 * It's used to evaluate possible phone config change, for example from single
 * SIM device to multi-SIM device.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable PhoneCapability {
    /**
     * maxActiveData defines how many logical modems can have
     * PS attached simultaneously. For example, for L+L modem it
     * should be 2.
     */
    byte maxActiveData;
    /**
     * maxActiveData defines how many logical modems can have
     * internet PDN connections simultaneously. For example, for L+L
     * DSDS modem it’s 1, and for DSDA modem it’s 2.
     */
    byte maxActiveInternetData;
    /**
     * Whether modem supports both internet PDN up so
     * that we can do ping test before tearing down the
     * other one.
     */
    boolean isInternetLingeringSupported;
    /**
     * List of logical modem IDs.
     */
    byte[] logicalModemIds;
}
