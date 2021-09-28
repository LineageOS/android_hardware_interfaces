/**
 * Copyright (c) 2021, The Android Open Source Project
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

import android.hardware.graphics.composer3.FormatColorComponent;

/**
 * Output parameters for IComposerClient.getDisplayedContentSamplingAttributes
 */
@VintfStability
parcelable DisplayContentSamplingAttributes {
    /**
     * The format of the sampled pixels.
     */
    android.hardware.graphics.common.PixelFormat format;
    /**
     * The dataspace of the sampled pixels.
     */
    android.hardware.graphics.common.Dataspace dataspace;
    /**
     * The mask of which components can be sampled.
     */
    FormatColorComponent componentMask;
}
