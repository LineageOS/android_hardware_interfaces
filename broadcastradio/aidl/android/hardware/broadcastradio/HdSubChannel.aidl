/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.broadcastradio;

/**
 * Index of HD radio subchannel.
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum HdSubChannel {
    /**
     * Index of HD radio subchannel 1.
     *
     * <p>There are at most 8 HD radio subchannels of 1-based om HD radio standard. It is
     * converted to 0-based index. 0 is the index of main program service (MPS). 1 to 7
     * are indexes of additional supplemental program services (SPS).
     */
    HD1 = 0,
    /**
     * {@see HD1}
     */
    HD2 = 1,
    /**
     * {@see HD1}
     */
    HD3 = 2,
    /**
     * {@see HD1}
     */
    HD4 = 3,
    /**
     * {@see HD1}
     */
    HD5 = 4,
    /**
     * {@see HD1}
     */
    HD6 = 5,
    /**
     * {@see HD1}
     */
    HD7 = 6,
    /**
     * {@see HD1}
     */
    HD8 = 7,
}
