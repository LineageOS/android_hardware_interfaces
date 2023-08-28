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

package android.hardware.broadcastradio;

/**
 * Type of an announcement.
 *
 * <p>It maps to different announcement types for each radio technology.
 */
@VintfStability
@Backing(type="byte")
@JavaDerive(equals=true, toString=true)
enum AnnouncementType {
    /**
     * Undefined announcement type
     */
    INVALID = 0,

    /**
     * DAB alarm, RDS emergency program type (PTY 31).
     */
    EMERGENCY = 1,

    /**
     * DAB warning.
     */
    WARNING,

    /**
     * DAB road traffic, RDS TA, HD Radio transportation.
     */
    TRAFFIC,

    /**
     * Weather.
     */
    WEATHER,

    /**
     * News.
     */
    NEWS,

    /**
     * DAB event, special event.
     */
    EVENT,

    /**
     * DAB sport report, RDS sports.
     */
    SPORT,

    /**
     * All others.
     */
    MISC,
}
