/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.input;

/**
 * Type of physical TV input.
 */
@VintfStability
@Backing(type="int")
enum TvInputType {
    OTHER = 1,
    TUNER = 2,
    COMPOSITE = 3,
    SVIDEO = 4,
    SCART = 5,
    COMPONENT = 6,
    VGA = 7,
    DVI = 8,
    HDMI = 9,
    DISPLAY_PORT = 10,
}
