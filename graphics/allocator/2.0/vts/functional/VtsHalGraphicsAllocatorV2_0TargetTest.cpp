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

#define LOG_TAG "graphics_allocator_hidl_hal_test"

#include <android-base/logging.h>
#include <VtsHalHidlTargetTestBase.h>

#include "VtsHalGraphicsAllocatorTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace tests {
namespace {

using android::hardware::graphics::common::V1_0::PixelFormat;

#define CHECK_FEATURE_OR_SKIP(FEATURE_NAME)                 \
  do {                                                      \
    if (!mAllocator->hasCapability(FEATURE_NAME)) {         \
      std::cout << "[  SKIPPED ] Feature " << #FEATURE_NAME \
                << " not supported" << std::endl;           \
      return;                                               \
    }                                                       \
  } while (0)

class GraphicsAllocatorHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(mAllocator = std::make_unique<Allocator>());
    ASSERT_NO_FATAL_FAILURE(mClient = mAllocator->createClient());

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
  std::unique_ptr<AllocatorClient> mClient;
  IAllocatorClient::BufferDescriptorInfo mDummyDescriptorInfo{};
};

TEST_F(GraphicsAllocatorHidlTest, GetCapabilities) {
  auto capabilities = mAllocator->getCapabilities();
  for (auto cap : capabilities) {
    EXPECT_NE(IAllocator::Capability::INVALID, cap);
  }
}

TEST_F(GraphicsAllocatorHidlTest, DumpDebugInfo) {
  mAllocator->dumpDebugInfo();
}

TEST_F(GraphicsAllocatorHidlTest, CreateDestroyDescriptor) {
  BufferDescriptor descriptor;
  ASSERT_NO_FATAL_FAILURE(descriptor =
                              mClient->createDescriptor(mDummyDescriptorInfo));
  mClient->destroyDescriptor(descriptor);
}

/**
 * Test testAllocate with a single buffer descriptor.
 */
TEST_F(GraphicsAllocatorHidlTest, TestAllocateBasic) {
  CHECK_FEATURE_OR_SKIP(IAllocator::Capability::TEST_ALLOCATE);

  BufferDescriptor descriptor;
  ASSERT_NO_FATAL_FAILURE(descriptor =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  ASSERT_TRUE(mClient->testAllocate(descriptor));
}

/**
 * Test testAllocate with two buffer descriptors.
 */
TEST_F(GraphicsAllocatorHidlTest, TestAllocateArray) {
  CHECK_FEATURE_OR_SKIP(IAllocator::Capability::TEST_ALLOCATE);

  BufferDescriptor descriptor;
  ASSERT_NO_FATAL_FAILURE(descriptor =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(2);
  descriptors[0] = descriptor;
  descriptors[1] = descriptor;

  auto error = mClient->testAllocate(descriptors);
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
}

/**
 * Test allocate/free with a single buffer descriptor.
 */
TEST_F(GraphicsAllocatorHidlTest, AllocateFreeBasic) {
  BufferDescriptor descriptor;
  ASSERT_NO_FATAL_FAILURE(descriptor =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  Buffer buffer;
  ASSERT_NO_FATAL_FAILURE(buffer = mClient->allocate(descriptor));

  mClient->free(buffer);
}

/**
 * Test allocate/free with an array of buffer descriptors.
 */
TEST_F(GraphicsAllocatorHidlTest, AllocateFreeArray) {
  BufferDescriptor descriptor1;
  ASSERT_NO_FATAL_FAILURE(descriptor1 =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  BufferDescriptor descriptor2;
  ASSERT_NO_FATAL_FAILURE(descriptor2 =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(3);
  descriptors[0] = descriptor1;
  descriptors[1] = descriptor1;
  descriptors[2] = descriptor2;

  std::vector<Buffer> buffers;
  ASSERT_NO_FATAL_FAILURE(mClient->allocate(descriptors, buffers));

  for (auto buf : buffers) {
    mClient->free(buf);
  }
}

TEST_F(GraphicsAllocatorHidlTest, ExportHandle) {
  BufferDescriptor descriptor;
  ASSERT_NO_FATAL_FAILURE(descriptor =
                              mClient->createDescriptor(mDummyDescriptorInfo));

  Buffer buffer;
  ASSERT_NO_FATAL_FAILURE(buffer = mClient->allocate(descriptor));

  native_handle_t* handle;
  ASSERT_NO_FATAL_FAILURE(handle = mClient->exportHandle(descriptor, buffer));

  native_handle_close(handle);
  native_handle_delete(handle);
}

}  // namespace anonymous
}  // namespace tests
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  return status;
}
