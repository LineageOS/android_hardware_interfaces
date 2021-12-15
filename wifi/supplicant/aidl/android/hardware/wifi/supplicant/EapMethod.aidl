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
 * Possble values for EapMethod param.
 */
@VintfStability
@Backing(type="int")
enum EapMethod {
    PEAP = 0,
    TLS = 1,
    TTLS = 2,
    PWD = 3,
    SIM = 4,
    AKA = 5,
    AKA_PRIME = 6,
    WFA_UNAUTH_TLS = 7,
}
