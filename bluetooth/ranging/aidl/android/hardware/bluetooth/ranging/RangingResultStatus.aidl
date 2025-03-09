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

@VintfStability
@Backing(type="byte")
enum RangingResultStatus {
    SUCCESS = 0x00,
    /**
     * The procedure of the initiator was aborted
     */
    FAIL_INITIATOR_ABORT = 0x01,
    /**
     * The procedure of the reflector was aborted
     */
    FAIL_REFLECTOR_ABORT = 0x02,
    /**
     * The procedure of both the initiator and the reflector were aborted
     */
    FAIL_BOTH_ABORT = 0x03,
    FAIL_UNSPECIFIED = 0xFFu8,
}
