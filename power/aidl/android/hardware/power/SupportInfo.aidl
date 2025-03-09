/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.power;

import android.hardware.power.Boost;
import android.hardware.power.Mode;
import android.hardware.power.SessionHint;
import android.hardware.power.SessionMode;

/**
 * Tells clients the status of various PowerHAL features in a single call.
 * SupportInfo consists of several bitsets, where each bit from the left
 * corresponds to the support status of that same value of that enum index.
 *
 * For "Boost", having the first bit set would mean "INTERACTION"
 * boost is supported, having the second bit set would mean
 * "DISPLAY_UPDATE_IMMINENT" is supported, and so on. The expectation
 * is that a client should be able to index the bitset like
 * "(supportInfo.boosts >> Boost::AUDIO_LAUNCH) % 2" and it should return
 * the support value of Boost::AUDIO_LAUNCH. This pattern is the same for
 * all four support bitsets.
 */
@VintfStability
parcelable SupportInfo {
    /**
     * Boolean representing whether hint sessions are supported on this device.
     */
    boolean usesSessions;

    /**
     * The set of "Boost" enum values that are supported by this device,
     * each bit should correspond to a value of the "Boost.aidl" enum.
     */
    long boosts;

    /**
     * The set of "Mode" enum values that are supported by this device,
     * each bit should correspond to a value of the "Mode.aidl" enum.
     */
    long modes;

    /**
     * The set of "SessionHint" enum values that are supported by this device,
     * each bit should correspond to a value of the "SessionHint.aidl" enum.
     */
    long sessionHints;

    /**
     * The set of "SessionMode" enum values that are supported by this device,
     * each bit should correspond to a value of the "SessionMode.aidl" enum.
     */
    long sessionModes;

    /**
     * The set of "SessionTag" enum values that are supported by this device,
     * each bit should correspond to a value of the "SessionTag.aidl" enum.
     */
    long sessionTags;

    /**
     * Parcel detailing support info for receiving additional frame composition
     * data when sessions are associated with frame producers.
     */
    CompositionDataSupportInfo compositionData;

    /**
     *  Parcel detailing support info for headroom information.
     */
    HeadroomSupportInfo headroom;

    @VintfStability
    parcelable CompositionDataSupportInfo {
        /**
         * Whether the sendCompositionData and sendCompositionUpdate APIs are
         * supported on this device. The rest of the fields on this parcelable
         * are ignored if this is false.
         */
        boolean isSupported;

        /**
         * Whether to disable sending relevant GPU fence file descriptors along with
         * timing information when the frame callback happens.
         */
        boolean disableGpuFences;

        /**
         * The maximum number of updates to batch before sending. This can be ignored
         * if "overrideIfUrgent" is set. Setting to a value less than or equal to 1
         * disables batching entirely.
         */
        int maxBatchSize;

        /**
         * Whether to ignore important notifications such as FPS changes and frame
         * deadline misses, and always send maximum size batches. By default, the
         * framework will send batches early if these important events happen.
         */
        boolean alwaysBatch;
    }

    @VintfStability
    parcelable HeadroomSupportInfo {
        /**
         * Whether the CPU headroom feature is supported.
         */
        boolean isCpuSupported;

        /**
         * Whether the GPU headroom feature is supported.
         */
        boolean isGpuSupported;

        /**
         * Minimum polling interval for calling getCpuHeadroom in milliseconds
         *
         * The getCpuHeadroom API may return cached result if called more frequent
         * than the interval.
         */
        int cpuMinIntervalMillis;

        /**
         * Minimum polling interval for calling getGpuHeadroom in milliseconds.
         *
         * The getGpuHeadroom API may return cached result if called more frequent
         * than the interval.
         */
        int gpuMinIntervalMillis;
    }
}
