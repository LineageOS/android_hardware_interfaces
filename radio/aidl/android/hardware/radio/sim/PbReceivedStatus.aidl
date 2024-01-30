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

package android.hardware.radio.sim;

/**
 * Enum representing the status of the received PB indication.
 * @hide
 */
@VintfStability
@Backing(type="byte")
@JavaDerive(toString=true)
enum PbReceivedStatus {
    /**
     * Indicates that retrieval is fine.
     */
    PB_RECEIVED_OK = 1,
    /**
     * Indicates that an error happened. In general, the process can't be restored soon.
     */
    PB_RECEIVED_ERROR = 2,
    /**
     * Indicates that the process is interrupted. In this case, the modem might need resources and
     * interrupt the current process, or it is timed out to receive all indications, and client can
     * retry soon.
     */
    PB_RECEIVED_ABORT = 3,
    /**
     * Indicates that the whole process is finished with a full chunk of phonebook data, meaning
     * this is the last indication with the remaining data.
     */
    PB_RECEIVED_FINAL = 4,
}
