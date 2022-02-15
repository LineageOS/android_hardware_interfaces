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

package android.hardware.wifi.supplicant;

/**
 * Vendor specific Error Indication for WPS event messages.
 */
@VintfStability
@Backing(type="int")
enum WpsErrorIndication {
    NO_ERROR = 0,
    SECURITY_TKIP_ONLY_PROHIBITED = 1,
    SECURITY_WEP_PROHIBITED = 2,
    AUTH_FAILURE = 3,
}
