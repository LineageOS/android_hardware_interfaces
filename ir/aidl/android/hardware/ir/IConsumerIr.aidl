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

package android.hardware.ir;

import android.hardware.ir.ConsumerIrFreqRange;

@VintfStability
interface IConsumerIr {
    /**
     * Enumerates which frequencies the IR transmitter supports.
     *
     * @return - an array of all supported frequency ranges.
     */
    ConsumerIrFreqRange[] getCarrierFreqs();

    /**
     * Sends an IR pattern at a given frequency in HZ.
     * This call must return when the transmit is complete or encounters an error.
     *
     * @param carrierFreq - Frequency of the transmission in HZ.
     *
     * @param pattern - Alternating series of on and off periods measured in
     * microseconds. The carrier should be turned off at the end of a transmit
     * even if there are an odd number of entries in the pattern array.
     *
     * @throws EX_UNSUPPORTED_OPERATION when the frequency is not supported.
     */
    void transmit(in int carrierFreqHz, in int[] pattern);
}
