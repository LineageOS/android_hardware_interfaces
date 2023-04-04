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

package android.hardware.confirmationui;
@VintfStability
interface IConfirmationUI {
  void abort();
  void deliverSecureInputEvent(in android.hardware.security.keymint.HardwareAuthToken secureInputToken);
  void promptUserConfirmation(in android.hardware.confirmationui.IConfirmationResultCallback resultCB, in byte[] promptText, in byte[] extraData, in @utf8InCpp String locale, in android.hardware.confirmationui.UIOption[] uiOptions);
  const int OK = 0;
  const int CANCELED = 1;
  const int ABORTED = 2;
  const int OPERATION_PENDING = 3;
  const int IGNORED = 4;
  const int SYSTEM_ERROR = 5;
  const int UNIMPLEMENTED = 6;
  const int UNEXPECTED = 7;
  const int UI_ERROR = 0x10000;
  const int UI_ERROR_MISSING_GLYPH = 0x10001;
  const int UI_ERROR_MESSAGE_TOO_LONG = 0x10002;
  const int UI_ERROR_MALFORMED_UTF8ENCODING = 0x10003;
  const int TEST_KEY_BYTE = 0xA5;
  const int MAX_MESSAGE_SIZE = 0x1800;
}
