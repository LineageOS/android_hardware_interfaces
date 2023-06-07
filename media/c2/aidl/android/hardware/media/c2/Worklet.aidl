/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.media.c2;

import android.hardware.media.c2.FrameData;
import android.hardware.media.c2.SettingResult;

/**
 * In/out structure containing some instructions for and results from output
 * processing.
 *
 * This is a part of @ref Work. One `Worklet` corresponds to one output
 * @ref FrameData. The client must construct an original `Worklet` object inside
 * a @ref Work object for each expected output before calling
 * IComponent::queue().
 */
@VintfStability
parcelable Worklet {
    /**
     * Component id. (Input)
     *
     * This is used only when tunneling is enabled.
     *
     * When used, this must match the return value from IConfigurable::getId().
     */
    int componentId;
    /**
     * List of C2Param objects describing tunings to be applied before
     * processing this `Worklet`. (Input)
     */
    byte[] tunings;
    /**
     * List of failures. (Output)
     */
    SettingResult[] failures;
    /**
     * Output frame data. (Output)
     */
    FrameData output;
}
