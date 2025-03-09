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

package android.hardware.wifi;

import android.hardware.wifi.PasnConfig;

/**
 * RTT secure configuration.
 */
@VintfStability
parcelable RttSecureConfig {
    /**
     * Pre-Association Security Negotiation (PASN) configuration.
     */
    PasnConfig pasnConfig;
    /**
     * Enable secure HE-LTF (High Efficiency Long Training Field).
     */
    boolean enableSecureHeLtf;
    /**
     * Enable Ranging frame protection.
     */
    boolean enableRangingFrameProtection;
    /**
     * Comeback cookie is an opaque sequence of octets retrieved from |RttResult|.
     */
    @nullable byte[] pasnComebackCookie;
}
