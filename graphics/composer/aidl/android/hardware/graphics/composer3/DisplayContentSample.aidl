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

/**
 * Output parameters for IComposerClient.getDisplayedContentSample
 */
@VintfStability
parcelable DisplayContentSample {
    /**
     * The number of frames represented by this sample.
     */
    long frameCount;
    /**
     * A histogram counting how many times a pixel of a given value was displayed
     * onscreen for FORMAT_COMPONENT_0. The buckets of the histogram are evenly
     * weighted, the number of buckets is device specific. eg, for RGBA_8888,
     * if sampleComponent0 is {10, 6, 4, 1} this means that 10 red pixels were
     * displayed onscreen in range 0x00->0x3F, 6 red pixels were displayed
     * onscreen in range 0x40->0x7F, etc.
     */
    long[] sampleComponent0;
    /**
     * The same sample definition as sampleComponent0, but for FORMAT_COMPONENT_1.
     */
    long[] sampleComponent1;
    /**
     * The same sample definition as sampleComponent0, but for FORMAT_COMPONENT_2.
     */
    long[] sampleComponent2;
    /**
     * The same sample definition as sampleComponent0, but for FORMAT_COMPONENT_3.
     */
    long[] sampleComponent3;
}
