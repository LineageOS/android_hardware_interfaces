/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * Android specific radar app config values set/expected in UCI command:
 * GID: 1100b (Android specific Group)
 * OID: 010001b (RADAR_SET_APP_CONFIG_CMD)
 * OID: 010010b (RADAR_GET_APP_CONFIG_CMD)
 */
@VintfStability
@Backing(type="int")
enum UwbVendorRadarAppConfigTlvValues {
    RADAR_DATA_TYPE_RADAR_SWEEP_SAMPLES = 0x0,
}
