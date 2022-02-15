/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android/hardware/automotive/can/1.0/types.h>
#include <libprotocan/Checksum.h>
#include <libprotocan/MessageCounter.h>
#include <libprotocan/Signal.h>

namespace android::hardware::automotive::protocan {

/**
 * CAN message definition (not the actual message data).
 *
 * Describes static message properties (message ID, signals etc).
 */
class MessageDef {
 public:
  const can::V1_0::CanMessageId id;

  /**
   * Create message definition.
   *
   * Currently only constant length messages are supported.
   *
   * \param id CAN message ID
   * \param len CAN message length
   * \param signals CAN signal definitions
   * \param counter Designated CAN signal definition for message counter, if the message has one
   * \param checksum Designated CAN signal definition for payload checksum, if the message has one
   */
  MessageDef(can::V1_0::CanMessageId id, uint16_t len, std::map<std::string, Signal> signals,
             std::optional<Signal> counter = std::nullopt,
             std::optional<Checksum> checksum = std::nullopt);

  const Signal& operator[](const std::string& signalName) const;

  can::V1_0::CanMessage makeDefault() const;
  MessageCounter makeCounter() const;

  void updateChecksum(can::V1_0::CanMessage& msg) const;

  /**
   * Validate the message payload is large enough to hold all the signals.
   */
  bool validate(const can::V1_0::CanMessage& msg) const;

private:
  const uint16_t kLen;
  const std::map<std::string, Signal> kSignals;
  const std::optional<Signal> kCounter;
  const std::optional<Checksum> kChecksum;
};

}  // namespace android::hardware::automotive::protocan
