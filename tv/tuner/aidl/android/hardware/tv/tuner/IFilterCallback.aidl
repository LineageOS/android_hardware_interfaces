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

import android.hardware.tv.tuner.DemuxFilterEvent;
import android.hardware.tv.tuner.DemuxFilterStatus;

/**
 * This interface is used by the HAL to notify the filter event and scan status
 * back to the client, the cient implements the interfaces and passes a handle
 * to the HAL.
 * @hide
 */
@VintfStability
oneway interface IFilterCallback {
    /**
     * Notify the client that a new filter event happened.
     *
     * @param events an array of DemuxFilterEvent.
     */
    void onFilterEvent(in DemuxFilterEvent[] events);

    /**
     * Notify the client a new status of a filter.
     *
     * @param status a new status of the filter.
     */
    void onFilterStatus(in DemuxFilterStatus status);
}
