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

package android.hardware.radio;

@VintfStability
@Backing(type="int")
enum SsServiceType {
    CFU,
    CF_BUSY,
    CF_NO_REPLY,
    CF_NOT_REACHABLE,
    CF_ALL,
    CF_ALL_CONDITIONAL,
    CLIP,
    CLIR,
    COLP,
    COLR,
    WAIT,
    BAOC,
    BAOIC,
    BAOIC_EXC_HOME,
    BAIC,
    BAIC_ROAMING,
    ALL_BARRING,
    OUTGOING_BARRING,
    INCOMING_BARRING,
}
