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
#include <VtsHalHidlTargetBaseTest.h>
#include <sync/sync.h>
#include "VtsHalGraphicsMapperTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace tests {
namespace {

using namespace android::hardware::graphics::allocator::V2_0;
using namespace android::hardware::graphics::allocator::V2_0::tests;

class GraphicsMapperHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(mAllocator = std::make_unique<Allocator>());
    ASSERT_NO_FATAL_FAILURE(mAllocatorClient = mAllocator->createClient());
    ASSERT_NO_FATAL_FAILURE(mMapper = std::make_unique<Mapper>());

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

  std::unique_ptr<Allocator> mAllocator;
  std::unique_ptr<AllocatorClient> mAllocatorClient;
  std::unique_ptr<Mapper> mMapper;
  IAllocatorClient::BufferDescriptorInfo mDummyDescriptorInfo{};
};

/**
 * Test IMapper::retain and IMapper::release.
 */
TEST_F(GraphicsMapperHidlTest, RetainRelease) {
  const native_handle_t* buffer;
  ASSERT_NO_FATAL_FAILURE(
      buffer = mMapper->allocate(mAllocatorClient, mDummyDescriptorInfo));

  const int maxRefs = 10;
  for (int i = 0; i < maxRefs; i++) {
    ASSERT_NO_FATAL_FAILURE(mMapper->retain(buffer));
  }
  for (int i = 0; i < maxRefs; i++) {
    ASSERT_NO_FATAL_FAILURE(mMapper->release(buffer));
  }

  ASSERT_NO_FATAL_FAILURE(mMapper->release(buffer));
}

/**
 * Test IMapper::get* getters.
 */
TEST_F(GraphicsMapperHidlTest, Getters) {
  const native_handle_t* buffer;
  ASSERT_NO_FATAL_FAILURE(
      buffer = mMapper->allocate(mAllocatorClient, mDummyDescriptorInfo));

  IAllocatorClient::BufferDescriptorInfo info = {};

  Mapper::Dimensions dimensions;
  ASSERT_NO_FATAL_FAILURE(dimensions = mMapper->getDimensions(buffer));
  info.width = dimensions.width;
  info.height = dimensions.height;

  ASSERT_NO_FATAL_FAILURE(info.format = mMapper->getFormat(buffer));
  ASSERT_NO_FATAL_FAILURE(info.producerUsageMask =
                              mMapper->getProducerUsageMask(buffer));
  ASSERT_NO_FATAL_FAILURE(info.consumerUsageMask =
                              mMapper->getConsumerUsageMask(buffer));

  EXPECT_EQ(mDummyDescriptorInfo.width, info.width);
  EXPECT_EQ(mDummyDescriptorInfo.height, info.height);
  EXPECT_EQ(mDummyDescriptorInfo.format, info.format);
  EXPECT_EQ(mDummyDescriptorInfo.producerUsageMask, info.producerUsageMask);
  EXPECT_EQ(mDummyDescriptorInfo.consumerUsageMask, info.consumerUsageMask);

  ASSERT_NO_FATAL_FAILURE(mMapper->getBackingStore(buffer));

  uint32_t stride;
  ASSERT_NO_FATAL_FAILURE(stride = mMapper->getStride(buffer));
  EXPECT_LE(info.width, stride);
}

/**
 * Test IMapper::lock and IMapper::unlock.
 */
TEST_F(GraphicsMapperHidlTest, LockBasic) {
  const auto& info = mDummyDescriptorInfo;

  const native_handle_t* buffer;
  ASSERT_NO_FATAL_FAILURE(
      buffer = mMapper->allocate(mAllocatorClient, mDummyDescriptorInfo));

  uint32_t stride;
  ASSERT_NO_FATAL_FAILURE(stride = mMapper->getStride(buffer));

  // lock buffer for writing
  const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                             static_cast<int32_t>(info.height)};
  int fence = -1;
  uint32_t* data;
  ASSERT_NO_FATAL_FAILURE(
      data = static_cast<uint32_t*>(
          mMapper->lock(buffer, info.producerUsageMask, 0, region, fence)));

  for (uint32_t y = 0; y < info.height; y++) {
    for (uint32_t x = 0; x < info.width; x++) {
      data[stride * y + x] = info.height * y + x;
    }
  }

  ASSERT_NO_FATAL_FAILURE(fence = mMapper->unlock(buffer));

  // lock buffer for reading
  ASSERT_NO_FATAL_FAILURE(
      data = static_cast<uint32_t*>(
          mMapper->lock(buffer, 0, info.consumerUsageMask, region, fence)));
  for (uint32_t y = 0; y < info.height; y++) {
    for (uint32_t x = 0; x < info.width; x++) {
      EXPECT_EQ(info.height * y + x, data[stride * y + x]);
    }
  }

  ASSERT_NO_FATAL_FAILURE(fence = mMapper->unlock(buffer));
  if (fence >= 0) {
    close(fence);
  }
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
