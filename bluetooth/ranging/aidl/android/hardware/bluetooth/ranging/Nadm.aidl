/*
 * Copyright 2023 The Android Open Source Project
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
enum Nadm {
    ATTACK_IS_EXTREMELY_UNLIKELY = 0x00,
    ATTACK_IS_VERY_UNLIKELY = 0x01,
    ATTACK_IS_UNLIKELY = 0x02,
    ATTACK_IS_POSSIBLE = 0x03,
    ATTACK_IS_LIKELY = 0x04,
    ATTACK_IS_VERY_LIKELY = 0x05,
    ATTACK_IS_EXTREMELY_LIKELY = 0x06,
    UNKNOWN = 0xFFu8,
}
