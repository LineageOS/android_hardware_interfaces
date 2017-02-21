/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef VTS_HAL_GRAPHICS_ALLOCATOR_UTILS
#define VTS_HAL_GRAPHICS_ALLOCATOR_UTILS

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace tests {

class AllocatorClient;

// A wrapper to IAllocator.
class Allocator {
 public:
  Allocator();

  sp<IAllocator> getRaw() const;

  // Returns true when the allocator supports the specified capability.
  bool hasCapability(IAllocator::Capability capability) const;

  std::vector<IAllocator::Capability> getCapabilities();
  std::string dumpDebugInfo();
  std::unique_ptr<AllocatorClient> createClient();

 private:
  void init();

  sp<IAllocator> mAllocator;
  std::unordered_set<IAllocator::Capability> mCapabilities;
};

// A wrapper to IAllocatorClient.
class AllocatorClient {
 public:
  AllocatorClient(const sp<IAllocatorClient>& client);
  ~AllocatorClient();

  sp<IAllocatorClient> getRaw() const;

  BufferDescriptor createDescriptor(
      const IAllocatorClient::BufferDescriptorInfo& info);
  void destroyDescriptor(BufferDescriptor descriptor);

  Error testAllocate(const std::vector<BufferDescriptor>& descriptors);
  bool testAllocate(BufferDescriptor descriptor);

  Error allocate(const std::vector<BufferDescriptor>& descriptors,
                 std::vector<Buffer>& buffers);
  Buffer allocate(BufferDescriptor descriptor);
  void free(Buffer buffer);

  // Returns a handle to the buffer.  The ownership of the handle is
  // transferred to the caller.
  native_handle_t* exportHandle(BufferDescriptor descriptor, Buffer buffer);

 private:
  sp<IAllocatorClient> mClient;

  // Keep track of all descriptors and buffers.  When a test fails with
  // ASSERT_*, the destructor will clean up the resources for the test.
  std::unordered_set<BufferDescriptor> mDescriptors;
  std::unordered_set<Buffer> mBuffers;
};

}  // namespace tests
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_GRAPHICS_ALLOCATOR_UTILS
