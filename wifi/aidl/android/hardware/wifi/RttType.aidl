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
 * RTT Types.
 */
@VintfStability
@Backing(type="int")
enum RttType {
    ONE_SIDED = 1,
    /**
     * Two-sided RTT 11mc type.
     *
     * Note: TWO_SIDED was used for IEEE 802.11mc. Use TWO_SIDED_11MC for IEEE 802.11mc instead.
     */
    TWO_SIDED = 2,
    /**
     * Two-sided RTT 11mc type is same as two-sided.
     */
    TWO_SIDED_11MC = TWO_SIDED,
    /**
     * Two-sided RTT 11az non trigger based (non-TB) type.
     */
    TWO_SIDED_11AZ_NTB = 3,
}
