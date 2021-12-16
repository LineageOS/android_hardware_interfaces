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

package android.hardware.wifi.supplicant;

/**
 * Access Network Query Protocol subtype elements
 * for Hotspot 2.0.
 */
@VintfStability
@Backing(type="int")
enum Hs20AnqpSubtypes {
    OPERATOR_FRIENDLY_NAME = 3,
    WAN_METRICS = 4,
    CONNECTION_CAPABILITY = 5,
    OSU_PROVIDERS_LIST = 8,
}
