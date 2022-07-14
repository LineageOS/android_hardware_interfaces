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

package android.hardware.wifi;

/**
 * The discovery bands supported by NAN.
 */
@VintfStability
@Backing(type="int")
enum NanBandIndex {
    NAN_BAND_24GHZ = 0,
    NAN_BAND_5GHZ,
    /**
     * Index for 6 GHz band.
     */
    NAN_BAND_6GHZ = 2,
}
