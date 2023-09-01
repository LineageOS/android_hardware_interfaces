/**
 * Copyright 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

@VintfStability
parcelable VrrConfig {
    /**
     * The minimal time (in nanoseconds) that needs to pass between the previously presented frame
     * and when the next frame can be presented.
     */
    int minFrameIntervalNs;

    /**
     * An optional mapping between frame intervals, and the physical display refresh period on
     * average. This provides useful information to the framework when picking a specific frame rate
     * (which is a divisor of the vsync rate) about the real display refresh rate, which could be
     * used for power optimizations. The implementation should populate this map for frame rates
     * that requires the display to run at a higher refresh rate due to self refresh frames. The
     * lowest frame rate provided should be according to the parameter `maxFrameIntervalNs`
     * specified in IComposerClient.getDisplayConfigurations, as the framework would generally not
     * try to run at a lower frame rate.
     */
    parcelable FrameIntervalPowerHint {
        int frameIntervalNs;
        int averageRefreshPeriodNs;
    }
    @nullable FrameIntervalPowerHint[] frameIntervalPowerHints;

    parcelable NotifyExpectedPresentConfig {
        /**
         * The minimal time in nanoseconds that IComposerClient.notifyExpectedPresent needs to be
         * called ahead of an expectedPresentTime provided on a presentDisplay command.
         */
        int notifyExpectedPresentHeadsUpNs;

        /**
         * The time in nanoseconds that represents a timeout from the previous presentDisplay, which
         * after this point the display needs a call to IComposerClient.notifyExpectedPresent before
         * sending the next frame. If set to 0, there is no need to call
         * IComposerClient.notifyExpectedPresent for timeout.
         */
        int notifyExpectedPresentTimeoutNs;
    }

    /**
     * Parameters for when to call IComposerClient.notifyExpectedPresent.
     *
     * When set to null, the framework will not call IComposerClient.notifyExpectedPresent.
     */
    @nullable NotifyExpectedPresentConfig notifyExpectedPresentConfig;
}
