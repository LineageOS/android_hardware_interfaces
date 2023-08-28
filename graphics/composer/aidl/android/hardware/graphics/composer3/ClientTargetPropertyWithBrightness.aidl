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

import android.hardware.graphics.composer3.ClientTargetProperty;
import android.hardware.graphics.composer3.DimmingStage;

@VintfStability
parcelable ClientTargetPropertyWithBrightness {
    /**
     * The display which this commands refers to.
     */
    long display;

    /**
     * The Client target property.
     */
    ClientTargetProperty clientTargetProperty;

    /**
     * The brightness as described in CommandResultPayload.clientTargetProperty
     */
    float brightness;

    /**
     * The stage in which dimming operations should be performed when compositing
     * the client target.
     *
     * Note that with a COLORIMETRIC RenderIntent, DimmingSpace must be LINEAR. That is, dimming
     * is defined to occur in linear space. However, some composer implementations may, with
     * other vendor-defined RenderIntents, apply certain image quality adjustments that are
     * sensitive to gamma shift when dimming in linear space. To avoid this issue, those
     * implementations must opt to dim in gamma space.
     */
    DimmingStage dimmingStage;
}
