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

/*
 * EapErrorCode: Error code for EAP or EAP Method as per RFC-4186
 */
@VintfStability
@Backing(type="int")
enum EapErrorCode {
    SIM_GENERAL_FAILURE_AFTER_AUTH = 0,
    SIM_TEMPORARILY_DENIED = 1026,
    SIM_NOT_SUBSCRIBED = 1031,
    SIM_GENERAL_FAILURE_BEFORE_AUTH = 16384,
    SIM_VENDOR_SPECIFIC_EXPIRED_CERT = 16385,
}
