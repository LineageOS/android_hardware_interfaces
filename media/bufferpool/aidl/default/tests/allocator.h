/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <pthread.h>
#include <bufferpool2/BufferPoolTypes.h>

using aidl::android::hardware::media::bufferpool2::implementation::
    BufferPoolStatus;
using aidl::android::hardware::media::bufferpool2::implementation::
    BufferPoolAllocation;
using aidl::android::hardware::media::bufferpool2::implementation::
    BufferPoolAllocator;
using aidl::android::hardware::media::bufferpool2::ResultStatus;

struct IpcMutex {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  int counter = 0;
  bool signalled = false;

  void init();

  static IpcMutex *Import(void *mem);
};

// buffer allocator for the tests
class TestBufferPoolAllocator : public BufferPoolAllocator {
 public:
  TestBufferPoolAllocator() {}

  ~TestBufferPoolAllocator() override {}

  BufferPoolStatus allocate(const std::vector<uint8_t> &params,
                        std::shared_ptr<BufferPoolAllocation> *alloc,
                        size_t *allocSize) override;

  bool compatible(const std::vector<uint8_t> &newParams,
                  const std::vector<uint8_t> &oldParams) override;

  static bool Fill(const native_handle_t *handle, const unsigned char val);

  static bool Verify(const native_handle_t *handle, const unsigned char val);

  static bool MapMemoryForMutex(const native_handle_t *handle, void **mem);

  static bool UnmapMemoryForMutex(void *mem);
};

// retrieve buffer allocator parameters
void getTestAllocatorParams(std::vector<uint8_t> *params);

void getIpcMutexParams(std::vector<uint8_t> *params);
