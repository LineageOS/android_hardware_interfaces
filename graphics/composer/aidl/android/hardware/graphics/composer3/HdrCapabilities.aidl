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
 * Output parameters for IComposerClient.getHdrCapabilities
 *
 * @param types is an array of HDR types, may have 0 elements if the
 *         display is not HDR-capable.
 * @param maxLuminance is the desired content maximum luminance for this
 *         display in cd/m^2.
 * @param maxAverageLuminance - the desired content maximum frame-average
 *         luminance for this display in cd/m^2.
 * @param minLuminance is the desired content minimum luminance for this
 *         display in cd/m^2.
 */
@VintfStability
parcelable HdrCapabilities {
    android.hardware.graphics.common.Hdr[] types;
    float maxLuminance;
    float maxAverageLuminance;
    float minLuminance;
}
