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
 * ALP Length Type
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum DemuxAlpLengthType {
    UNDEFINED = 0,

    /**
     * Length does NOT include additional header. Used in US region.
     */
    WITHOUT_ADDITIONAL_HEADER,

    /**
     * Length includes additional header. Used in Korea region.
     */
    WITH_ADDITIONAL_HEADER,
}
