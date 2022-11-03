/*
 * Copyright 2022 The Android Open Source Project
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

#include <cstdlib>

namespace android::hardware::bluetooth::hci {

// HCI UART transport packet types (Volume 4, Part A, 2)
enum class PacketType : uint8_t {
  UNKNOWN = 0,
  COMMAND = 1,
  ACL_DATA = 2,
  SCO_DATA = 3,
  EVENT = 4,
  ISO_DATA = 5,
};

// 2 bytes for opcode, 1 byte for parameter length (Volume 4, Part E, 5.4.1)
static constexpr size_t kCommandHeaderSize = 3;
static constexpr size_t kCommandLengthOffset = 2;

// 2 bytes for handle, 2 bytes for data length (Volume 4, Part E, 5.4.2)
static constexpr size_t kAclHeaderSize = 4;
static constexpr size_t kAclLengthOffset = 2;

// 2 bytes for handle, 1 byte for data length (Volume 4, Part E, 5.4.3)
static constexpr size_t kScoHeaderSize = 3;
static constexpr size_t kScoLengthOffset = 2;

// 1 byte for event code, 1 byte for parameter length (Volume 4, Part E, 5.4.4)
static constexpr size_t kEventHeaderSize = 2;
static constexpr size_t kEventLengthOffset = 1;

// 2 bytes for handle, 2 bytes for data length (Volume 4, Part E, 5.4.5)
static constexpr size_t kIsoHeaderSize = 4;
static constexpr size_t kIsoLengthOffset = 2;

static constexpr size_t kMaxHeaderSize = kAclHeaderSize;

}  // namespace android::hardware::bluetooth::hci
