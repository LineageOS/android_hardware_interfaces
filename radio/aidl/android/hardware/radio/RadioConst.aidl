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

package android.hardware.radio;

@VintfStability
@Backing(type="int")
enum RadioConst {
    CDMA_ALPHA_INFO_BUFFER_LENGTH = 64,
    CDMA_NUMBER_INFO_BUFFER_LENGTH = 81,
    MAX_RILDS = 3,
    MAX_SOCKET_NAME_LENGTH = 6,
    MAX_CLIENT_ID_LENGTH = 2,
    MAX_DEBUG_SOCKET_NAME_LENGTH = 12,
    MAX_QEMU_PIPE_NAME_LENGTH = 11,
    MAX_UUID_LENGTH = 64,
    CARD_MAX_APPS = 8,
    CDMA_MAX_NUMBER_OF_INFO_RECS = 10,
    SS_INFO_MAX = 4,
    NUM_SERVICE_CLASSES = 7,
    NUM_TX_POWER_LEVELS = 5,
    RADIO_ACCESS_SPECIFIER_MAX_SIZE = 8,
    /**
     * No P2 value is provided
     */
    P2_CONSTANT_NO_P2 = -1,
}
