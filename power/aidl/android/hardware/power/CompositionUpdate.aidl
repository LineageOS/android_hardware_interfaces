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

import android.hardware.power.FrameProducerUpdate;

/**
 * An update regarding composition objects that might be sent outside of a normal
 * sendCompositionData call, such as for lifecycle updates. This object is either
 * attached to CompositionData or sent separately, depending on current activity
 * and urgency.
 */
@VintfStability
parcelable CompositionUpdate {
    /**
     * Timestamp for when the message was sent.
     */
    long timestampNanos;

    /**
     * Update objects for all frame producers that have changed.
     */
    FrameProducerUpdate[] producerUpdates;

    /**
     * The IDs of any outputs that have disconnected in the framework.
     */
    long[] deadOutputIds;
}
