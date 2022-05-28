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

#undef LOG_TAG
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
#include <hwui/Bitmap.h>
#include <renderthread/EglManager.h>
#include <utils/GLUtils.h>
#include <vndk/hardware_buffer.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <tuple>

using namespace aidl::android::hardware::graphics::allocator;
using namespace aidl::android::hardware::graphics::common;
using namespace android;
using namespace android::hardware;
using namespace android::hardware::graphics::mapper::V4_0;
using android::uirenderer::AutoEglImage;
using android::uirenderer::AutoGLFramebuffer;
using android::uirenderer::AutoSkiaGlTexture;
using android::uirenderer::renderthread::EglManager;

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
    const IMapper::BufferDescriptorInfo mInfo;

    BufferHandle(const BufferHandle&) = delete;
    void operator=(const BufferHandle&) = delete;

  public:
    BufferHandle(const sp<IMapper> mapper, native_handle_t* handle, bool imported, uint32_t stride,
                 const IMapper::BufferDescriptorInfo& info)
        : mMapper(mapper), mRawHandle(handle), mImported(imported), mStride(stride), mInfo(info) {}

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

    AHardwareBuffer_Desc describe() const {
        return {
                .width = mInfo.width,
                .height = mInfo.height,
                .layers = mInfo.layerCount,
                .format = static_cast<uint32_t>(mInfo.format),
                .usage = mInfo.usage,
                .stride = stride(),
                .rfu0 = 0,
                .rfu1 = 0,
        };
    }

    AHardwareBuffer* createAHardwareBuffer() const {
        auto desc = describe();
        AHardwareBuffer* buffer = nullptr;
        int err = AHardwareBuffer_createFromHandle(
                &desc, mRawHandle, AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE, &buffer);
        EXPECT_EQ(0, err) << "Failed to AHardwareBuffer_createFromHandle";
        return err ? nullptr : buffer;
    }
};

class GraphicsTestsBase {
  private:
    std::shared_ptr<IAllocator> mAllocator;
    sp<IMapper> mMapper;

  protected:
    void Initialize(std::string allocatorService, std::string mapperService) {
        mAllocator = IAllocator::fromBinder(
                ndk::SpAIBinder(AServiceManager_checkService(allocatorService.c_str())));
        mMapper = IMapper::getService(mapperService);

        ASSERT_NE(nullptr, mAllocator.get()) << "failed to get allocator service";
        ASSERT_NE(nullptr, mMapper.get()) << "failed to get mapper service";
        ASSERT_FALSE(mMapper->isRemote()) << "mapper is not in passthrough mode";
    }

  public:
    BufferDescriptor createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo) {
        BufferDescriptor descriptor;
        mMapper->createDescriptor(
                descriptorInfo, [&](const auto& tmpError, const auto& tmpDescriptor) {
                    ASSERT_EQ(Error::NONE, tmpError) << "failed to create descriptor";
                    descriptor = tmpDescriptor;
                });

        return descriptor;
    }

    std::unique_ptr<BufferHandle> allocate(const IMapper::BufferDescriptorInfo& descriptorInfo) {
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
            } else {
                EXPECT_EQ(OK, error) << "Allocation transport failure";
            }
            return nullptr;
        } else {
            return std::make_unique<BufferHandle>(mMapper, dupFromAidl(result.buffers[0]), false,
                                                  result.stride, descriptorInfo);
        }
    }

    bool isSupported(const IMapper::BufferDescriptorInfo& descriptorInfo) {
        bool ret = false;
        EXPECT_TRUE(mMapper->isSupported(descriptorInfo,
                                         [&](auto error, bool supported) {
                                             ASSERT_EQ(Error::NONE, error);
                                             ret = supported;
                                         })
                            .isOk());
        return ret;
    }
};

class GraphicsAllocatorAidlTests
    : public GraphicsTestsBase,
      public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
  public:
    void SetUp() override { Initialize(std::get<0>(GetParam()), std::get<1>(GetParam())); }

    void TearDown() override {}
};

struct FlushMethod {
    std::string name;
    std::function<void(EglManager&)> func;
};

class GraphicsFrontBufferTests
    : public GraphicsTestsBase,
      public ::testing::TestWithParam<std::tuple<std::string, std::string, FlushMethod>> {
  private:
    EglManager eglManager;
    std::function<void(EglManager&)> flush;

  public:
    void SetUp() override {
        Initialize(std::get<0>(GetParam()), std::get<1>(GetParam()));
        flush = std::get<2>(GetParam()).func;
        eglManager.initialize();
    }

    void TearDown() override { eglManager.destroy(); }

    void fillWithGpu(AHardwareBuffer* buffer, float red, float green, float blue, float alpha) {
        const EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID(buffer);
        AutoEglImage eglImage(eglManager.eglDisplay(), clientBuffer);
        AutoSkiaGlTexture glTexture;
        AutoGLFramebuffer glFbo;
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage.image);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               glTexture.mTexture, 0);

        AHardwareBuffer_Desc desc;
        AHardwareBuffer_describe(buffer, &desc);
        glViewport(0, 0, desc.width, desc.height);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(red, green, blue, alpha);
        glClear(GL_COLOR_BUFFER_BIT);
        flush(eglManager);
    }

    void fillWithGpu(AHardwareBuffer* buffer, /*RGBA*/ uint32_t color) {
        // Keep it simple for now
        static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
        float a = float((color >> 24) & 0xff) / 255.0f;
        float b = float((color >> 16) & 0xff) / 255.0f;
        float g = float((color >> 8) & 0xff) / 255.0f;
        float r = float((color)&0xff) / 255.0f;
        fillWithGpu(buffer, r, g, b, a);
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

TEST_P(GraphicsFrontBufferTests, FrontBufferGpuToCpu) {
    IMapper::BufferDescriptorInfo info{
            .name = "CPU_8888",
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = cast(PixelFormat::RGBA_8888),
            .usage = pack({BufferUsage::GPU_RENDER_TARGET, BufferUsage::CPU_READ_OFTEN,
                           BufferUsage::FRONT_BUFFER}),
            .reservedSize = 0,
    };
    const bool supported = isSupported(info);
    auto buffer = allocate(info);
    if (!supported) {
        ASSERT_EQ(nullptr, buffer.get())
                << "Allocation succeeded, but IMapper::isSupported was false";
        GTEST_SKIP();
    } else {
        ASSERT_NE(nullptr, buffer.get()) << "Allocation failed, but IMapper::isSupported was true";
    }

    AHardwareBuffer* ahb = buffer->createAHardwareBuffer();
    ASSERT_NE(nullptr, ahb);

    // We draw 3 times with 3 different colors to ensure the flush is consistently flushing.
    // Particularly for glFlush() there's occasions where it seems something triggers a flush
    // to happen even though glFlush itself isn't consistently doing so, but for FRONT_BUFFER
    // bound buffers it is supposed to consistently flush.
    for (uint32_t color : {0xFF0000FFu, 0x00FF00FFu, 0x0000FFFFu}) {
        fillWithGpu(ahb, color);
        uint32_t* addr;
        ASSERT_EQ(0, AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr,
                                          (void**)&addr));
        // Spot check a few pixels
        EXPECT_EQ(color, addr[0]);
        EXPECT_EQ(color, addr[32 + (32 * buffer->stride())]);
        AHardwareBuffer_unlock(ahb, nullptr);
    }

    AHardwareBuffer_release(ahb);
}

TEST_P(GraphicsFrontBufferTests, FrontBufferGpuToGpu) {
    IMapper::BufferDescriptorInfo info{
            .name = "CPU_8888",
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = cast(PixelFormat::RGBA_8888),
            .usage = pack({BufferUsage::GPU_RENDER_TARGET, BufferUsage::GPU_TEXTURE,
                           BufferUsage::FRONT_BUFFER}),
            .reservedSize = 0,
    };
    const bool supported = isSupported(info);
    auto buffer = allocate(info);
    if (!supported) {
        ASSERT_EQ(nullptr, buffer.get())
                << "Allocation succeeded, but IMapper::isSupported was false";
        GTEST_SKIP();
    } else {
        ASSERT_NE(nullptr, buffer.get()) << "Allocation failed, but IMapper::isSupported was true";
    }

    AHardwareBuffer* ahb = buffer->createAHardwareBuffer();
    ASSERT_NE(nullptr, ahb);

    // We draw 3 times with 3 different colors to ensure the flush is consistently flushing.
    // Particularly for glFlush() there's occasions where it seems something triggers a flush
    // to happen even though glFlush itself isn't consistently doing so, but for FRONT_BUFFER
    // bound buffers it is supposed to consistently flush.
    for (uint32_t color : {0xFF0000FFu, 0x00FF00FFu, 0x0000FFFFu}) {
        fillWithGpu(ahb, color);
        sk_sp<Bitmap> hwBitmap = Bitmap::createFrom(ahb, SkColorSpace::MakeSRGB());
        SkBitmap cpuBitmap = hwBitmap->getSkBitmap();
        // Spot check a few pixels
        EXPECT_EQ(color, *cpuBitmap.getAddr32(0, 0));
        EXPECT_EQ(color, *cpuBitmap.getAddr32(16, 30));
    }

    AHardwareBuffer_release(ahb);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsAllocatorAidlTests);
INSTANTIATE_TEST_CASE_P(
        PerInstance, GraphicsAllocatorAidlTests,
        testing::Combine(testing::ValuesIn(getAidlHalInstanceNames(IAllocator::descriptor)),
                         testing::ValuesIn(getAllHalInstanceNames(IMapper::descriptor))),
        PrintInstanceTupleNameToString<>);

const auto FlushMethodsValues = testing::Values(
        FlushMethod{"glFinish", [](EglManager&) { glFinish(); }},
        FlushMethod{"glFlush",
                    [](EglManager&) {
                        glFlush();
                        // Since the goal is to verify that glFlush() actually flushes, we can't
                        // wait on any sort of fence since that will change behavior So instead we
                        // just sleep & hope
                        sleep(1);
                    }},
        FlushMethod{"eglClientWaitSync", [](EglManager& eglManager) {
                        EGLDisplay display = eglManager.eglDisplay();
                        EGLSyncKHR fence = eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, NULL);
                        eglClientWaitSyncKHR(display, fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                             EGL_FOREVER_KHR);
                        eglDestroySyncKHR(display, fence);
                    }});
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsFrontBufferTests);
INSTANTIATE_TEST_CASE_P(
        PerInstance, GraphicsFrontBufferTests,
        testing::Combine(testing::ValuesIn(getAidlHalInstanceNames(IAllocator::descriptor)),
                         testing::ValuesIn(getAllHalInstanceNames(IMapper::descriptor)),
                         FlushMethodsValues),
        [](auto info) -> std::string {
            std::string name = std::to_string(info.index) + "/" + std::get<2>(info.param).name;
            return Sanitize(name);
        });
