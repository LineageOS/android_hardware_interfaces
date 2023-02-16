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

package android.hardware.wifi;

/**
 * WifiChipCapabilities captures various Wifi chip capability params.
 */
@VintfStability
parcelable WifiChipCapabilities {
    /**
     * Maximum number of links supported by the chip for MLO association.
     */
    int maxMloAssociationLinkCount;
    /**
     * Maximum number of Simultaneous Transmit and Receive (STR) links used
     * in Multi-Link Operation. The maximum number of STR links used can be
     * different from the maximum number of radios supported by the chip.
     *
     * This is a static configuration of the chip.
     */
    int maxMloStrLinkCount;
    /**
     * Maximum number of concurrent TDLS sessions that can be enabled
     * by framework via ISupplicantStaIface#initiateTdlsSetup().
     */
    int maxConcurrentTdlsSessionCount;
}
