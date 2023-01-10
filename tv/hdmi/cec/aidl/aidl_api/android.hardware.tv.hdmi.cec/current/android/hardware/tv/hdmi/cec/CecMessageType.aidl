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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.tv.hdmi.cec;
@Backing(type="int") @VintfStability
enum CecMessageType {
  FEATURE_ABORT = 0,
  IMAGE_VIEW_ON = 4,
  TUNER_STEP_INCREMENT = 5,
  TUNER_STEP_DECREMENT = 6,
  TUNER_DEVICE_STATUS = 7,
  GIVE_TUNER_DEVICE_STATUS = 8,
  RECORD_ON = 9,
  RECORD_STATUS = 10,
  RECORD_OFF = 11,
  TEXT_VIEW_ON = 13,
  RECORD_TV_SCREEN = 15,
  GIVE_DECK_STATUS = 26,
  DECK_STATUS = 27,
  SET_MENU_LANGUAGE = 50,
  CLEAR_ANALOG_TIMER = 51,
  SET_ANALOG_TIMER = 52,
  TIMER_STATUS = 53,
  STANDBY = 54,
  PLAY = 65,
  DECK_CONTROL = 66,
  TIMER_CLEARED_STATUS = 67,
  USER_CONTROL_PRESSED = 68,
  USER_CONTROL_RELEASED = 69,
  GIVE_OSD_NAME = 70,
  SET_OSD_NAME = 71,
  SET_OSD_STRING = 100,
  SET_TIMER_PROGRAM_TITLE = 103,
  SYSTEM_AUDIO_MODE_REQUEST = 112,
  GIVE_AUDIO_STATUS = 113,
  SET_SYSTEM_AUDIO_MODE = 114,
  REPORT_AUDIO_STATUS = 122,
  GIVE_SYSTEM_AUDIO_MODE_STATUS = 125,
  SYSTEM_AUDIO_MODE_STATUS = 126,
  ROUTING_CHANGE = 128,
  ROUTING_INFORMATION = 129,
  ACTIVE_SOURCE = 130,
  GIVE_PHYSICAL_ADDRESS = 131,
  REPORT_PHYSICAL_ADDRESS = 132,
  REQUEST_ACTIVE_SOURCE = 133,
  SET_STREAM_PATH = 134,
  DEVICE_VENDOR_ID = 135,
  VENDOR_COMMAND = 137,
  VENDOR_REMOTE_BUTTON_DOWN = 138,
  VENDOR_REMOTE_BUTTON_UP = 139,
  GIVE_DEVICE_VENDOR_ID = 140,
  MENU_REQUEST = 141,
  MENU_STATUS = 142,
  GIVE_DEVICE_POWER_STATUS = 143,
  REPORT_POWER_STATUS = 144,
  GET_MENU_LANGUAGE = 145,
  SELECT_ANALOG_SERVICE = 146,
  SELECT_DIGITAL_SERVICE = 147,
  SET_DIGITAL_TIMER = 151,
  CLEAR_DIGITAL_TIMER = 153,
  SET_AUDIO_RATE = 154,
  INACTIVE_SOURCE = 157,
  CEC_VERSION = 158,
  GET_CEC_VERSION = 159,
  VENDOR_COMMAND_WITH_ID = 160,
  CLEAR_EXTERNAL_TIMER = 161,
  SET_EXTERNAL_TIMER = 162,
  REPORT_SHORT_AUDIO_DESCRIPTOR = 163,
  REQUEST_SHORT_AUDIO_DESCRIPTOR = 164,
  INITIATE_ARC = 192,
  REPORT_ARC_INITIATED = 193,
  REPORT_ARC_TERMINATED = 194,
  REQUEST_ARC_INITIATION = 195,
  REQUEST_ARC_TERMINATION = 196,
  TERMINATE_ARC = 197,
  ABORT = 255,
  GIVE_FEATURES = 165,
  REPORT_FEATURES = 166,
  REQUEST_CURRENT_LATENCY = 167,
  REPORT_CURRENT_LATENCY = 168,
}
