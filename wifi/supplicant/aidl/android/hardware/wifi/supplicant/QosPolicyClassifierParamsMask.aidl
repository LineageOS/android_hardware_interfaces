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

package android.hardware.wifi.supplicant;

/**
 * Enum values for QoS policy classifier params mask bits.
 */
@VintfStability
@Backing(type="int")
enum QosPolicyClassifierParamsMask {
    SRC_IP = 1 << 0,
    DST_IP = 1 << 1,
    SRC_PORT = 1 << 2,
    DST_PORT_RANGE = 1 << 3,
    PROTOCOL_NEXT_HEADER = 1 << 4,
    FLOW_LABEL = 1 << 5,
    DOMAIN_NAME = 1 << 6,
    DSCP = 1 << 7,
}
