/*
 * Copyright 2023 The Android Open Source Project
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

#pragma once

#include <cstdint>

namespace bluetooth_hal {
namespace debug {

/* HCI connect/disconnect event result codes to human readable strings. */
static const char* result_code_strings[] = {
    "Success",                                                   // 0x00
    "Unknown HCI Command",                                       // 0x01
    "Unknown Connection Identifier",                             // 0x02
    "Hardware Failure",                                          // 0x03
    "Page Timeout",                                              // 0x04
    "Authentication Failure",                                    // 0x05
    "PIN or Key Missing",                                        // 0x06
    "Memory Capacity Exceeded",                                  // 0x07
    "Connection Timeout",                                        // 0x08
    "Connection Limit Exceeded",                                 // 0x09
    "Synchronous Connection Limit To A Device Exceeded",         // 0x0A
    "Connection Already Exists",                                 // 0x0B
    "Command Disallowed",                                        // 0x0C
    "Connection Rejected due to Limited Resources",              // 0x0D
    "Connection Rejected Due To Security Reasons",               // 0x0E
    "Connection Rejected due to Unacceptable BD_ADDR",           // 0x0F
    "Connection Accept Timeout Exceeded",                        // 0x10
    "Unsupported Feature or Parameter Value",                    // 0x11
    "Invalid HCI Command Parameters",                            // 0x12
    "Remote User Terminated Connection",                         // 0x13
    "Remote Device Terminated Connection due to Low Resources",  // 0x14
    "Remote Device Terminated Connection due to Power Off",      // 0x15
    "Connection Terminated By Local Host",                       // 0x16
    "Repeated Attempts",                                         // 0x17
    "Pairing Not Allowed",                                       // 0x18
    "Unknown LMP PDU",                                           // 0x19
    "Unknown result code"};                                      // 0x1A

/*******************************************************************************
 *
 * Function         GetResultString
 *
 * Description      This function returns the human-readable string for a given
 *                  result code.
 *
 * Returns          a pointer to the human-readable string for the given result.
 *
 ******************************************************************************/
const char* GetResultString(const uint8_t result_code) {
  if (result_code > 0x19) {
    return result_code_strings[0x1A];
  }

  return result_code_strings[result_code];
}

}  // namespace debug
}  // namespace bluetooth_hal
