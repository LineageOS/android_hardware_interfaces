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

#include <unordered_set>

#include <android-base/logging.h>
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <VtsHalHidlTargetBaseTest.h>

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
    if (!hasCapability(FEATURE_NAME)) {                     \
      std::cout << "[  SKIPPED ] Feature " << #FEATURE_NAME \
                << " not supported" << std::endl;           \
      return;                                               \
    }                                                       \
  } while (0)

class TempDescriptor {
 public:
  TempDescriptor(const sp<IAllocatorClient>& client,
                 const IAllocatorClient::BufferDescriptorInfo& info)
      : mClient(client), mError(Error::NO_RESOURCES) {
    mClient->createDescriptor(
        info, [&](const auto& tmpError, const auto& tmpDescriptor) {
          mError = tmpError;
          mDescriptor = tmpDescriptor;
        });
  }

  ~TempDescriptor() {
    if (mError == Error::NONE) {
      mClient->destroyDescriptor(mDescriptor);
    }
  }

  bool isValid() const { return (mError == Error::NONE); }

  operator BufferDescriptor() const { return mDescriptor; }

 private:
  sp<IAllocatorClient> mClient;
  Error mError;
  BufferDescriptor mDescriptor;
};

class GraphicsAllocatorHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 protected:
  void SetUp() override {
    mAllocator = ::testing::VtsHalHidlTargetBaseTest::getService<IAllocator>();
    ASSERT_NE(mAllocator, nullptr);

    mAllocator->createClient([this](const auto& error, const auto& client) {
      if (error == Error::NONE) {
        mClient = client;
      }
    });
    ASSERT_NE(mClient, nullptr);

    initCapabilities();

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

  /**
   * Initialize the set of supported capabilities.
   */
  void initCapabilities() {
    mAllocator->getCapabilities([this](const auto& capabilities) {
      std::vector<IAllocator::Capability> caps = capabilities;
      mCapabilities.insert(caps.cbegin(), caps.cend());
    });
  }

  /**
   * Test whether a capability is supported.
   */
  bool hasCapability(IAllocator::Capability capability) const {
    return (mCapabilities.count(capability) > 0);
  }

  sp<IAllocator> mAllocator;
  sp<IAllocatorClient> mClient;
  IAllocatorClient::BufferDescriptorInfo mDummyDescriptorInfo{};

 private:
  std::unordered_set<IAllocator::Capability> mCapabilities;
};

TEST_F(GraphicsAllocatorHidlTest, GetCapabilities) {
  auto ret = mAllocator->getCapabilities([](const auto& capabilities) {
    std::vector<IAllocator::Capability> caps = capabilities;
    for (auto cap : caps) {
      EXPECT_NE(IAllocator::Capability::INVALID, cap);
    }
  });

  ASSERT_TRUE(ret.isOk());
}

TEST_F(GraphicsAllocatorHidlTest, DumpDebugInfo) {
  auto ret = mAllocator->dumpDebugInfo([](const auto&) {
    // nothing to do
  });

  ASSERT_TRUE(ret.isOk());
}

TEST_F(GraphicsAllocatorHidlTest, CreateDestroyDescriptor) {
  Error error;
  BufferDescriptor descriptor;
  auto ret = mClient->createDescriptor(
      mDummyDescriptorInfo,
      [&](const auto& tmpError, const auto& tmpDescriptor) {
        error = tmpError;
        descriptor = tmpDescriptor;
      });

  ASSERT_TRUE(ret.isOk());
  ASSERT_EQ(Error::NONE, error);

  auto err_ret = mClient->destroyDescriptor(descriptor);
  ASSERT_TRUE(err_ret.isOk());
  ASSERT_EQ(Error::NONE, static_cast<Error>(err_ret));
}

/**
 * Test testAllocate with a single buffer descriptor.
 */
TEST_F(GraphicsAllocatorHidlTest, TestAllocateBasic) {
  CHECK_FEATURE_OR_SKIP(IAllocator::Capability::TEST_ALLOCATE);

  TempDescriptor descriptor(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor.isValid());

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(1);
  descriptors[0] = descriptor;

  auto ret = mClient->testAllocate(descriptors);
  ASSERT_TRUE(ret.isOk());

  auto error = static_cast<Error>(ret);
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
}

/**
 * Test testAllocate with two buffer descriptors.
 */
TEST_F(GraphicsAllocatorHidlTest, TestAllocateArray) {
  CHECK_FEATURE_OR_SKIP(IAllocator::Capability::TEST_ALLOCATE);

  TempDescriptor descriptor(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor.isValid());

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(2);
  descriptors[0] = descriptor;
  descriptors[1] = descriptor;

  auto ret = mClient->testAllocate(descriptors);
  ASSERT_TRUE(ret.isOk());

  auto error = static_cast<Error>(ret);
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
}

/**
 * Test allocate/free with a single buffer descriptor.
 */
TEST_F(GraphicsAllocatorHidlTest, AllocateFreeBasic) {
  TempDescriptor descriptor(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor.isValid());

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(1);
  descriptors[0] = descriptor;

  Error error;
  std::vector<Buffer> buffers;
  auto ret = mClient->allocate(
      descriptors, [&](const auto& tmpError, const auto& tmpBuffers) {
        error = tmpError;
        buffers = tmpBuffers;
      });

  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
  EXPECT_EQ(1u, buffers.size());

  if (!buffers.empty()) {
    auto err_ret = mClient->free(buffers[0]);
    EXPECT_TRUE(err_ret.isOk());
    EXPECT_EQ(Error::NONE, static_cast<Error>(err_ret));
  }
}

/**
 * Test allocate/free with an array of buffer descriptors.
 */
TEST_F(GraphicsAllocatorHidlTest, AllocateFreeArray) {
  TempDescriptor descriptor1(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor1.isValid());

  TempDescriptor descriptor2(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor2.isValid());

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(3);
  descriptors[0] = descriptor1;
  descriptors[1] = descriptor1;
  descriptors[2] = descriptor2;

  Error error;
  std::vector<Buffer> buffers;
  auto ret = mClient->allocate(
      descriptors, [&](const auto& tmpError, const auto& tmpBuffers) {
        error = tmpError;
        buffers = tmpBuffers;
      });

  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
  EXPECT_EQ(descriptors.size(), buffers.size());

  for (auto buf : buffers) {
    auto err_ret = mClient->free(buf);
    EXPECT_TRUE(err_ret.isOk());
    EXPECT_EQ(Error::NONE, static_cast<Error>(err_ret));
  }
}

TEST_F(GraphicsAllocatorHidlTest, ExportHandle) {
  TempDescriptor descriptor(mClient, mDummyDescriptorInfo);
  ASSERT_TRUE(descriptor.isValid());

  hidl_vec<BufferDescriptor> descriptors;
  descriptors.resize(1);
  descriptors[0] = descriptor;

  Error error;
  std::vector<Buffer> buffers;
  auto ret = mClient->allocate(
      descriptors, [&](const auto& tmpError, const auto& tmpBuffers) {
        error = tmpError;
        buffers = tmpBuffers;
      });

  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(error == Error::NONE || error == Error::NOT_SHARED);
  ASSERT_EQ(1u, buffers.size());

  ret = mClient->exportHandle(
      descriptors[0], buffers[0],
      [&](const auto& tmpError, const auto&) { error = tmpError; });
  EXPECT_TRUE(ret.isOk());
  EXPECT_EQ(Error::NONE, error);

  auto err_ret = mClient->free(buffers[0]);
  EXPECT_TRUE(err_ret.isOk());
  EXPECT_EQ(Error::NONE, static_cast<Error>(err_ret));
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
