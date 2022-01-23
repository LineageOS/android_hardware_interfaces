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

package android.hardware.nfc;

/**
 * Vendor Specific Proprietary Protocol & Discovery Configuration.
 * Set to 0xFF if not supported.
 * discovery* fields map to "RF Technology and Mode" in NCI Spec
 * protocol* fields map to "RF protocols" in NCI Spec
 */
@VintfStability
parcelable ProtocolDiscoveryConfig {
    byte protocol18092Active;
    byte protocolBPrime;
    byte protocolDual;
    byte protocol15693;
    byte protocolKovio;
    byte protocolMifare;
    byte discoveryPollKovio;
    byte discoveryPollBPrime;
    byte discoveryListenBPrime;
}
