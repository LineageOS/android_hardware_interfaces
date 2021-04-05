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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_BUFFER_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Types.h>

namespace android::nn {

class MockBuffer final : public IBuffer {
  public:
    MOCK_METHOD(Request::MemoryDomainToken, getToken, (), (const, override));
    MOCK_METHOD(GeneralResult<void>, copyTo, (const SharedMemory& dst), (const, override));
    MOCK_METHOD(GeneralResult<void>, copyFrom,
                (const SharedMemory& src, const Dimensions& dimensions), (const, override));
};

}  // namespace android::nn

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_TEST_MOCK_BUFFER_H
