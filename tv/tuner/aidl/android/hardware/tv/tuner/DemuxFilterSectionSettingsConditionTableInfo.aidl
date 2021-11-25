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
 * Table information for Section Filter.
 * @hide
 */
@VintfStability
parcelable DemuxFilterSectionSettingsConditionTableInfo {
    /**
     * Table ID for Section Filter
     */
    int tableId;

    /**
     * Optional version number for Section Filter. It should be ignored if INVALID_TABINFO_VERSION
     * is set.
     */
    int version;
}
