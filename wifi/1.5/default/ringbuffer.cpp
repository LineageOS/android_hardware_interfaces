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

#include <android-base/logging.h>

#include "ringbuffer.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {

Ringbuffer::Ringbuffer(size_t maxSize) : size_(0), maxSize_(maxSize) {}

void Ringbuffer::append(const std::vector<uint8_t>& input) {
    if (input.size() == 0) {
        return;
    }
    if (input.size() > maxSize_) {
        LOG(INFO) << "Oversized message of " << input.size()
                  << " bytes is dropped";
        return;
    }
    data_.push_back(input);
    size_ += input.size() * sizeof(input[0]);
    while (size_ > maxSize_) {
        size_ -= data_.front().size() * sizeof(data_.front()[0]);
        data_.pop_front();
    }
}

const std::list<std::vector<uint8_t>>& Ringbuffer::getData() const {
    return data_;
}

void Ringbuffer::clear() {
    data_.clear();
    size_ = 0;
}

}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
