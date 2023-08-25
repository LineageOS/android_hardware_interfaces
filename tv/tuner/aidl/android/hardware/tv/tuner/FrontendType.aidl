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

package android.hardware.tv.tuner;

/**
 * Extended Frontend Type.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendType {
    UNDEFINED = 0,

    ANALOG,

    /**
     * Advanced Television Systems Committee (ATSC) Standard A/72.
     */
    ATSC,

    /**
     * Advanced Television Systems Committee (ATSC 3.0) Standard A/300.
     */
    ATSC3,

    /**
     * Digital Video Broadcasting - Cable
     * DVB Cable Frontend Standard ETSI EN 300 468 V1.15.1.
     */
    DVBC,

    /**
     * Digital Video Broadcasting - Satellite
     * DVB Satellite Frontend Standard ETSI EN 300 468 V1.15.1 and
     * ETSI EN 302 307-2 V1.1.1.
     */
    DVBS,

    /**
     * Digital Video Broadcasting - Terrestrial
     * DVB Terrestrial Frontend Standard ETSI EN 300 468 V1.15.1 and
     * ETSI EN 302 755 V1.4.1.
     */
    DVBT,

    /**
     * Integrated Services Digital Broadcasting-Satellite (ISDB-S)
     * ARIB STD-B20 is technical document of ISDB-S.
     */
    ISDBS,

    /**
     * Integrated Services Digital Broadcasting-Satellite (ISDB-S)
     * ARIB STD-B44 is technical document of ISDB-S3.
     */
    ISDBS3,

    /**
     * Integrated Services Digital Broadcasting-Terrestrial (ISDB-T or SBTVD)
     * ABNT NBR 15603 is technical document of ISDB-T.
     */
    ISDBT,

    /**
     * DTMB (Digital Terrestrial Multimedia Broadcast) standard.
     */
    DTMB,

    /**
     * ITU IPTV (ITU-T FG IPTV)
     */
    IPTV,
}
