/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Copyright 2021 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb;

import android.hardware.uwb.IUwbChip;

/**
 * HAL Interface for UWB (Ultrawideband) subsystem.
 * https://en.wikipedia.org/wiki/Ultra-wideband.
 */
@VintfStability
interface IUwb {
    /**
     * Returns list of IUwbChip instance names representing each UWB chip on the device.
     */
    List<String> getChips();

    /**
     * Returns IUwbChip instance corresponding to the name.
     *
     * @param Unique identifier of the chip.
     */
    IUwbChip getChip(String name);
}
