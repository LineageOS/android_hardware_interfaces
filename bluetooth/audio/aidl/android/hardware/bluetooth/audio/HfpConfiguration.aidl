/*
 * Copyright 2023 The Android Open Source Project
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

import android.hardware.bluetooth.audio.CodecId;

@VintfStability
parcelable HfpConfiguration {
    /**
     * Codec identifier.
     */
    CodecId codecId;

    /**
     * The connection handle used for SCO connection.
     * Range: 0x0000 to 0x0EFF.
     */
    int connectionHandle;

    /**
     *  Echo canceling and noise reduction functions resident in the AG.
     */
    boolean nrec;

    /**
     *  Indicate whether the codec is encoded and decoded in the controller.
     *  If the codec is inside the DSP, then it would be transparent mode.
     */
    boolean controllerCodec;
}
