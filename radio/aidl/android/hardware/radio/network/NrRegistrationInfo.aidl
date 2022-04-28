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

package android.hardware.radio.network;

import android.hardware.radio.network.EutranRegistrationInfo;
import android.hardware.radio.network.NrVopsInfo;

@VintfStability
parcelable NrRegistrationInfo {
    /** SMS over NAS is allowed - TS 24.501 9.11.3.6. */
    const int EXTRA_SMS_OVER_NAS_ALLOWED = 1 << 0;

    /** Registered for emergency services - TS 24.501 9.11.3.6. */
    const int EXTRA_REGISTERED_FOR_EMERGENCY = 1 << 1;

    /** Network capabilities for voice over PS services. */
    NrVopsInfo ngranNrVopsInfo;

    /** Values are bitwise ORs of EXTRA_* constants */
    int extraInfo;
}
