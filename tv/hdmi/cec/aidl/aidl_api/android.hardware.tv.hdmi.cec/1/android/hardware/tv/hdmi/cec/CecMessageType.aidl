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
  FEATURE_ABORT = 0x00,
  IMAGE_VIEW_ON = 0x04,
  TUNER_STEP_INCREMENT = 0x05,
  TUNER_STEP_DECREMENT = 0x06,
  TUNER_DEVICE_STATUS = 0x07,
  GIVE_TUNER_DEVICE_STATUS = 0x08,
  RECORD_ON = 0x09,
  RECORD_STATUS = 0x0A,
  RECORD_OFF = 0x0B,
  TEXT_VIEW_ON = 0x0D,
  RECORD_TV_SCREEN = 0x0F,
  GIVE_DECK_STATUS = 0x1A,
  DECK_STATUS = 0x1B,
  SET_MENU_LANGUAGE = 0x32,
  CLEAR_ANALOG_TIMER = 0x33,
  SET_ANALOG_TIMER = 0x34,
  TIMER_STATUS = 0x35,
  STANDBY = 0x36,
  PLAY = 0x41,
  DECK_CONTROL = 0x42,
  TIMER_CLEARED_STATUS = 0x43,
  USER_CONTROL_PRESSED = 0x44,
  USER_CONTROL_RELEASED = 0x45,
  GIVE_OSD_NAME = 0x46,
  SET_OSD_NAME = 0x47,
  SET_OSD_STRING = 0x64,
  SET_TIMER_PROGRAM_TITLE = 0x67,
  SYSTEM_AUDIO_MODE_REQUEST = 0x70,
  GIVE_AUDIO_STATUS = 0x71,
  SET_SYSTEM_AUDIO_MODE = 0x72,
  REPORT_AUDIO_STATUS = 0x7A,
  GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D,
  SYSTEM_AUDIO_MODE_STATUS = 0x7E,
  ROUTING_CHANGE = 0x80,
  ROUTING_INFORMATION = 0x81,
  ACTIVE_SOURCE = 0x82,
  GIVE_PHYSICAL_ADDRESS = 0x83,
  REPORT_PHYSICAL_ADDRESS = 0x84,
  REQUEST_ACTIVE_SOURCE = 0x85,
  SET_STREAM_PATH = 0x86,
  DEVICE_VENDOR_ID = 0x87,
  VENDOR_COMMAND = 0x89,
  VENDOR_REMOTE_BUTTON_DOWN = 0x8A,
  VENDOR_REMOTE_BUTTON_UP = 0x8B,
  GIVE_DEVICE_VENDOR_ID = 0x8C,
  MENU_REQUEST = 0x8D,
  MENU_STATUS = 0x8E,
  GIVE_DEVICE_POWER_STATUS = 0x8F,
  REPORT_POWER_STATUS = 0x90,
  GET_MENU_LANGUAGE = 0x91,
  SELECT_ANALOG_SERVICE = 0x92,
  SELECT_DIGITAL_SERVICE = 0x93,
  SET_DIGITAL_TIMER = 0x97,
  CLEAR_DIGITAL_TIMER = 0x99,
  SET_AUDIO_RATE = 0x9A,
  INACTIVE_SOURCE = 0x9D,
  CEC_VERSION = 0x9E,
  GET_CEC_VERSION = 0x9F,
  VENDOR_COMMAND_WITH_ID = 0xA0,
  CLEAR_EXTERNAL_TIMER = 0xA1,
  SET_EXTERNAL_TIMER = 0xA2,
  REPORT_SHORT_AUDIO_DESCRIPTOR = 0xA3,
  REQUEST_SHORT_AUDIO_DESCRIPTOR = 0xA4,
  INITIATE_ARC = 0xC0,
  REPORT_ARC_INITIATED = 0xC1,
  REPORT_ARC_TERMINATED = 0xC2,
  REQUEST_ARC_INITIATION = 0xC3,
  REQUEST_ARC_TERMINATION = 0xC4,
  TERMINATE_ARC = 0xC5,
  ABORT = 0xFF,
  GIVE_FEATURES = 0xA5,
  REPORT_FEATURES = 0xA6,
  REQUEST_CURRENT_LATENCY = 0xA7,
  REPORT_CURRENT_LATENCY = 0xA8,
}
