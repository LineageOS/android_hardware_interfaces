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

#include <mapper-vts/2.1/MapperVts.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace vts {

using V2_0::Error;

// abuse VTS to check binary compatibility between BufferDescriptorInfos
using OldBufferDescriptorInfo =
    android::hardware::graphics::mapper::V2_0::IMapper::BufferDescriptorInfo;
static_assert(sizeof(OldBufferDescriptorInfo) == sizeof(IMapper::BufferDescriptorInfo) &&
                  offsetof(OldBufferDescriptorInfo, width) ==
                      offsetof(IMapper::BufferDescriptorInfo, width) &&
                  offsetof(OldBufferDescriptorInfo, height) ==
                      offsetof(IMapper::BufferDescriptorInfo, height) &&
                  offsetof(OldBufferDescriptorInfo, layerCount) ==
                      offsetof(IMapper::BufferDescriptorInfo, layerCount) &&
                  offsetof(OldBufferDescriptorInfo, format) ==
                      offsetof(IMapper::BufferDescriptorInfo, format) &&
                  offsetof(OldBufferDescriptorInfo, usage) ==
                      offsetof(IMapper::BufferDescriptorInfo, usage),
              "");

Gralloc::Gralloc(bool errOnFailure) : V2_0::vts::Gralloc() {
    if (::testing::Test::HasFatalFailure()) {
        return;
    }
    init(errOnFailure);
}

Gralloc::Gralloc(const std::string& allocatorServiceName, const std::string& mapperServiceName,
                 bool errOnFailure)
    : V2_0::vts::Gralloc(allocatorServiceName, mapperServiceName) {
    if (::testing::Test::HasFatalFailure()) {
        return;
    }
    init(errOnFailure);
}

void Gralloc::init(bool errOnFailure) {
    mMapperV2_1 = IMapper::castFrom(V2_0::vts::Gralloc::getMapper());
    if (errOnFailure) ASSERT_NE(nullptr, mMapperV2_1.get()) << "failed to get mapper 2.1 service";
}

sp<IMapper> Gralloc::getMapper() const {
    return mMapperV2_1;
}

bool Gralloc::validateBufferSize(const native_handle_t* bufferHandle,
                                 const IMapper::BufferDescriptorInfo& descriptorInfo,
                                 uint32_t stride) {
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    Error error = mMapperV2_1->validateBufferSize(buffer, descriptorInfo, stride);
    return error == Error::NONE;
}

void Gralloc::getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                               uint32_t* outNumInts) {
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    *outNumFds = 0;
    *outNumInts = 0;
    mMapperV2_1->getTransportSize(
        buffer, [&](const auto& tmpError, const auto& tmpNumFds, const auto& tmpNumInts) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get transport size";
            ASSERT_GE(bufferHandle->numFds, int(tmpNumFds)) << "invalid numFds " << tmpNumFds;
            ASSERT_GE(bufferHandle->numInts, int(tmpNumInts)) << "invalid numInts " << tmpNumInts;

            *outNumFds = tmpNumFds;
            *outNumInts = tmpNumInts;
        });
}

BufferDescriptor Gralloc::createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo) {
    BufferDescriptor descriptor;
    mMapperV2_1->createDescriptor_2_1(
        descriptorInfo, [&](const auto& tmpError, const auto& tmpDescriptor) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to create descriptor";
            descriptor = tmpDescriptor;
        });

    return descriptor;
}

const native_handle_t* Gralloc::allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                         bool import, uint32_t* outStride) {
    BufferDescriptor descriptor = createDescriptor(descriptorInfo);
    if (::testing::Test::HasFatalFailure()) {
        return nullptr;
    }

    auto buffers = V2_0::vts::Gralloc::allocate(descriptor, 1, import, outStride);
    if (::testing::Test::HasFatalFailure()) {
        return nullptr;
    }

    return buffers[0];
}

}  // namespace vts
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
