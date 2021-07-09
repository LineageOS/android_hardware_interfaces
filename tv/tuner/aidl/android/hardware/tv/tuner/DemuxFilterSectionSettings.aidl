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

import android.hardware.tv.tuner.DemuxFilterSectionSettingsCondition;

/**
 * Filter Settings for Section data according to ISO/IEC 13818-1.
 * @hide
 */
@VintfStability
parcelable DemuxFilterSectionSettings {
    DemuxFilterSectionSettingsCondition condition;

    /**
     * true if the filter checks CRC and discards data with wrong CRC
     */
    boolean isCheckCrc;

    /**
     * true if the filter repeats the data with the same version
     */
    boolean isRepeat;

    /**
     * true if the filter send onFilterStatus instead of onFilterEvent.
     */
    boolean isRaw;
}
