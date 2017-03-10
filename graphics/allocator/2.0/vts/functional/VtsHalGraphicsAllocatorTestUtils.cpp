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

#include <VtsHalHidlTargetTestBase.h>

#include "VtsHalGraphicsAllocatorTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace tests {

Allocator::Allocator() { init(); }

void Allocator::init() {
  mAllocator = ::testing::VtsHalHidlTargetTestBase::getService<IAllocator>();
  ASSERT_NE(nullptr, mAllocator.get()) << "failed to get allocator service";

  std::vector<IAllocator::Capability> capabilities = getCapabilities();
  mCapabilities.insert(capabilities.begin(), capabilities.end());
}

sp<IAllocator> Allocator::getRaw() const { return mAllocator; }

bool Allocator::hasCapability(IAllocator::Capability capability) const {
  return mCapabilities.count(capability) > 0;
}

std::vector<IAllocator::Capability> Allocator::getCapabilities() {
  std::vector<IAllocator::Capability> capabilities;
  mAllocator->getCapabilities(
      [&](const auto& tmpCapabilities) { capabilities = tmpCapabilities; });

  return capabilities;
}

std::string Allocator::dumpDebugInfo() {
  std::string debugInfo;
  mAllocator->dumpDebugInfo(
      [&](const auto& tmpDebugInfo) { debugInfo = tmpDebugInfo.c_str(); });

  return debugInfo;
}

std::unique_ptr<AllocatorClient> Allocator::createClient() {
  std::unique_ptr<AllocatorClient> client;
  mAllocator->createClient([&](const auto& tmpError, const auto& tmpClient) {
    ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
    client = std::make_unique<AllocatorClient>(tmpClient);
  });

  return client;
}

AllocatorClient::AllocatorClient(const sp<IAllocatorClient>& client)
    : mClient(client) {}

AllocatorClient::~AllocatorClient() {
  for (auto buffer : mBuffers) {
    EXPECT_EQ(Error::NONE, mClient->free(buffer))
        << "failed to free buffer " << buffer;
  }
  mBuffers.clear();

  for (auto descriptor : mDescriptors) {
    EXPECT_EQ(Error::NONE, mClient->destroyDescriptor(descriptor))
        << "failed to destroy descriptor " << descriptor;
  }
  mDescriptors.clear();
}

sp<IAllocatorClient> AllocatorClient::getRaw() const { return mClient; }

BufferDescriptor AllocatorClient::createDescriptor(
    const IAllocatorClient::BufferDescriptorInfo& info) {
  BufferDescriptor descriptor = 0;
  mClient->createDescriptor(
      info, [&](const auto& tmpError, const auto& tmpDescriptor) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create descriptor";
        descriptor = tmpDescriptor;

        EXPECT_TRUE(mDescriptors.insert(descriptor).second)
            << "duplicated descriptor id " << descriptor;
      });

  return descriptor;
}

void AllocatorClient::destroyDescriptor(BufferDescriptor descriptor) {
  ASSERT_EQ(Error::NONE, mClient->destroyDescriptor(descriptor))
      << "failed to destroy descriptor " << descriptor;

  mDescriptors.erase(descriptor);
}

Error AllocatorClient::testAllocate(
    const std::vector<BufferDescriptor>& descriptors) {
  return mClient->testAllocate(descriptors);
}

bool AllocatorClient::testAllocate(BufferDescriptor descriptor) {
  std::vector<BufferDescriptor> descriptors(1, descriptor);
  Error error = testAllocate(descriptors);
  return (error == Error::NONE || error == Error::NOT_SHARED);
}

Error AllocatorClient::allocate(
    const std::vector<BufferDescriptor>& descriptors,
    std::vector<Buffer>& buffers) {
  Error error = Error::NO_RESOURCES;
  mClient->allocate(descriptors, [&](const auto& tmpError,
                                     const auto& tmpBuffers) {
    ASSERT_TRUE(tmpError == Error::NONE || tmpError == Error::NOT_SHARED)
        << "failed to allocate buffer";
    ASSERT_EQ(descriptors.size(), tmpBuffers.size()) << "invalid buffer count";

    error = tmpError;
    buffers = tmpBuffers;

    for (auto buffer : buffers) {
      EXPECT_TRUE(mBuffers.insert(buffer).second)
          << "duplicated buffer id " << buffer;
    }
  });

  return error;
}

Buffer AllocatorClient::allocate(BufferDescriptor descriptor) {
  std::vector<BufferDescriptor> descriptors(1, descriptor);
  std::vector<Buffer> buffers;
  allocate(descriptors, buffers);
  if (::testing::Test::HasFatalFailure()) {
    return 0;
  }

  return buffers[0];
}

void AllocatorClient::free(Buffer buffer) {
  ASSERT_EQ(Error::NONE, mClient->free(buffer))
      << "failed to free buffer " << buffer;

  mBuffers.erase(buffer);
}

native_handle_t* AllocatorClient::exportHandle(BufferDescriptor descriptor,
                                               Buffer buffer) {
  native_handle_t* handle;
  mClient->exportHandle(
      descriptor, buffer, [&](const auto& tmpError, const auto& tmpHandle) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to export buffer handle";
        ASSERT_NE(nullptr, tmpHandle.getNativeHandle())
            << "invalid buffer handle";

        handle = native_handle_clone(tmpHandle.getNativeHandle());
        ASSERT_NE(nullptr, handle) << "failed to clone handle";
      });

  return handle;
}

}  // namespace tests
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
