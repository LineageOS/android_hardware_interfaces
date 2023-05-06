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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SlotPortMapping {
    /**
     * Physical slot id is the index of the slots
     **/
    int physicalSlotId;
    /**
     * PortId is the id (enumerated value) for the associated port available on the SIM.
     * Example:
     * if eUICC1 supports 2 ports, then the portId is numbered 0,1.
     * if eUICC2 supports 4 ports, then the portId is numbered: 0,1,2,3.
     * Each portId is unique within a UICC, but not necessarily unique across UICC’s.
     * SEP(Single enabled profile) eUICC and non-eUICC will only have portId 0.
     **/
    int portId;
}
