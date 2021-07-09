/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * The bits of EventFlag in FMQ (Fast message queue) are used by client to
 * notify HAL the status change.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxQueueNotifyBits {
    /**
     * client writes data and notify HAL the data is ready.
     */
    DATA_READY = 1 << 0,

    /**
     * client reads data and notify HAL the data is consumed.
     */
    DATA_CONSUMED = 1 << 1,
}
