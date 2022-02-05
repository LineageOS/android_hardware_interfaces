/*
 * Copyright 2022 The Android Open Source Project
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

#define LOG_TAG "VtsHalGraphicsAllocatorAidl_TargetTest"

#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidl/android/hardware/graphics/allocator/AllocationResult.h>
#include <aidl/android/hardware/graphics/allocator/IAllocator.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_manager.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <tuple>

using namespace aidl::android::hardware::graphics::allocator;
using namespace aidl::android::hardware::graphics::common;
using namespace android;
using namespace android::hardware;
using namespace android::hardware::graphics::mapper::V4_0;

static constexpr uint64_t pack(const std::initializer_list<BufferUsage>& usages) {
    uint64_t ret = 0;
    for (const auto u : usages) {
        ret |= static_cast<uint64_t>(u);
    }
    return ret;
}

static constexpr hardware::graphics::common::V1_2::PixelFormat cast(PixelFormat format) {
    return static_cast<hardware::graphics::common::V1_2::PixelFormat>(format);
}

class BufferHandle {
    sp<IMapper> mMapper;
    native_handle_t* mRawHandle;
    bool mImported = false;
    uint32_t mStride;

    BufferHandle(const BufferHandle&) = delete;
    void operator=(const BufferHandle&) = delete;

  public:
    BufferHandle(const sp<IMapper> mapper, native_handle_t* handle, bool imported, uint32_t stride)
        : mMapper(mapper), mRawHandle(handle), mImported(imported), mStride(stride) {}

    ~BufferHandle() {
        if (mRawHandle == nullptr) return;

        if (mImported) {
            Error error = mMapper->freeBuffer(mRawHandle);
            EXPECT_EQ(Error::NONE, error) << "failed to free buffer " << mRawHandle;
        } else {
            native_handle_close(mRawHandle);
            native_handle_delete(mRawHandle);
        }
    }

    uint32_t stride() const { return mStride; }
};

class GraphicsAllocatorAidlTests
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
  private:
    std::shared_ptr<IAllocator> mAllocator;
    sp<IMapper> mMapper;

  public:
    void SetUp() override {
        mAllocator = IAllocator::fromBinder(
                ndk::SpAIBinder(AServiceManager_checkService(std::get<0>(GetParam()).c_str())));
        mMapper = IMapper::getService(std::get<1>(GetParam()));

        ASSERT_NE(nullptr, mAllocator.get()) << "failed to get allocator service";
        ASSERT_NE(nullptr, mMapper.get()) << "failed to get mapper service";
        ASSERT_FALSE(mMapper->isRemote()) << "mapper is not in passthrough mode";
    }

    void TearDown() override {}

    BufferDescriptor createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo) {
        BufferDescriptor descriptor;
        mMapper->createDescriptor(
                descriptorInfo, [&](const auto& tmpError, const auto& tmpDescriptor) {
                    ASSERT_EQ(Error::NONE, tmpError) << "failed to create descriptor";
                    descriptor = tmpDescriptor;
                });

        return descriptor;
    }

    native_handle_t* importBuffer(const hidl_handle& rawHandle) {
        native_handle_t* bufferHandle = nullptr;
        mMapper->importBuffer(rawHandle, [&](const auto& tmpError, const auto& tmpBuffer) {
            ASSERT_EQ(Error::NONE, tmpError)
                    << "failed to import buffer %p" << rawHandle.getNativeHandle();
            bufferHandle = static_cast<native_handle_t*>(tmpBuffer);
        });
        return bufferHandle;
    }

    std::unique_ptr<BufferHandle> allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                           bool import = false) {
        auto descriptor = createDescriptor(descriptorInfo);
        if (::testing::Test::HasFatalFailure()) {
            return nullptr;
        }

        AllocationResult result;
        auto status = mAllocator->allocate(descriptor, 1, &result);
        if (!status.isOk()) {
            status_t error = status.getExceptionCode();
            if (error == EX_SERVICE_SPECIFIC) {
                error = status.getServiceSpecificError();
                EXPECT_NE(OK, error) << "Failed to set error properly";
                EXPECT_EQ(OK, error) << "Failed to allocate";
            } else {
                EXPECT_EQ(OK, error) << "Allocation transport failure";
            }
            return nullptr;
        } else {
            if (import) {
                native_handle_t* importedHandle = importBuffer(makeFromAidl(result.buffers[0]));
                if (importedHandle) {
                    return std::make_unique<BufferHandle>(mMapper, importedHandle, true,
                                                          result.stride);
                } else {
                    return nullptr;
                }
            } else {
                return std::make_unique<BufferHandle>(mMapper, dupFromAidl(result.buffers[0]),
                                                      false, result.stride);
            }
        }
    }
};

TEST_P(GraphicsAllocatorAidlTests, CreateDescriptorBasic) {
    ASSERT_NO_FATAL_FAILURE(createDescriptor({
            .name = "CPU_8888",
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = cast(PixelFormat::RGBA_8888),
            .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
            .reservedSize = 0,
    }));
}

TEST_P(GraphicsAllocatorAidlTests, CanAllocate) {
    auto buffer = allocate({
            .name = "CPU_8888",
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = cast(PixelFormat::RGBA_8888),
            .usage = pack({BufferUsage::CPU_WRITE_OFTEN, BufferUsage::CPU_READ_OFTEN}),
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsAllocatorAidlTests);
INSTANTIATE_TEST_CASE_P(
        PerInstance, GraphicsAllocatorAidlTests,
        testing::Combine(testing::ValuesIn(getAidlHalInstanceNames(IAllocator::descriptor)),
                         testing::ValuesIn(getAllHalInstanceNames(IMapper::descriptor))),
        PrintInstanceTupleNameToString<>);