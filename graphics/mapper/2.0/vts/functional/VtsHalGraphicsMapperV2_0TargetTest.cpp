/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "graphics_mapper_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <VtsHalHidlTargetBaseTest.h>
#include <sync/sync.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace tests {
namespace {

using namespace android::hardware::graphics::allocator::V2_0;
using namespace android::hardware::graphics::common::V1_0;

class GraphicsMapperHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 protected:
  void SetUp() override {
    mAllocator = ::testing::VtsHalHidlTargetBaseTest::getService<IAllocator>();
    ASSERT_NE(mAllocator, nullptr);

    mAllocator->createClient([this](const auto& error, const auto& client) {
      if (error == Error::NONE) {
        mAllocatorClient = client;
      }
    });
    ASSERT_NE(mAllocatorClient, nullptr);

    mMapper = ::testing::VtsHalHidlTargetBaseTest::getService<IMapper>();
    ASSERT_NE(nullptr, mMapper.get());
    ASSERT_FALSE(mMapper->isRemote());

    mDummyDescriptorInfo.width = 64;
    mDummyDescriptorInfo.height = 64;
    mDummyDescriptorInfo.layerCount = 1;
    mDummyDescriptorInfo.format = PixelFormat::RGBA_8888;
    mDummyDescriptorInfo.producerUsageMask =
        static_cast<uint64_t>(ProducerUsage::CPU_WRITE);
    mDummyDescriptorInfo.consumerUsageMask =
        static_cast<uint64_t>(ConsumerUsage::CPU_READ);
  }

  void TearDown() override {}

  const native_handle_t* allocate(
      const IAllocatorClient::BufferDescriptorInfo& info) {
    // create descriptor
    Error err = Error::NO_RESOURCES;
    BufferDescriptor descriptor;
    mAllocatorClient->createDescriptor(
        info, [&](const auto& tmpError, const auto& tmpDescriptor) {
          err = tmpError;
          descriptor = tmpDescriptor;
        });
    if (err != Error::NONE) {
      return nullptr;
    }

    // allocate buffer
    hidl_vec<BufferDescriptor> descriptors;
    hidl_vec<Buffer> buffers;
    descriptors.setToExternal(&descriptor, 1);
    err = Error::NO_RESOURCES;
    mAllocatorClient->allocate(
        descriptors, [&](const auto& tmpError, const auto& tmpBuffers) {
          err = tmpError;
          buffers = tmpBuffers;
        });
    if ((err != Error::NONE && err != Error::NOT_SHARED) ||
        buffers.size() != 1) {
      mAllocatorClient->destroyDescriptor(descriptors[0]);
      return nullptr;
    }

    // export handle
    err = Error::NO_RESOURCES;
    const native_handle_t* handle = nullptr;
    mAllocatorClient->exportHandle(
        descriptors[0], buffers[0],
        [&](const auto& tmpError, const auto& tmpHandle) {
          err = tmpError;
          if (err != Error::NONE) {
            return;
          }

          handle = native_handle_clone(tmpHandle);
          if (!handle) {
            err = Error::NO_RESOURCES;
            return;
          }

          err = mMapper->retain(handle);
          if (err != Error::NONE) {
            native_handle_close(handle);
            native_handle_delete(const_cast<native_handle_t*>(handle));
            handle = nullptr;
          }
        });

    mAllocatorClient->destroyDescriptor(descriptors[0]);
    mAllocatorClient->free(buffers[0]);

    if (err != Error::NONE) {
      return nullptr;
    }

    return handle;
  }

  sp<IMapper> mMapper;

  IAllocatorClient::BufferDescriptorInfo mDummyDescriptorInfo{};

 private:
  sp<IAllocator> mAllocator;
  sp<IAllocatorClient> mAllocatorClient;
};

/**
 * Test IMapper::retain and IMapper::release.
 */
TEST_F(GraphicsMapperHidlTest, RetainRelease) {
  const native_handle_t* buffer = allocate(mDummyDescriptorInfo);
  ASSERT_NE(buffer, nullptr);

  const int maxRefs = 10;
  for (int i = 0; i < maxRefs; i++) {
    auto err = mMapper->retain(buffer);
    EXPECT_EQ(Error::NONE, err);
  }
  for (int i = 0; i < maxRefs; i++) {
    auto err = mMapper->release(buffer);
    EXPECT_EQ(Error::NONE, err);
  }

  auto err = mMapper->release(buffer);
  EXPECT_EQ(Error::NONE, err);
}

/**
 * Test IMapper::get* getters.
 */
TEST_F(GraphicsMapperHidlTest, Getters) {
  const native_handle_t* buffer = allocate(mDummyDescriptorInfo);
  ASSERT_NE(buffer, nullptr);

  Error err = Error::NO_RESOURCES;
  IAllocatorClient::BufferDescriptorInfo info{};
  mMapper->getDimensions(buffer, [&](const auto& tmpError, const auto& tmpWidth,
                                     const auto& tmpHeight) {
    err = tmpError;
    info.width = tmpWidth;
    info.height = tmpHeight;
  });
  EXPECT_EQ(Error::NONE, err);
  mMapper->getFormat(buffer, [&](const auto& tmpError, const auto& tmpFormat) {
    err = tmpError;
    info.format = tmpFormat;
  });
  EXPECT_EQ(Error::NONE, err);
  mMapper->getProducerUsageMask(
      buffer, [&](const auto& tmpError, const auto& tmpUsage) {
        err = tmpError;
        info.producerUsageMask = tmpUsage;
      });
  EXPECT_EQ(Error::NONE, err);
  mMapper->getConsumerUsageMask(
      buffer, [&](const auto& tmpError, const auto& tmpUsage) {
        err = tmpError;
        info.consumerUsageMask = tmpUsage;
      });
  EXPECT_EQ(Error::NONE, err);

  EXPECT_EQ(mDummyDescriptorInfo.width, info.width);
  EXPECT_EQ(mDummyDescriptorInfo.height, info.height);
  EXPECT_EQ(mDummyDescriptorInfo.format, info.format);
  EXPECT_EQ(mDummyDescriptorInfo.producerUsageMask, info.producerUsageMask);
  EXPECT_EQ(mDummyDescriptorInfo.consumerUsageMask, info.consumerUsageMask);

  BackingStore store = 0;
  mMapper->getBackingStore(buffer,
                           [&](const auto& tmpError, const auto& tmpStore) {
                             err = tmpError;
                             store = tmpStore;
                           });
  EXPECT_EQ(Error::NONE, err);

  uint32_t stride = 0;
  mMapper->getStride(buffer, [&](const auto& tmpError, const auto& tmpStride) {
    err = tmpError;
    stride = tmpStride;
  });
  EXPECT_EQ(Error::NONE, err);
  EXPECT_LE(info.width, stride);

  err = mMapper->release(buffer);
  EXPECT_EQ(Error::NONE, err);
}

/**
 * Test IMapper::lock and IMapper::unlock.
 */
TEST_F(GraphicsMapperHidlTest, LockBasic) {
  const auto& info = mDummyDescriptorInfo;
  const native_handle_t* buffer = allocate(info);
  ASSERT_NE(buffer, nullptr);

  Error err = Error::NO_RESOURCES;
  uint32_t stride = 0;
  mMapper->getStride(buffer, [&](const auto& tmpError, const auto& tmpStride) {
    err = tmpError;
    stride = tmpStride;
  });
  EXPECT_EQ(Error::NONE, err);

  // lock buffer for writing
  const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                             static_cast<int32_t>(info.height)};
  hidl_handle acquireFence(nullptr);
  uint32_t* data;
  err = Error::NO_RESOURCES;
  mMapper->lock(buffer, info.producerUsageMask, 0, region, acquireFence,
          [&](const auto& tmpError, const auto& tmpData) {
            err = tmpError;
            data = static_cast<uint32_t*>(tmpData);
          });

  if (err == Error::NONE) {
    for (uint32_t y = 0; y < info.height; y++) {
      for (uint32_t x = 0; x < info.width; x++) {
        data[stride * y + x] = info.height * y + x;
      }
    }
  } else {
    EXPECT_EQ(Error::NONE, err);
  }

  err = Error::NO_RESOURCES;
  mMapper->unlock(buffer, [&](const auto& tmpError, const auto& tmpReleaseFence) {
            err = tmpError;
            auto handle = tmpReleaseFence.getNativeHandle();
            if (handle && handle->numFds == 1) {
                sync_wait(handle->data[0], -1);
                close(handle->data[0]);
            }
          });
  EXPECT_EQ(Error::NONE, err);

  // lock buffer for reading
  mMapper->lock(buffer, 0, info.consumerUsageMask, region, acquireFence,
          [&](const auto& tmpError, const auto& tmpData) {
            err = tmpError;
            data = static_cast<uint32_t*>(tmpData);
          });
  if (err == Error::NONE) {
    for (uint32_t y = 0; y < info.height; y++) {
      for (uint32_t x = 0; x < info.width; x++) {
        EXPECT_EQ(info.height * y + x, data[stride * y + x]);
      }
    }
  } else {
    EXPECT_EQ(Error::NONE, err);
  }

  err = Error::NO_RESOURCES;
  mMapper->unlock(buffer, [&](const auto& tmpError, const auto& tmpReleaseFence) {
            err = tmpError;
            auto handle = tmpReleaseFence.getNativeHandle();
            if (handle && handle->numFds == 1) {
                sync_wait(handle->data[0], -1);
                close(handle->data[0]);
            }
          });
  EXPECT_EQ(Error::NONE, err);

  err = mMapper->release(buffer);
  EXPECT_EQ(Error::NONE, err);
}

}  // namespace anonymous
}  // namespace tests
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  return status;
}
