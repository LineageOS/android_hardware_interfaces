/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

/**
 * The channel selection algorithm for non-mode-0 steps
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.42 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
@Backing(type="byte")
enum ChannelSelectionType {
    /**
     * Use Channel Selection Algorithm #3b for non-mode-0 CS steps
     */
    ALOGRITHM_3B = 0x00,
    /**
     * Use Channel Selection Algorithm #3c for non-mode-0 CS steps
     */
    ALOGRITHM_3C = 0x01,
}
