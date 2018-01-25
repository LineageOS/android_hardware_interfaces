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

#define LOG_TAG "VtsHalGraphicsMapperV2_1TargetTest"

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <sync/sync.h>
#include "VtsHalGraphicsMapperTestUtils.h"

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace tests {
namespace {

using android::hardware::graphics::mapper::V2_0::BufferDescriptor;
using android::hardware::graphics::mapper::V2_0::Error;

using android::hardware::graphics::common::V1_1::BufferUsage;
using android::hardware::graphics::common::V1_1::PixelFormat;

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

class Gralloc : public V2_0::tests::Gralloc {
   public:
    Gralloc() : V2_0::tests::Gralloc() {
        if (::testing::Test::HasFatalFailure()) {
            return;
        }

        init();
    }

    sp<IMapper> getMapper() const { return mMapper; }

    bool validateBufferSize(const native_handle_t* bufferHandle,
                            const IMapper::BufferDescriptorInfo& descriptorInfo, uint32_t stride) {
        auto buffer = const_cast<native_handle_t*>(bufferHandle);

        Error error = mMapper->validateBufferSize(buffer, descriptorInfo, stride);
        return error == Error::NONE;
    }

    void getTransportSize(const native_handle_t* bufferHandle, uint32_t* numFds,
                          uint32_t* numInts) {
        auto buffer = const_cast<native_handle_t*>(bufferHandle);

        *numFds = 0;
        *numInts = 0;
        mMapper->getTransportSize(buffer, [&](const auto& tmpError, const auto& tmpNumFds,
                                              const auto& tmpNumInts) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get transport size";
            ASSERT_GE(bufferHandle->numFds, int(tmpNumFds)) << "invalid numFds " << tmpNumFds;
            ASSERT_GE(bufferHandle->numInts, int(tmpNumInts)) << "invalid numInts " << tmpNumInts;

            *numFds = tmpNumFds;
            *numInts = tmpNumInts;
        });
    }

    BufferDescriptor createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo) {
        BufferDescriptor descriptor;
        mMapper->createDescriptor_2_1(
            descriptorInfo, [&](const auto& tmpError, const auto& tmpDescriptor) {
                ASSERT_EQ(Error::NONE, tmpError) << "failed to create descriptor";
                descriptor = tmpDescriptor;
            });

        return descriptor;
    }

    const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                    bool import, uint32_t* outStride = nullptr) {
        BufferDescriptor descriptor = createDescriptor(descriptorInfo);
        if (::testing::Test::HasFatalFailure()) {
            return nullptr;
        }

        auto buffers = V2_0::tests::Gralloc::allocate(descriptor, 1, import, outStride);
        if (::testing::Test::HasFatalFailure()) {
            return nullptr;
        }

        return buffers[0];
    }

   private:
    void init() {
        mMapper = IMapper::castFrom(V2_0::tests::Gralloc::getMapper());
        ASSERT_NE(nullptr, mMapper.get()) << "failed to find IMapper 2.1";
    }

    sp<IMapper> mMapper;
};

class GraphicsMapperHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(mGralloc = std::make_unique<Gralloc>());

        mDummyDescriptorInfo.width = 64;
        mDummyDescriptorInfo.height = 64;
        mDummyDescriptorInfo.layerCount = 1;
        mDummyDescriptorInfo.format = PixelFormat::RGBA_8888;
        mDummyDescriptorInfo.usage =
            static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);
    }

    void TearDown() override {}

    std::unique_ptr<Gralloc> mGralloc;
    IMapper::BufferDescriptorInfo mDummyDescriptorInfo{};
};

/**
 * Test that IMapper::validateBufferSize works.
 */
TEST_F(GraphicsMapperHidlTest, ValidateBufferSizeBasic) {
    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true, &stride));

    ASSERT_TRUE(mGralloc->validateBufferSize(bufferHandle, mDummyDescriptorInfo, stride));

    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(bufferHandle));
}

/**
 * Test IMapper::validateBufferSize with invalid buffers.
 */
TEST_F(GraphicsMapperHidlTest, ValidateBufferSizeBadBuffer) {
    native_handle_t* invalidHandle = nullptr;
    Error ret = mGralloc->getMapper()->validateBufferSize(invalidHandle, mDummyDescriptorInfo,
                                                          mDummyDescriptorInfo.width);
    ASSERT_EQ(Error::BAD_BUFFER, ret)
        << "validateBufferSize with nullptr did not fail with BAD_BUFFER";

    invalidHandle = native_handle_create(0, 0);
    ret = mGralloc->getMapper()->validateBufferSize(invalidHandle, mDummyDescriptorInfo,
                                                    mDummyDescriptorInfo.width);
    ASSERT_EQ(Error::BAD_BUFFER, ret)
        << "validateBufferSize with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(invalidHandle);

    native_handle_t* rawBufferHandle;
    ASSERT_NO_FATAL_FAILURE(rawBufferHandle = const_cast<native_handle_t*>(
                                mGralloc->allocate(mDummyDescriptorInfo, false)));
    ret = mGralloc->getMapper()->validateBufferSize(rawBufferHandle, mDummyDescriptorInfo,
                                                    mDummyDescriptorInfo.width);
    ASSERT_EQ(Error::BAD_BUFFER, ret)
        << "validateBufferSize with raw buffer handle did not fail with BAD_BUFFER";
    native_handle_delete(rawBufferHandle);
}

/**
 * Test IMapper::validateBufferSize with invalid descriptor and/or stride.
 */
TEST_F(GraphicsMapperHidlTest, ValidateBufferSizeBadValue) {
    auto info = mDummyDescriptorInfo;
    info.width = 1024;
    info.height = 1024;
    info.layerCount = 1;
    info.format = PixelFormat::RGBA_8888;

    native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(
        bufferHandle = const_cast<native_handle_t*>(mGralloc->allocate(info, true, &stride)));

    // All checks below test if a 8MB buffer can fit in a 4MB buffer.
    info.width *= 2;
    Error ret = mGralloc->getMapper()->validateBufferSize(bufferHandle, info, stride);
    ASSERT_EQ(Error::BAD_VALUE, ret)
        << "validateBufferSize with bad width did not fail with BAD_VALUE";
    info.width /= 2;

    info.height *= 2;
    ret = mGralloc->getMapper()->validateBufferSize(bufferHandle, info, stride);
    ASSERT_EQ(Error::BAD_VALUE, ret)
        << "validateBufferSize with bad height did not fail with BAD_VALUE";
    info.height /= 2;

    info.layerCount *= 2;
    ret = mGralloc->getMapper()->validateBufferSize(bufferHandle, info, stride);
    ASSERT_EQ(Error::BAD_VALUE, ret)
        << "validateBufferSize with bad layer count did not fail with BAD_VALUE";
    info.layerCount /= 2;

    info.format = PixelFormat::RGBA_FP16;
    ret = mGralloc->getMapper()->validateBufferSize(bufferHandle, info, stride);
    ASSERT_EQ(Error::BAD_VALUE, ret)
        << "validateBufferSize with bad format did not fail with BAD_VALUE";
    info.format = PixelFormat::RGBA_8888;

    ret = mGralloc->getMapper()->validateBufferSize(bufferHandle, mDummyDescriptorInfo, stride * 2);
    ASSERT_EQ(Error::BAD_VALUE, ret)
        << "validateBufferSize with bad stride did not fail with BAD_VALUE";

    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(bufferHandle));
}

/**
 * Test IMapper::getTransportSize.
 */
TEST_F(GraphicsMapperHidlTest, GetTransportSizeBasic) {
    const native_handle_t* bufferHandle;
    uint32_t numFds;
    uint32_t numInts;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));
    ASSERT_NO_FATAL_FAILURE(mGralloc->getTransportSize(bufferHandle, &numFds, &numInts));
    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(bufferHandle));
}

/**
 * Test IMapper::getTransportSize with invalid buffers.
 */
TEST_F(GraphicsMapperHidlTest, GetTransportSizeBadBuffer) {
    native_handle_t* invalidHandle = nullptr;
    mGralloc->getMapper()->getTransportSize(
        invalidHandle, [&](const auto& tmpError, const auto&, const auto&) {
            ASSERT_EQ(Error::BAD_BUFFER, tmpError)
                << "getTransportSize with nullptr did not fail with BAD_BUFFER";
        });

    invalidHandle = native_handle_create(0, 0);
    mGralloc->getMapper()->getTransportSize(
        invalidHandle, [&](const auto& tmpError, const auto&, const auto&) {
            ASSERT_EQ(Error::BAD_BUFFER, tmpError)
                << "getTransportSize with invalid handle did not fail with BAD_BUFFER";
        });
    native_handle_delete(invalidHandle);

    native_handle_t* rawBufferHandle;
    ASSERT_NO_FATAL_FAILURE(rawBufferHandle = const_cast<native_handle_t*>(
                                mGralloc->allocate(mDummyDescriptorInfo, false)));
    mGralloc->getMapper()->getTransportSize(
        invalidHandle, [&](const auto& tmpError, const auto&, const auto&) {
            ASSERT_EQ(Error::BAD_BUFFER, tmpError)
                << "getTransportSize with raw buffer handle did not fail with BAD_BUFFER";
        });
    native_handle_delete(rawBufferHandle);
}

/**
 * Test IMapper::createDescriptor with valid descriptor info.
 */
TEST_F(GraphicsMapperHidlTest, CreateDescriptor_2_1Basic) {
    ASSERT_NO_FATAL_FAILURE(mGralloc->createDescriptor(mDummyDescriptorInfo));
}

/**
 * Test IMapper::createDescriptor with invalid descriptor info.
 */
TEST_F(GraphicsMapperHidlTest, CreateDescriptor_2_1Negative) {
    auto info = mDummyDescriptorInfo;
    info.width = 0;
    mGralloc->getMapper()->createDescriptor_2_1(info, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_VALUE, tmpError) << "createDescriptor did not fail with BAD_VALUE";
    });
}

}  // namespace
}  // namespace tests
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::mapper::V2_0::tests::GraphicsMapperHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsMapperHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsMapperHidlEnvironment::Instance()->init(&argc, argv);

    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;

    return status;
}
