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
#include <libprotocan/Signal.h>

namespace android::hardware::automotive::protocan {

class MessageCounter {
 public:
  const Signal::value upperBound;

  MessageCounter(Signal signal);

  /**
   * Parse CAN message sent by external ECU to determine current counter value.
   */
  void read(const can::V1_0::CanMessage& msg);

  /**
   * States whether current counter value is determined.
   */
  bool isReady() const;

  /**
   * Increment current counter value and set it in a new message.
   *
   * Caller must check isReady() at least once before calling this method.
   */
  void increment(can::V1_0::CanMessage& msg);

 private:
  const Signal mSignal;

  std::optional<Signal::value> mCurrent = std::nullopt;

  Signal::value next() const;
};

}  // namespace android::hardware::automotive::protocan
