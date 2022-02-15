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

class Checksum {
 public:
  using formula = std::function<Signal::value(const can::V1_0::CanMessage&)>;

  Checksum(Signal signal, formula f);

  void update(can::V1_0::CanMessage& msg) const;

 private:
  const Signal mSignal;
  const formula mFormula;
};

}  // namespace android::hardware::automotive::protocan
