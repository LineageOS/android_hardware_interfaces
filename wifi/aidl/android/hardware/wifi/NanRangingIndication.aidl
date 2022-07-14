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
 * Ranging in the context of discovery session indication controls. Controls
 * the frequency of ranging-driven |IWifiNanIfaceEventCallback.eventMatch|.
 */
@VintfStability
@Backing(type="int")
enum NanRangingIndication {
    CONTINUOUS_INDICATION_MASK = 1 << 0,
    INGRESS_MET_MASK = 1 << 1,
    EGRESS_MET_MASK = 1 << 2,
}
