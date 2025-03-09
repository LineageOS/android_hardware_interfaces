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

package android.hardware.tv.mediaquality;

import android.hardware.tv.mediaquality.AmbientBacklightCompressAlgorithm;
import android.hardware.tv.mediaquality.AmbientBacklightSettings;

@VintfStability
parcelable AmbientBacklightMetadata {
    /**
     * The settings which are used to generate the colors.
     */
    AmbientBacklightSettings settings;

    /**
     * The compress algorithm of the ambient backlight colors.
     */
    AmbientBacklightCompressAlgorithm compressAlgorithm;

    /**
     * The colors for the zones. Format of the color will be indicated in the
     * AmbientBacklightSettings::colorFormat.
     */
    int[] zonesColors;
}
