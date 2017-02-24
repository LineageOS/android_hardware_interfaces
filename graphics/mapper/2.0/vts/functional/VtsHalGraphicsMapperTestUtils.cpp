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

#include <VtsHalHidlTargetBaseTest.h>

#include "VtsHalGraphicsMapperTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace tests {

using android::hardware::graphics::allocator::V2_0::Buffer;
using android::hardware::graphics::allocator::V2_0::BufferDescriptor;
using android::hardware::graphics::allocator::V2_0::Error;

Mapper::Mapper() { init(); }

void Mapper::init() {
  mMapper = ::testing::VtsHalHidlTargetBaseTest::getService<IMapper>();
  ASSERT_NE(nullptr, mMapper.get()) << "failed to get mapper service";
  ASSERT_FALSE(mMapper->isRemote()) << "mapper is not in passthrough mode";
}

Mapper::~Mapper() {
  for (auto it : mHandles) {
    while (it.second) {
      EXPECT_EQ(Error::NONE, mMapper->release(it.first))
          << "failed to release handle " << it.first;
      it.second--;
    }
  }
  mHandles.clear();
}

sp<IMapper> Mapper::getRaw() const { return mMapper; }

void Mapper::retain(const native_handle_t* handle) {
  Error error = mMapper->retain(handle);
  ASSERT_EQ(Error::NONE, error) << "failed to retain handle " << handle;

  mHandles[handle]++;
}

void Mapper::release(const native_handle_t* handle) {
  Error error = mMapper->release(handle);
  ASSERT_EQ(Error::NONE, error) << "failed to release handle " << handle;

  if (--mHandles[handle] == 0) {
    mHandles.erase(handle);
  }
}

Mapper::Dimensions Mapper::getDimensions(const native_handle_t* handle) {
  Dimensions dimensions = {};
  mMapper->getDimensions(handle, [&](const auto& tmpError, const auto& tmpWidth,
                                     const auto& tmpHeight) {
    ASSERT_EQ(Error::NONE, tmpError)
        << "failed to get dimensions for handle " << handle;
    dimensions.width = tmpWidth;
    dimensions.height = tmpHeight;
  });

  return dimensions;
}

PixelFormat Mapper::getFormat(const native_handle_t* handle) {
  PixelFormat format = static_cast<PixelFormat>(0);
  mMapper->getFormat(handle, [&](const auto& tmpError, const auto& tmpFormat) {
    ASSERT_EQ(Error::NONE, tmpError)
        << "failed to get format for handle " << handle;
    format = tmpFormat;
  });

  return format;
}

uint32_t Mapper::getLayerCount(const native_handle_t* handle) {
  uint32_t count = 0;
  mMapper->getLayerCount(
      handle, [&](const auto& tmpError, const auto& tmpCount) {
        ASSERT_EQ(Error::NONE, tmpError)
            << "failed to get layer count for handle " << handle;
        count = tmpCount;
      });

  return count;
}

uint64_t Mapper::getProducerUsageMask(const native_handle_t* handle) {
  uint64_t usageMask = 0;
  mMapper->getProducerUsageMask(
      handle, [&](const auto& tmpError, const auto& tmpUsageMask) {
        ASSERT_EQ(Error::NONE, tmpError)
            << "failed to get producer usage mask for handle " << handle;
        usageMask = tmpUsageMask;
      });

  return usageMask;
}

uint64_t Mapper::getConsumerUsageMask(const native_handle_t* handle) {
  uint64_t usageMask = 0;
  mMapper->getConsumerUsageMask(
      handle, [&](const auto& tmpError, const auto& tmpUsageMask) {
        ASSERT_EQ(Error::NONE, tmpError)
            << "failed to get consumer usage mask for handle " << handle;
        usageMask = tmpUsageMask;
      });

  return usageMask;
}

BackingStore Mapper::getBackingStore(const native_handle_t* handle) {
  BackingStore backingStore = 0;
  mMapper->getBackingStore(
      handle, [&](const auto& tmpError, const auto& tmpBackingStore) {
        ASSERT_EQ(Error::NONE, tmpError)
            << "failed to get backing store for handle " << handle;
        backingStore = tmpBackingStore;
      });

  return backingStore;
}

uint32_t Mapper::getStride(const native_handle_t* handle) {
  uint32_t stride = 0;
  mMapper->getStride(handle, [&](const auto& tmpError, const auto& tmpStride) {
    ASSERT_EQ(Error::NONE, tmpError)
        << "failed to get stride for handle " << handle;
    stride = tmpStride;
  });

  return stride;
}

void* Mapper::lock(const native_handle_t* handle, uint64_t producerUsageMask,
                   uint64_t consumerUsageMask,
                   const IMapper::Rect& accessRegion, int acquireFence) {
  NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 0, 1);
  native_handle_t* acquireFenceHandle = nullptr;
  if (acquireFence >= 0) {
    acquireFenceHandle = native_handle_init(acquireFenceStorage, 0, 1);
    acquireFenceHandle->data[0] = acquireFence;
  }

  void* data = nullptr;
  mMapper->lock(
      handle, producerUsageMask, consumerUsageMask, accessRegion,
      acquireFenceHandle, [&](const auto& tmpError, const auto& tmpData) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to lock handle " << handle;
        data = tmpData;
      });

  if (acquireFence >= 0) {
    close(acquireFence);
  }

  return data;
}

FlexLayout Mapper::lockFlex(const native_handle_t* handle,
                            uint64_t producerUsageMask,
                            uint64_t consumerUsageMask,
                            const IMapper::Rect& accessRegion,
                            int acquireFence) {
  NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 0, 1);
  native_handle_t* acquireFenceHandle = nullptr;
  if (acquireFence >= 0) {
    acquireFenceHandle = native_handle_init(acquireFenceStorage, 0, 1);
    acquireFenceHandle->data[0] = acquireFence;
  }

  FlexLayout layout = {};
  mMapper->lockFlex(handle, producerUsageMask, consumerUsageMask, accessRegion,
                    acquireFenceHandle,
                    [&](const auto& tmpError, const auto& tmpLayout) {
                      ASSERT_EQ(Error::NONE, tmpError)
                          << "failed to lockFlex handle " << handle;
                      layout = tmpLayout;
                    });

  if (acquireFence >= 0) {
    close(acquireFence);
  }

  return layout;
}

int Mapper::unlock(const native_handle_t* handle) {
  int releaseFence = -1;
  mMapper->unlock(handle, [&](const auto& tmpError,
                              const auto& tmpReleaseFence) {
    ASSERT_EQ(Error::NONE, tmpError) << "failed to unlock handle " << handle;

    auto handle = tmpReleaseFence.getNativeHandle();
    if (handle) {
      ASSERT_EQ(0, handle->numInts) << "invalid fence handle " << handle;
      if (handle->numFds == 1) {
        releaseFence = dup(handle->data[0]);
        ASSERT_LT(0, releaseFence) << "failed to dup fence fd";
      } else {
        ASSERT_EQ(0, handle->numFds) << " invalid fence handle " << handle;
      }
    }
  });

  return releaseFence;
}

const native_handle_t* Mapper::allocate(
    std::unique_ptr<AllocatorClient>& allocatorClient,
    const IAllocatorClient::BufferDescriptorInfo& info) {
  BufferDescriptor descriptor = allocatorClient->createDescriptor(info);
  if (::testing::Test::HasFatalFailure()) {
    return nullptr;
  }

  Buffer buffer = allocatorClient->allocate(descriptor);
  if (::testing::Test::HasFatalFailure()) {
    allocatorClient->destroyDescriptor(descriptor);
    return nullptr;
  }

  const native_handle_t* handle =
      allocatorClient->exportHandle(descriptor, buffer);
  if (handle) {
    retain(handle);
  }

  allocatorClient->free(buffer);
  allocatorClient->destroyDescriptor(descriptor);

  return handle;
}

}  // namespace tests
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
