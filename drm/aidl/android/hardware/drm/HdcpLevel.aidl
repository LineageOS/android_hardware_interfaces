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

package android.hardware.drm;

/**
 * HDCP specifications are defined by Digital Content Protection LLC (DCP).
 *   "HDCP Specification Rev. 2.3 Interface Independent Adaptation"
 *   "HDCP 2.3 on HDMI Specification"
 */
@VintfStability
@Backing(type="int")
enum HdcpLevel {
    /**
     * Unable to determine the HDCP level
     */
    HDCP_UNKNOWN,
    /**
     * No HDCP, output is unprotected
     */
    HDCP_NONE,
    /**
     * HDCP version 1.0
     */
    HDCP_V1,
    /**
     * HDCP version 2.0 Type 1.
     */
    HDCP_V2,
    /**
     * HDCP version 2.1 Type 1.
     */
    HDCP_V2_1,
    /**
     *  HDCP version 2.2 Type 1.
     */
    HDCP_V2_2,
    /**
     * No digital output, implicitly secure
     */
    HDCP_NO_OUTPUT,
    /**
     * HDCP version 2.3 Type 1.
     */
    HDCP_V2_3,
}
