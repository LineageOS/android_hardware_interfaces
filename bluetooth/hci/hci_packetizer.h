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

#include <functional>
#include <memory>
#include <vector>

#include "hci_internals.h"

namespace android::hardware::bluetooth::hci {

class HciPacketizer {
 public:
  HciPacketizer() = default;
  bool OnDataReady(PacketType packet_type, const std::vector<uint8_t>& data,
                   size_t* offset);
  const std::vector<uint8_t>& GetPacket() const;

 protected:
  enum State { HCI_HEADER, HCI_PAYLOAD };
  State state_{HCI_HEADER};
  std::vector<uint8_t> packet_;
  size_t bytes_remaining_{0};
};

}  // namespace android::hardware::bluetooth::hci
