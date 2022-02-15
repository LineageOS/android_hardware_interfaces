/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

@VintfStability
@Backing(type="byte")
enum AacObjectType {
    /**
     * MPEG-2 Low Complexity. Support is Mandatory.
     */
    MPEG2_LC,
    /**
     * MPEG-4 Low Complexity. Support is Optional.
     */
    MPEG4_LC,
    /**
     * MPEG-4 Long Term Prediction. Support is Optional.
     */
    MPEG4_LTP,
    /**
     * MPEG-4 Scalable. Support is Optional.
     */
    MPEG4_SCALABLE,
}
