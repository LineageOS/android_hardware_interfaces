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

#ifndef VTS_HAL_GRAPHICS_MAPPER_UTILS
#define VTS_HAL_GRAPHICS_MAPPER_UTILS

#include <memory>
#include <unordered_map>

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <utils/StrongPointer.h>

#include "VtsHalGraphicsAllocatorTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace tests {

using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::allocator::V2_0::IAllocatorClient;
using android::hardware::graphics::allocator::V2_0::tests::AllocatorClient;

// A wrapper to IMapper.
class Mapper {
 public:
  Mapper();
  ~Mapper();

  sp<IMapper> getRaw() const;

  void retain(const native_handle_t* handle);
  void release(const native_handle_t* handle);

  struct Dimensions {
    uint32_t width;
    uint32_t height;
  };
  Dimensions getDimensions(const native_handle_t* handle);

  PixelFormat getFormat(const native_handle_t* handle);
  uint32_t getLayerCount(const native_handle_t* handle);
  uint64_t getProducerUsageMask(const native_handle_t* handle);
  uint64_t getConsumerUsageMask(const native_handle_t* handle);
  BackingStore getBackingStore(const native_handle_t* handle);
  uint32_t getStride(const native_handle_t* handle);

  // We use fd instead of hidl_handle in these functions to pass fences
  // in and out of the mapper.  The ownership of the fd is always transferred
  // with each of these functions.
  void* lock(const native_handle_t* handle, uint64_t producerUsageMask,
             uint64_t consumerUsageMask, const IMapper::Rect& accessRegion,
             int acquireFence);
  FlexLayout lockFlex(const native_handle_t* handle, uint64_t producerUsageMask,
                      uint64_t consumerUsageMask,
                      const IMapper::Rect& accessRegion, int acquireFence);
  int unlock(const native_handle_t* handle);

  // Requests AllocatorClient to allocate a buffer, export the handle, and
  // register the handle with mapper.
  const native_handle_t* allocate(
      std::unique_ptr<AllocatorClient>& allocatorClient,
      const IAllocatorClient::BufferDescriptorInfo& info);

 private:
  void init();

  sp<IMapper> mMapper;

  // Keep track of all registered (retained) handles.  When a test fails with
  // ASSERT_*, the destructor will release the handles for the test.
  std::unordered_map<const native_handle_t*, uint64_t> mHandles;
};

}  // namespace tests
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_GRAPHICS_MAPPER_UTILS
