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

package android.hardware.broadcastradio;

/**
 * Coordinate reprensenting the geographic location in alert message
 *
 * <p>(see ITU-T X.1303 bis for more info).
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable Coordinate {
    /**
     * Latitude of the cooridinate.
     *
     * <p>Latitude is in the range of -90 to 90.
     */
    double latitude;

    /**
     * Longitude of the cooridinate.
     *
     * <p>Longitude is in the range of -180 to 180.
     */
    double longitude;
}
