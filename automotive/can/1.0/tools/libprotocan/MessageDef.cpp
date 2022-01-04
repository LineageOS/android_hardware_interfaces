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

#include <libprotocan/MessageDef.h>

#include <android-base/logging.h>

namespace android::hardware::automotive::protocan {

using can::V1_0::CanMessage;
using can::V1_0::CanMessageId;

MessageDef::MessageDef(CanMessageId id, uint16_t len, std::map<std::string, Signal> signals,
                       std::optional<Signal> counter, std::optional<Checksum> checksum)
    : id(id), kLen(len), kSignals(std::move(signals)), kCounter(counter), kChecksum(checksum) {}

const Signal& MessageDef::operator[](const std::string& signalName) const {
  auto it = kSignals.find(signalName);
  CHECK(it != kSignals.end()) << "Signal " << signalName << " doesn't exist";
  return it->second;
}

CanMessage MessageDef::makeDefault() const {
  CanMessage msg = {};
  msg.id = id;
  msg.payload.resize(kLen);

  for (auto const& [name, signal] : kSignals) {
    signal.setDefault(msg);
  }

  return msg;
}

MessageCounter MessageDef::makeCounter() const {
  CHECK(kCounter.has_value()) << "Can't build a counter for message without such signal";
  return MessageCounter(*kCounter);
}

void MessageDef::updateChecksum(can::V1_0::CanMessage& msg) const {
  if (!kChecksum.has_value()) return;
  kChecksum->update(msg);
}

bool MessageDef::validate(const can::V1_0::CanMessage& msg) const {
    return msg.payload.size() >= kLen;
}

}  // namespace android::hardware::automotive::protocan
