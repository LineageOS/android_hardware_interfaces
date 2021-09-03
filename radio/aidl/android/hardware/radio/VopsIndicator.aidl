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

package android.hardware.radio;

/**
 * Defines the values for VoPS indicator of NR as per 3gpp spec 24.501 sec 9.10.3.5
 */
@VintfStability
@Backing(type="byte")
enum VopsIndicator {
    /**
     * IMS voice over PS session not supported
     */
    VOPS_NOT_SUPPORTED,
    /**
     * IMS voice over PS session supported over 3GPP access
     */
    VOPS_OVER_3GPP,
    /**
     * IMS voice over PS session supported over non-3GPP access
     */
    VOPS_OVER_NON_3GPP,
}
