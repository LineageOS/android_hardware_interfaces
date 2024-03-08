/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb.fira_android;

/**
 * Android specific session type set in UCI command:
 * GID: 0001b (UWB Session config Group)
 * OID: 000000b (SESSION_INIT_CMD)
 *
 * Note: Refer to Table 13 of the UCI specification for the other
 * session types.
 */
@VintfStability
@Backing(type="int")
enum UwbVendorSessionInitSessionType {
    /** Added in vendor version 0. */
    CCC = 0xA0,
    RADAR = 0xA1,
}
