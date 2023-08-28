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

package android.hardware.automotive.evs;

/**
 * Color channels.
 */
@VintfStability
@Backing(type="int")
enum ColorChannel {
    R,
    G_EVEN,      // The green channel in even lines.
    B,
    G_ODD_OR_Y,  // The green channel in odd lines of
                 // color formats that have two green (or
                 // equivalent) channels, or the luminance
                 // if it exists in associated data.
}
