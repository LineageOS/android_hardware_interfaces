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

package android.hardware.automotive.vehicle;

/**
 * Every VMS message starts with the type of the message from the VmsMessageType enum.
 * Messages with no parameters such as VmsMessageType.AVAILABILITY_REQUEST,
 * VmsMessageType.SUBSCRIPTIONS_REQUEST and VmsMessageType.DATA are also based on this enum.
 */
@VintfStability
@Backing(type="int")
enum VmsBaseMessageIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
}
