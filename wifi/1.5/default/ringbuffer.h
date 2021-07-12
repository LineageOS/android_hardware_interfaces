/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <list>
#include <vector>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {

/**
 * Ringbuffer object used to store debug data.
 */
class Ringbuffer {
   public:
    explicit Ringbuffer(size_t maxSize);

    // Appends the data buffer and deletes from the front until buffer is
    // within |maxSize_|.
    void append(const std::vector<uint8_t>& input);
    const std::list<std::vector<uint8_t>>& getData() const;
    void clear();

   private:
    std::list<std::vector<uint8_t>> data_;
    size_t size_;
    size_t maxSize_;
};

}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // RINGBUFFER_H_
