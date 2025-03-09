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

package android.hardware.tv.mediaquality;

@VintfStability
enum ColorTemperature {
    USER,
    COOL,
    STANDARD,
    WARM,
    USER_HDR10PLUS,
    COOL_HDR10PLUS,
    STANDARD_HDR10PLUS,
    WARM_HDR10PLUS,
    FMMSDR,
    FMMHDR,
}
