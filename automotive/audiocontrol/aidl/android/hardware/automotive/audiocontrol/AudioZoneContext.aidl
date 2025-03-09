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

package android.hardware.automotive.audiocontrol;

import android.hardware.automotive.audiocontrol.AudioZoneContextInfo;

/**
 * Encapsulates the list of car audio context info definitions
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioZoneContext {
    /**
     * List of car audio context info.
     *
     * <p>The list must include all audio attributes usages currently supported so that all audio
     * attribute usages can be routed for each car audio configuration.
     */
    List<AudioZoneContextInfo> audioContextInfos;
}
