/**
 * Copyright (c) 2022, The Android Open Source Project
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

package android.hardware.graphics.common;

import android.hardware.graphics.common.ParcelableHdr;

/**
 * Output parameter for IComposerClient.getHdrConversionCapabilities
 *
 * @param sourceType is the HDR type that can be converted to outputType.
 *
 * @param outputType is the HDR type/ SDR that the source type can be converted to. The value null
 * is used to depict SDR outputType.
 *
 * @param addsLatency is false if no latency added due to HDR conversion from sourceType to
 * outputType, otherwise true.
 */
@VintfStability
parcelable HdrConversionCapability {
    ParcelableHdr sourceType;
    @nullable ParcelableHdr outputType;
    boolean addsLatency;
}
