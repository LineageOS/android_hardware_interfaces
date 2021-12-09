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

package android.hardware.graphics.composer3.command;

import android.hardware.graphics.common.ColorTransform;

@VintfStability
parcelable ColorTransformPayload {
    /**
     * 4x4 transform matrix (16 floats) as described in DisplayCommand.colorTransform.
     */
    float[] matrix;

    /**
     * Hint value which may be used instead of the given matrix unless it
     * is ColorTransform::ARBITRARY.
     */
    ColorTransform hint;
}
