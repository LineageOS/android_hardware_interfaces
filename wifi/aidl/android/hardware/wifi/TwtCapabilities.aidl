/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * Target Wake Time (TWT) Capabilities supported.
 *
 * TWT allows Wi-Fi stations to manage activity in a network by scheduling to operate at different
 * times. This minimizes the contention and reduces the required amount of time that a station
 * utilizing a power management mode needs to be awake.
 *
 * IEEE 802.11ax standard defines two modes of TWT operation:
 *  - Individual TWT (default mode of operation if TWT requester is supported)
 *  - Broadcast TWT
 */
@VintfStability
parcelable TwtCapabilities {
    /**
     * Whether the TWT requester mode supported.
     */
    boolean isTwtRequesterSupported;
    /**
     * Whether the TWT responder mode supported.
     */
    boolean isTwtResponderSupported;
    /**
     * Whether the Broadcast TWT mode (TWT scheduling STA) supported.
     */
    boolean isBroadcastTwtSupported;
    /**
     * Whether supports Flexible TWT schedules.
     */
    boolean isFlexibleTwtScheduleSupported;
    /**
     * Minimum TWT wake duration in microseconds.
     */
    int minWakeDurationUs;
    /**
     * Maximum TWT wake duration in microseconds.
     */
    int maxWakeDurationUs;
    /**
     * Minimum TWT wake interval in microseconds.
     */
    long minWakeIntervalUs;
    /**
     * Maximum TWT wake interval in microseconds.
     */
    long maxWakeIntervalUs;
}
