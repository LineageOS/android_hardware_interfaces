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
#define LOG_TAG "VtsHalGraphicsMapperStableC_TargetTest"

#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidl/android/hardware/graphics/allocator/AllocationResult.h>
#include <aidl/android/hardware/graphics/allocator/IAllocator.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_enums.h>
#include <android/binder_manager.h>
#include <android/dlext.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>
#include <gralloctypes/Gralloc4.h>
#include <hidl/GtestPrinter.h>
#include <system/graphics.h>

#include <dlfcn.h>
#include <drm/drm_fourcc.h>
#include <gtest/gtest.h>
#include <vndksupport/linker.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace aidl::android::hardware::graphics::allocator;
using namespace aidl::android::hardware::graphics::common;
using namespace android;
using namespace android::hardware;
using namespace ::android::hardware::graphics::mapper;

typedef AIMapper_Error (*AIMapper_loadIMapperFn)(AIMapper* _Nullable* _Nonnull outImplementation);

inline constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
    using T = std::underlying_type_t<BufferUsage>;
    return static_cast<BufferUsage>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline BufferUsage& operator|=(BufferUsage& lhs, BufferUsage rhs) {
    lhs = lhs | rhs;
    return lhs;
}

struct YCbCr {
    android_ycbcr yCbCr;
    int64_t horizontalSubSampling;
    int64_t verticalSubSampling;
};

constexpr const char* STANDARD_METADATA_NAME =
        "android.hardware.graphics.common.StandardMetadataType";

static bool isStandardMetadata(AIMapper_MetadataType metadataType) {
    return strcmp(STANDARD_METADATA_NAME, metadataType.name) == 0;
}

static std::string toString(const std::vector<StandardMetadataType> types) {
    std::stringstream buf;
    buf << "[";
    for (auto type : types) {
        buf << toString(type) << ", ";
    }
    buf.seekp(-2, buf.cur);
    buf << "]";
    return buf.str();
}

class BufferHandle {
    AIMapper* mIMapper;
    buffer_handle_t mHandle = nullptr;

  public:
    explicit BufferHandle(AIMapper* mapper, native_handle_t* rawHandle) : mIMapper(mapper) {
        EXPECT_EQ(AIMAPPER_ERROR_NONE, mIMapper->v5.importBuffer(rawHandle, &mHandle));
    }

    explicit BufferHandle(BufferHandle&& other) { *this = std::move(other); }

    BufferHandle& operator=(BufferHandle&& other) noexcept {
        reset();
        mIMapper = other.mIMapper;
        mHandle = other.mHandle;
        other.mHandle = nullptr;
        return *this;
    }

    ~BufferHandle() { reset(); }

    constexpr explicit operator bool() const noexcept { return mHandle != nullptr; }

    buffer_handle_t operator*() const noexcept { return mHandle; }

    void reset() {
        if (mHandle != nullptr) {
            EXPECT_EQ(AIMAPPER_ERROR_NONE, mIMapper->v5.freeBuffer(mHandle));
            mHandle = nullptr;
        }
    }
};

class BufferAllocation {
    AIMapper* mIMapper;
    native_handle_t* mRawHandle;
    uint32_t mStride;
    const BufferDescriptorInfo mInfo;

  public:
    BufferAllocation(const BufferAllocation&) = delete;
    void operator=(const BufferAllocation&) = delete;

    BufferAllocation(AIMapper* mapper, native_handle_t* handle, uint32_t stride,
                     const BufferDescriptorInfo& info)
        : mIMapper(mapper), mRawHandle(handle), mStride(stride), mInfo(info) {}

    ~BufferAllocation() {
        if (mRawHandle == nullptr) return;

        native_handle_close(mRawHandle);
        native_handle_delete(mRawHandle);
    }

    uint32_t stride() const { return mStride; }
    const BufferDescriptorInfo& info() const { return mInfo; }

    BufferHandle import() { return BufferHandle{mIMapper, mRawHandle}; }

    const native_handle_t* rawHandle() const { return mRawHandle; }
};

class GraphicsTestsBase {
  private:
    friend class BufferAllocation;
    int32_t mIAllocatorVersion = 1;
    std::shared_ptr<IAllocator> mAllocator;
    AIMapper* mIMapper = nullptr;
    AIMapper_loadIMapperFn mIMapperLoader;
    int32_t* mIMapperHALVersion = nullptr;

  protected:
    void Initialize(std::shared_ptr<IAllocator> allocator) {
        mAllocator = allocator;
        ASSERT_NE(nullptr, mAllocator.get()) << "failed to get allocator service";
        ASSERT_TRUE(mAllocator->getInterfaceVersion(&mIAllocatorVersion).isOk());
        ASSERT_GE(mIAllocatorVersion, 2);
        std::string mapperSuffix;
        auto status = mAllocator->getIMapperLibrarySuffix(&mapperSuffix);
        ASSERT_TRUE(status.isOk()) << "Failed to get IMapper library suffix";
        std::string lib_name = "mapper." + mapperSuffix + ".so";
        void* so = android_load_sphal_library(lib_name.c_str(), RTLD_LOCAL | RTLD_NOW);
        ASSERT_NE(nullptr, so) << "Failed to load " << lib_name;
        mIMapperLoader = (AIMapper_loadIMapperFn)dlsym(so, "AIMapper_loadIMapper");
        ASSERT_NE(nullptr, mIMapperLoader) << "AIMapper_locaIMapper missing from " << lib_name;
        ASSERT_EQ(AIMAPPER_ERROR_NONE, mIMapperLoader(&mIMapper));
        ASSERT_NE(mIMapper, nullptr);
        mIMapperHALVersion = (int32_t*)dlsym(so, "ANDROID_HAL_MAPPER_VERSION");
    }

  public:
    AIMapper_loadIMapperFn getIMapperLoader() const { return mIMapperLoader; }
    int32_t* getHalVersion() const { return mIMapperHALVersion; }

    std::unique_ptr<BufferAllocation> allocate(const BufferDescriptorInfo& descriptorInfo) {
        AllocationResult result;
        ::ndk::ScopedAStatus status = mAllocator->allocate2(descriptorInfo, 1, &result);
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
            return std::make_unique<BufferAllocation>(mIMapper, dupFromAidl(result.buffers[0]),
                                                      result.stride, descriptorInfo);
        }
    }

    std::unique_ptr<BufferAllocation> allocateGeneric() {
        return allocate({
                .name = {"VTS_TEMP"},
                .width = 64,
                .height = 64,
                .layerCount = 1,
                .format = PixelFormat::RGBA_8888,
                .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
                .reservedSize = 0,
        });
    }

    bool isSupported(const BufferDescriptorInfo& descriptorInfo) {
        bool ret = false;
        EXPECT_TRUE(mAllocator->isSupported(descriptorInfo, &ret).isOk());
        return ret;
    }

    AIMapper* mapper() const { return mIMapper; }

    template <StandardMetadataType T>
    auto getStandardMetadata(buffer_handle_t bufferHandle)
            -> decltype(StandardMetadata<T>::value::decode(nullptr, 0)) {
        using Value = typename StandardMetadata<T>::value;
        std::vector<uint8_t> buffer;
        // Initial guess
        buffer.resize(512);
        int32_t sizeRequired = mapper()->v5.getStandardMetadata(
                bufferHandle, static_cast<int64_t>(T), buffer.data(), buffer.size());
        if (sizeRequired < 0) {
            EXPECT_EQ(-AIMAPPER_ERROR_UNSUPPORTED, sizeRequired)
                    << "Received something other than UNSUPPORTED from valid getStandardMetadata "
                       "call";
            return std::nullopt;
        }
        if (sizeRequired > buffer.size()) {
            buffer.resize(sizeRequired);
            sizeRequired = mapper()->v5.getStandardMetadata(bufferHandle, static_cast<int64_t>(T),
                                                            buffer.data(), buffer.size());
        }
        if (sizeRequired < 0 || sizeRequired > buffer.size()) {
            ADD_FAILURE() << "getStandardMetadata failed, received " << sizeRequired
                          << " with buffer size " << buffer.size();
            // Generate a fail type
            return std::nullopt;
        }
        return Value::decode(buffer.data(), sizeRequired);
    }

    template <StandardMetadataType T>
    AIMapper_Error setStandardMetadata(buffer_handle_t bufferHandle,
                                       const typename StandardMetadata<T>::value_type& value) {
        using Value = typename StandardMetadata<T>::value;
        int32_t sizeRequired = Value::encode(value, nullptr, 0);
        if (sizeRequired < 0) {
            EXPECT_GE(sizeRequired, 0) << "Failed to calculate required size";
            return static_cast<AIMapper_Error>(-sizeRequired);
        }
        std::vector<uint8_t> buffer;
        buffer.resize(sizeRequired);
        sizeRequired = Value::encode(value, buffer.data(), buffer.size());
        if (sizeRequired < 0 || sizeRequired > buffer.size()) {
            ADD_FAILURE() << "Failed to encode with calculated size " << sizeRequired
                          << "; buffer size" << buffer.size();
            return static_cast<AIMapper_Error>(-sizeRequired);
        }
        return mapper()->v5.setStandardMetadata(bufferHandle, static_cast<int64_t>(T),
                                                buffer.data(), sizeRequired);
    }

    void verifyRGBA8888PlaneLayouts(const std::vector<PlaneLayout>& planeLayouts) {
        ASSERT_EQ(1, planeLayouts.size());

        const auto& planeLayout = planeLayouts.front();

        ASSERT_EQ(4, planeLayout.components.size());

        int64_t offsetInBitsR = -1;
        int64_t offsetInBitsG = -1;
        int64_t offsetInBitsB = -1;
        int64_t offsetInBitsA = -1;

        for (const auto& component : planeLayout.components) {
            if (!gralloc4::isStandardPlaneLayoutComponentType(component.type)) {
                continue;
            }
            EXPECT_EQ(8, component.sizeInBits);
            if (component.type.value == gralloc4::PlaneLayoutComponentType_R.value) {
                offsetInBitsR = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_G.value) {
                offsetInBitsG = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_B.value) {
                offsetInBitsB = component.offsetInBits;
            }
            if (component.type.value == gralloc4::PlaneLayoutComponentType_A.value) {
                offsetInBitsA = component.offsetInBits;
            }
        }

        EXPECT_EQ(0, offsetInBitsR);
        EXPECT_EQ(8, offsetInBitsG);
        EXPECT_EQ(16, offsetInBitsB);
        EXPECT_EQ(24, offsetInBitsA);

        EXPECT_EQ(0, planeLayout.offsetInBytes);
        EXPECT_EQ(32, planeLayout.sampleIncrementInBits);
        // Skip testing stride because any stride is valid
        EXPECT_LE(planeLayout.widthInSamples * planeLayout.heightInSamples * 4,
                  planeLayout.totalSizeInBytes);
        EXPECT_EQ(1, planeLayout.horizontalSubsampling);
        EXPECT_EQ(1, planeLayout.verticalSubsampling);
    }

    void fillRGBA8888(uint8_t* data, uint32_t height, size_t strideInBytes, size_t widthInBytes) {
        for (uint32_t y = 0; y < height; y++) {
            memset(data, y, widthInBytes);
            data += strideInBytes;
        }
    }

    void verifyRGBA8888(const buffer_handle_t bufferHandle, const uint8_t* data, uint32_t height,
                        size_t strideInBytes, size_t widthInBytes) {
        auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(bufferHandle);
        ASSERT_TRUE(decodeResult.has_value());
        const auto& planeLayouts = *decodeResult;
        ASSERT_TRUE(planeLayouts.size() > 0);

        verifyRGBA8888PlaneLayouts(planeLayouts);

        for (uint32_t y = 0; y < height; y++) {
            for (size_t i = 0; i < widthInBytes; i++) {
                EXPECT_EQ(static_cast<uint8_t>(y), data[i]);
            }
            data += strideInBytes;
        }
    }

    void traverseYCbCrData(const android_ycbcr& yCbCr, int32_t width, int32_t height,
                           int64_t hSubsampling, int64_t vSubsampling,
                           std::function<void(uint8_t*, uint8_t)> traverseFuncion) {
        auto yData = static_cast<uint8_t*>(yCbCr.y);
        auto cbData = static_cast<uint8_t*>(yCbCr.cb);
        auto crData = static_cast<uint8_t*>(yCbCr.cr);
        auto yStride = yCbCr.ystride;
        auto cStride = yCbCr.cstride;
        auto chromaStep = yCbCr.chroma_step;

        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                auto val = static_cast<uint8_t>(height * y + x);

                traverseFuncion(yData + yStride * y + x, val);

                if (y % vSubsampling == 0 && x % hSubsampling == 0) {
                    uint32_t subSampleX = x / hSubsampling;
                    uint32_t subSampleY = y / vSubsampling;
                    const auto subSampleOffset = cStride * subSampleY + chromaStep * subSampleX;
                    const auto subSampleVal =
                            static_cast<uint8_t>(height * subSampleY + subSampleX);

                    traverseFuncion(cbData + subSampleOffset, subSampleVal);
                    traverseFuncion(crData + subSampleOffset, subSampleVal + 1);
                }
            }
        }
    }

    void fillYCbCrData(const android_ycbcr& yCbCr, int32_t width, int32_t height,
                       int64_t hSubsampling, int64_t vSubsampling) {
        traverseYCbCrData(yCbCr, width, height, hSubsampling, vSubsampling,
                          [](auto address, auto fillingData) { *address = fillingData; });
    }

    void verifyYCbCrData(const android_ycbcr& yCbCr, int32_t width, int32_t height,
                         int64_t hSubsampling, int64_t vSubsampling) {
        traverseYCbCrData(
                yCbCr, width, height, hSubsampling, vSubsampling,
                [](auto address, auto expectedData) { EXPECT_EQ(*address, expectedData); });
    }

    constexpr uint64_t bitsToBytes(int64_t bits) { return bits / 8; }
    constexpr uint64_t bytesToBits(int64_t bytes) { return bytes * 8; }

    void getAndroidYCbCr(buffer_handle_t bufferHandle, uint8_t* data, android_ycbcr* outYCbCr,
                         int64_t* hSubsampling, int64_t* vSubsampling) {
        auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(bufferHandle);
        ASSERT_TRUE(decodeResult.has_value());
        const auto& planeLayouts = *decodeResult;
        ASSERT_TRUE(planeLayouts.size() > 0);

        outYCbCr->y = nullptr;
        outYCbCr->cb = nullptr;
        outYCbCr->cr = nullptr;
        outYCbCr->ystride = 0;
        outYCbCr->cstride = 0;
        outYCbCr->chroma_step = 0;

        for (const auto& planeLayout : planeLayouts) {
            for (const auto& planeLayoutComponent : planeLayout.components) {
                if (!gralloc4::isStandardPlaneLayoutComponentType(planeLayoutComponent.type)) {
                    continue;
                }
                ASSERT_EQ(0, planeLayoutComponent.offsetInBits % 8);

                uint8_t* tmpData = data + planeLayout.offsetInBytes +
                                   bitsToBytes(planeLayoutComponent.offsetInBits);
                uint64_t sampleIncrementInBytes;

                auto type = static_cast<PlaneLayoutComponentType>(planeLayoutComponent.type.value);
                switch (type) {
                    case PlaneLayoutComponentType::Y:
                        ASSERT_EQ(nullptr, outYCbCr->y);
                        ASSERT_EQ(8, planeLayoutComponent.sizeInBits);
                        ASSERT_EQ(8, planeLayout.sampleIncrementInBits);
                        outYCbCr->y = tmpData;
                        outYCbCr->ystride = planeLayout.strideInBytes;
                        break;

                    case PlaneLayoutComponentType::CB:
                    case PlaneLayoutComponentType::CR:
                        ASSERT_EQ(0, planeLayout.sampleIncrementInBits % 8);

                        sampleIncrementInBytes = planeLayout.sampleIncrementInBits / 8;
                        ASSERT_TRUE(sampleIncrementInBytes == 1 || sampleIncrementInBytes == 2);

                        if (outYCbCr->cstride == 0 && outYCbCr->chroma_step == 0) {
                            outYCbCr->cstride = planeLayout.strideInBytes;
                            outYCbCr->chroma_step = sampleIncrementInBytes;
                        } else {
                            ASSERT_EQ(outYCbCr->cstride, planeLayout.strideInBytes);
                            ASSERT_EQ(outYCbCr->chroma_step, sampleIncrementInBytes);
                        }

                        if (*hSubsampling == 0 && *vSubsampling == 0) {
                            *hSubsampling = planeLayout.horizontalSubsampling;
                            *vSubsampling = planeLayout.verticalSubsampling;
                        } else {
                            ASSERT_EQ(*hSubsampling, planeLayout.horizontalSubsampling);
                            ASSERT_EQ(*vSubsampling, planeLayout.verticalSubsampling);
                        }

                        if (type == PlaneLayoutComponentType::CB) {
                            ASSERT_EQ(nullptr, outYCbCr->cb);
                            outYCbCr->cb = tmpData;
                        } else {
                            ASSERT_EQ(nullptr, outYCbCr->cr);
                            outYCbCr->cr = tmpData;
                        }
                        break;
                    default:
                        break;
                };
            }
        }

        ASSERT_NE(nullptr, outYCbCr->y);
        ASSERT_NE(nullptr, outYCbCr->cb);
        ASSERT_NE(nullptr, outYCbCr->cr);
    }

    YCbCr getAndroidYCbCr_P010(const native_handle_t* bufferHandle, uint8_t* data) {
        YCbCr yCbCr_P010;
        auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(bufferHandle);
        if (!decodeResult.has_value()) {
            ADD_FAILURE() << "failed to get plane layout";
            return YCbCr{};
        }
        const auto& planeLayouts = *decodeResult;
        EXPECT_EQ(2, planeLayouts.size());
        EXPECT_EQ(1, planeLayouts[0].components.size());
        EXPECT_EQ(2, planeLayouts[1].components.size());

        yCbCr_P010.yCbCr.y = nullptr;
        yCbCr_P010.yCbCr.cb = nullptr;
        yCbCr_P010.yCbCr.cr = nullptr;
        yCbCr_P010.yCbCr.ystride = 0;
        yCbCr_P010.yCbCr.cstride = 0;
        yCbCr_P010.yCbCr.chroma_step = 0;
        int64_t cb_offset = 0;
        int64_t cr_offset = 0;

        for (const auto& planeLayout : planeLayouts) {
            for (const auto& planeLayoutComponent : planeLayout.components) {
                if (!gralloc4::isStandardPlaneLayoutComponentType(planeLayoutComponent.type)) {
                    continue;
                }

                uint8_t* tmpData = data + planeLayout.offsetInBytes +
                                   bitsToBytes(planeLayoutComponent.offsetInBits);
                uint64_t sampleIncrementInBytes = 0;
                auto type = static_cast<PlaneLayoutComponentType>(planeLayoutComponent.type.value);
                switch (type) {
                    case PlaneLayoutComponentType::Y:
                        // For specs refer:
                        // https://docs.microsoft.com/en-us/windows/win32/medfound/10-bit-and-16-bit-yuv-video-formats
                        EXPECT_EQ(6, planeLayoutComponent.offsetInBits);
                        EXPECT_EQ(nullptr, yCbCr_P010.yCbCr.y);
                        EXPECT_EQ(10, planeLayoutComponent.sizeInBits);
                        EXPECT_EQ(16, planeLayout.sampleIncrementInBits);

                        yCbCr_P010.yCbCr.y = tmpData;
                        yCbCr_P010.yCbCr.ystride = planeLayout.strideInBytes;
                        break;

                    case PlaneLayoutComponentType::CB:
                    case PlaneLayoutComponentType::CR:
                        sampleIncrementInBytes = bitsToBytes(planeLayout.sampleIncrementInBits);
                        EXPECT_EQ(4, sampleIncrementInBytes);

                        if (yCbCr_P010.yCbCr.cstride == 0 && yCbCr_P010.yCbCr.chroma_step == 0) {
                            yCbCr_P010.yCbCr.cstride = planeLayout.strideInBytes;
                            yCbCr_P010.yCbCr.chroma_step = sampleIncrementInBytes;
                        } else {
                            EXPECT_EQ(yCbCr_P010.yCbCr.cstride, planeLayout.strideInBytes);
                            EXPECT_EQ(yCbCr_P010.yCbCr.chroma_step, sampleIncrementInBytes);
                        }

                        if (yCbCr_P010.horizontalSubSampling == 0 &&
                            yCbCr_P010.verticalSubSampling == 0) {
                            yCbCr_P010.horizontalSubSampling = planeLayout.horizontalSubsampling;
                            yCbCr_P010.verticalSubSampling = planeLayout.verticalSubsampling;
                        } else {
                            EXPECT_EQ(yCbCr_P010.horizontalSubSampling,
                                      planeLayout.horizontalSubsampling);
                            EXPECT_EQ(yCbCr_P010.verticalSubSampling,
                                      planeLayout.verticalSubsampling);
                        }

                        if (type == PlaneLayoutComponentType::CB) {
                            EXPECT_EQ(nullptr, yCbCr_P010.yCbCr.cb);
                            yCbCr_P010.yCbCr.cb = tmpData;
                            cb_offset = planeLayoutComponent.offsetInBits;
                        } else {
                            EXPECT_EQ(nullptr, yCbCr_P010.yCbCr.cr);
                            yCbCr_P010.yCbCr.cr = tmpData;
                            cr_offset = planeLayoutComponent.offsetInBits;
                        }
                        break;
                    default:
                        break;
                };
            }
        }

        EXPECT_EQ(cb_offset + bytesToBits(2), cr_offset);
        EXPECT_NE(nullptr, yCbCr_P010.yCbCr.y);
        EXPECT_NE(nullptr, yCbCr_P010.yCbCr.cb);
        EXPECT_NE(nullptr, yCbCr_P010.yCbCr.cr);
        return yCbCr_P010;
    }
};

class GraphicsMapperStableCTests
    : public GraphicsTestsBase,
      public ::testing::TestWithParam<std::tuple<std::string, std::shared_ptr<IAllocator>>> {
  public:
    void SetUp() override { Initialize(std::get<1>(GetParam())); }

    void TearDown() override {}
};

TEST_P(GraphicsMapperStableCTests, VersionChecks) {
    ASSERT_NE(nullptr, getHalVersion()) << "Resolving ANDROID_HAL_MAPPER_VERSION symbol failed";
    int32_t halVersion = *getHalVersion();
    EXPECT_EQ(halVersion, AIMAPPER_VERSION_5) << "Unrecognized ANDROID_HAL_MAPPER_VERSION";
    EXPECT_EQ(mapper()->version, AIMAPPER_VERSION_5) << "Unrecognized AIMapper::version";
    EXPECT_EQ(halVersion, mapper()->version)
            << "AIMapper version & ANDROID_HAL_MAPPER_VERSION don't agree";
}

TEST_P(GraphicsMapperStableCTests, AllV5CallbacksDefined) {
    ASSERT_GE(mapper()->version, AIMAPPER_VERSION_5);

    EXPECT_TRUE(mapper()->v5.importBuffer);
    EXPECT_TRUE(mapper()->v5.freeBuffer);
    EXPECT_TRUE(mapper()->v5.getTransportSize);
    EXPECT_TRUE(mapper()->v5.lock);
    EXPECT_TRUE(mapper()->v5.unlock);
    EXPECT_TRUE(mapper()->v5.flushLockedBuffer);
    EXPECT_TRUE(mapper()->v5.rereadLockedBuffer);
    EXPECT_TRUE(mapper()->v5.getMetadata);
    EXPECT_TRUE(mapper()->v5.getStandardMetadata);
    EXPECT_TRUE(mapper()->v5.setMetadata);
    EXPECT_TRUE(mapper()->v5.setStandardMetadata);
    EXPECT_TRUE(mapper()->v5.listSupportedMetadataTypes);
    EXPECT_TRUE(mapper()->v5.dumpBuffer);
    EXPECT_TRUE(mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, DualLoadIsIdentical) {
    ASSERT_GE(mapper()->version, AIMAPPER_VERSION_5);
    AIMapper* secondMapper;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, getIMapperLoader()(&secondMapper));

    EXPECT_EQ(secondMapper->v5.importBuffer, mapper()->v5.importBuffer);
    EXPECT_EQ(secondMapper->v5.freeBuffer, mapper()->v5.freeBuffer);
    EXPECT_EQ(secondMapper->v5.getTransportSize, mapper()->v5.getTransportSize);
    EXPECT_EQ(secondMapper->v5.lock, mapper()->v5.lock);
    EXPECT_EQ(secondMapper->v5.unlock, mapper()->v5.unlock);
    EXPECT_EQ(secondMapper->v5.flushLockedBuffer, mapper()->v5.flushLockedBuffer);
    EXPECT_EQ(secondMapper->v5.rereadLockedBuffer, mapper()->v5.rereadLockedBuffer);
    EXPECT_EQ(secondMapper->v5.getMetadata, mapper()->v5.getMetadata);
    EXPECT_EQ(secondMapper->v5.getStandardMetadata, mapper()->v5.getStandardMetadata);
    EXPECT_EQ(secondMapper->v5.setMetadata, mapper()->v5.setMetadata);
    EXPECT_EQ(secondMapper->v5.setStandardMetadata, mapper()->v5.setStandardMetadata);
    EXPECT_EQ(secondMapper->v5.listSupportedMetadataTypes, mapper()->v5.listSupportedMetadataTypes);
    EXPECT_EQ(secondMapper->v5.dumpBuffer, mapper()->v5.dumpBuffer);
    EXPECT_EQ(secondMapper->v5.getReservedRegion, mapper()->v5.getReservedRegion);
}

TEST_P(GraphicsMapperStableCTests, CanAllocate) {
    auto buffer = allocate({
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);
}

TEST_P(GraphicsMapperStableCTests, ImportFreeBuffer) {
    auto buffer = allocate({
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);

    {
        auto import1 = buffer->import();
        auto import2 = buffer->import();
        EXPECT_TRUE(import1);
        EXPECT_TRUE(import2);
        EXPECT_NE(*import1, *import2);
    }
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer cross mapper instances.
 */
TEST_P(GraphicsMapperStableCTests, ImportFreeBufferSingleton) {
    auto buffer = allocate({
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());
    EXPECT_GE(buffer->stride(), 64);

    buffer_handle_t bufferHandle = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.importBuffer(buffer->rawHandle(), &bufferHandle));
    ASSERT_NE(nullptr, bufferHandle);

    AIMapper* secondMapper;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, getIMapperLoader()(&secondMapper));
    ASSERT_EQ(AIMAPPER_ERROR_NONE, secondMapper->v5.freeBuffer(bufferHandle));
}

/**
 * Test IMapper::importBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperStableCTests, ImportBufferNegative) {
    native_handle_t* invalidHandle = nullptr;
    buffer_handle_t bufferHandle = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.importBuffer(invalidHandle, &bufferHandle))
            << "importBuffer with nullptr did not fail with BAD_BUFFER";

    invalidHandle = native_handle_create(0, 0);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.importBuffer(invalidHandle, &bufferHandle))
            << "importBuffer with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(invalidHandle);
}

/**
 * Test IMapper::freeBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperStableCTests, FreeBufferNegative) {
    native_handle_t* bufferHandle = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(bufferHandle))
            << "freeBuffer with nullptr did not fail with BAD_BUFFER";

    bufferHandle = native_handle_create(0, 0);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(bufferHandle))
            << "freeBuffer with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(bufferHandle);

    auto buffer = allocateGeneric();
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.freeBuffer(buffer->rawHandle()))
            << "freeBuffer with un-imported handle did not fail with BAD_BUFFER";
}

/**
 * Test IMapper::lock and IMapper::unlock.
 */
TEST_P(GraphicsMapperStableCTests, LockUnlockBasic) {
    constexpr auto usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN;
    auto buffer = allocate({
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = usage,
            .reservedSize = 0,
    });
    ASSERT_NE(nullptr, buffer.get());

    // lock buffer for writing
    const auto& info = buffer->info();
    const auto stride = buffer->stride();
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.lock(*handle, static_cast<int64_t>(usage), region, -1, (void**)&data));

    // RGBA_8888
    fillRGBA8888(data, info.height, stride * 4, info.width * 4);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));

    // lock again for reading
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(usage), region,
                                                     releaseFence, (void**)&data));
    releaseFence = -1;

    ASSERT_NO_FATAL_FAILURE(verifyRGBA8888(*handle, data, info.height, stride * 4, info.width * 4));

    releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

/**
 *  Test multiple operations associated with different color formats
 */
TEST_P(GraphicsMapperStableCTests, Lock_YCRCB_420_SP) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::YCRCB_420_SP,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    if (!buffer) {
        ASSERT_FALSE(isSupported(info));
        GTEST_SUCCEED() << "YCRCB_420_SP format is unsupported";
        return;
    }

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    auto cbData = static_cast<uint8_t*>(yCbCr.cb);
    auto crData = static_cast<uint8_t*>(yCbCr.cr);
    ASSERT_EQ(crData + 1, cbData);
    ASSERT_EQ(2, yCbCr.chroma_step);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));

    // lock again for reading
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, releaseFence, (void**)&data));
    releaseFence = -1;

    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, YV12SubsampleMetadata) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::YV12,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    ASSERT_NE(nullptr, buffer.get());

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(*handle);
    ASSERT_TRUE(decodeResult.has_value());
    const auto& planeLayouts = *decodeResult;

    ASSERT_EQ(3, planeLayouts.size());

    auto yPlane = planeLayouts[0];
    auto crPlane = planeLayouts[1];
    auto cbPlane = planeLayouts[2];

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    EXPECT_EQ(kCbCrSubSampleFactor, crPlane.horizontalSubsampling);
    EXPECT_EQ(kCbCrSubSampleFactor, crPlane.verticalSubsampling);

    EXPECT_EQ(kCbCrSubSampleFactor, cbPlane.horizontalSubsampling);
    EXPECT_EQ(kCbCrSubSampleFactor, cbPlane.verticalSubsampling);

    const long chromaSampleWidth = info.width / kCbCrSubSampleFactor;
    const long chromaSampleHeight = info.height / kCbCrSubSampleFactor;

    EXPECT_EQ(info.width, yPlane.widthInSamples);
    EXPECT_EQ(info.height, yPlane.heightInSamples);

    EXPECT_EQ(chromaSampleWidth, crPlane.widthInSamples);
    EXPECT_EQ(chromaSampleHeight, crPlane.heightInSamples);

    EXPECT_EQ(chromaSampleWidth, cbPlane.widthInSamples);
    EXPECT_EQ(chromaSampleHeight, cbPlane.heightInSamples);

    EXPECT_LE(crPlane.widthInSamples, crPlane.strideInBytes);
    EXPECT_LE(cbPlane.widthInSamples, cbPlane.strideInBytes);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, Lock_YV12) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::YV12,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    ASSERT_NE(nullptr, buffer.get());

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    auto cbData = static_cast<uint8_t*>(yCbCr.cb);
    auto crData = static_cast<uint8_t*>(yCbCr.cr);
    ASSERT_EQ(crData + yCbCr.cstride * info.height / vSubsampling, cbData);
    ASSERT_EQ(1, yCbCr.chroma_step);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));

    // lock again for reading
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, releaseFence, (void**)&data));
    releaseFence = -1;

    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, Lock_YCBCR_420_888) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::YCBCR_420_888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    ASSERT_NE(nullptr, buffer.get());

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));

    // lock again for reading
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, releaseFence, (void**)&data));
    releaseFence = -1;

    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(*handle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, Lock_RAW10) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RAW10,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    if (!buffer) {
        ASSERT_FALSE(isSupported(info));
        GTEST_SUCCEED() << "RAW10 format is unsupported";
        return;
    }

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(*handle);
    ASSERT_TRUE(decodeResult.has_value());
    const auto& planeLayouts = *decodeResult;

    ASSERT_EQ(1, planeLayouts.size());
    auto planeLayout = planeLayouts[0];

    EXPECT_EQ(0, planeLayout.sampleIncrementInBits);
    EXPECT_EQ(1, planeLayout.horizontalSubsampling);
    EXPECT_EQ(1, planeLayout.verticalSubsampling);

    ASSERT_EQ(1, planeLayout.components.size());
    auto planeLayoutComponent = planeLayout.components[0];

    EXPECT_EQ(PlaneLayoutComponentType::RAW,
              static_cast<PlaneLayoutComponentType>(planeLayoutComponent.type.value));
    EXPECT_EQ(0, planeLayoutComponent.offsetInBits % 8);
    EXPECT_EQ(-1, planeLayoutComponent.sizeInBits);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, Lock_RAW12) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RAW12,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    if (!buffer) {
        ASSERT_FALSE(isSupported(info));
        GTEST_SUCCEED() << "RAW12 format is unsupported";
        return;
    }

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    auto decodeResult = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(*handle);
    ASSERT_TRUE(decodeResult.has_value());
    const auto& planeLayouts = *decodeResult;

    ASSERT_EQ(1, planeLayouts.size());
    auto planeLayout = planeLayouts[0];

    EXPECT_EQ(0, planeLayout.sampleIncrementInBits);
    EXPECT_EQ(1, planeLayout.horizontalSubsampling);
    EXPECT_EQ(1, planeLayout.verticalSubsampling);

    ASSERT_EQ(1, planeLayout.components.size());
    auto planeLayoutComponent = planeLayout.components[0];

    EXPECT_EQ(PlaneLayoutComponentType::RAW,
              static_cast<PlaneLayoutComponentType>(planeLayoutComponent.type.value));
    EXPECT_EQ(0, planeLayoutComponent.offsetInBits % 8);
    EXPECT_EQ(-1, planeLayoutComponent.sizeInBits);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, Lock_YCBCR_P010) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::YCBCR_P010,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    if (!buffer) {
        ASSERT_FALSE(isSupported(info));
        GTEST_SUCCEED() << "YCBCR_P010 format is unsupported";
        return;
    }

    // lock buffer for writing
    const ARect region{0, 0, info.width, info.height};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                     region, -1, (void**)&data));

    YCbCr yCbCr;
    ASSERT_NO_FATAL_FAILURE(yCbCr = getAndroidYCbCr_P010(*handle, data));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, yCbCr.horizontalSubSampling);
    ASSERT_EQ(kCbCrSubSampleFactor, yCbCr.verticalSubSampling);

    ASSERT_EQ(0, info.height % 2);

    // fill the data
    fillYCbCrData(yCbCr.yCbCr, info.width, info.height, yCbCr.horizontalSubSampling,
                  yCbCr.verticalSubSampling);
    // verify the YCbCr data
    verifyYCbCrData(yCbCr.yCbCr, info.width, info.height, yCbCr.horizontalSubSampling,
                    yCbCr.verticalSubSampling);

    int releaseFence = -1;
    ASSERT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
    }
}

TEST_P(GraphicsMapperStableCTests, LockBadAccessRegion) {
    auto buffer = allocateGeneric();
    ASSERT_NE(nullptr, buffer);
    const auto& info = buffer->info();

    // lock buffer for writing
    const ARect region{0, 0, info.width * 2, info.height * 2};
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_VALUE, mapper()->v5.lock(*handle, static_cast<int64_t>(info.usage),
                                                          region, -1, (void**)&data));
}

TEST_P(GraphicsMapperStableCTests, UnlockNegative) {
    native_handle_t* invalidHandle = nullptr;
    int releaseFence = -1;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(invalidHandle, &releaseFence))
            << "unlock with nullptr did not fail with BAD_BUFFER";

    invalidHandle = native_handle_create(0, 0);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(invalidHandle, &releaseFence))
            << "unlock with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(invalidHandle);

    auto buffer = allocateGeneric();
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(buffer->rawHandle(), &releaseFence))
            << "unlock with un-imported handle did not fail with BAD_BUFFER";
}

TEST_P(GraphicsMapperStableCTests, UnlockNotImported) {
    int releaseFence = -1;
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(buffer->rawHandle(), &releaseFence))
            << "unlock with un-imported handle did not fail with BAD_BUFFER";
}

TEST_P(GraphicsMapperStableCTests, UnlockNotLocked) {
    int releaseFence = -1;
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(*bufferHandle, &releaseFence))
            << "unlock with unlocked handle did not fail with BAD_BUFFER";
}

TEST_P(GraphicsMapperStableCTests, LockUnlockNested) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    const ARect region{0, 0, buffer->info().width, buffer->info().height};
    auto usage = static_cast<int64_t>(buffer->info().usage);
    auto handle = buffer->import();
    uint8_t* data = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, usage, region, -1, (void**)&data));
    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.lock(*handle, usage, region, -1, (void**)&data))
            << "Second lock failed";
    int releaseFence = -1;
    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
        releaseFence = -1;
    }
    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*handle, &releaseFence))
            << "Second unlock failed";
    if (releaseFence != -1) {
        close(releaseFence);
        releaseFence = -1;
    }
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.unlock(*handle, &releaseFence))
            << "Third, unmatched, unlock should have failed with BAD_BUFFER";
}

TEST_P(GraphicsMapperStableCTests, FlushRereadBasic) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    const auto& info = buffer->info();
    const auto stride = buffer->stride();
    const ARect region{0, 0, buffer->info().width, buffer->info().height};

    auto writeHandle = buffer->import();
    auto readHandle = buffer->import();
    ASSERT_TRUE(writeHandle && readHandle);

    // lock buffer for writing

    uint8_t* writeData;
    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.lock(*writeHandle, static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN),
                                region, -1, (void**)&writeData));

    uint8_t* readData;
    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.lock(*readHandle, static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN),
                                region, -1, (void**)&readData));

    fillRGBA8888(writeData, info.height, stride * 4, info.width * 4);

    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.flushLockedBuffer(*writeHandle));
    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.rereadLockedBuffer(*readHandle));

    ASSERT_NO_FATAL_FAILURE(
            verifyRGBA8888(*readHandle, readData, info.height, stride * 4, info.width * 4));

    int releaseFence = -1;

    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*readHandle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
        releaseFence = -1;
    }

    EXPECT_EQ(AIMAPPER_ERROR_NONE, mapper()->v5.unlock(*writeHandle, &releaseFence));
    if (releaseFence != -1) {
        close(releaseFence);
        releaseFence = -1;
    }
}

TEST_P(GraphicsMapperStableCTests, FlushLockedBufferBadBuffer) {
    // Amazingly this is enough to make the compiler happy even though flushLockedBuffer
    // is _Nonnull :shrug:
    buffer_handle_t badBuffer = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.flushLockedBuffer(badBuffer));
}

TEST_P(GraphicsMapperStableCTests, RereadLockedBufferBadBuffer) {
    buffer_handle_t badBuffer = nullptr;
    EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, mapper()->v5.rereadLockedBuffer(badBuffer));
}

TEST_P(GraphicsMapperStableCTests, GetBufferId) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto bufferId = getStandardMetadata<StandardMetadataType::BUFFER_ID>(*bufferHandle);
    ASSERT_TRUE(bufferId.has_value());

    auto buffer2 = allocateGeneric();
    auto bufferHandle2 = buffer2->import();
    auto bufferId2 = getStandardMetadata<StandardMetadataType::BUFFER_ID>(*bufferHandle2);
    ASSERT_TRUE(bufferId2.has_value());

    EXPECT_NE(*bufferId, *bufferId2);
}

TEST_P(GraphicsMapperStableCTests, GetName) {
    auto buffer = allocate({
            .name = {"Hello, World!"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    auto bufferHandle = buffer->import();
    auto name = getStandardMetadata<StandardMetadataType::NAME>(*bufferHandle);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "Hello, World!");
}

TEST_P(GraphicsMapperStableCTests, GetWidthHeight) {
    auto buffer = allocate({
            .name = {"Hello, World!"},
            .width = 64,
            .height = 128,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::WIDTH>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 64);
    value = getStandardMetadata<StandardMetadataType::HEIGHT>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 128);
}

TEST_P(GraphicsMapperStableCTests, GetLayerCount) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::LAYER_COUNT>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, buffer->info().layerCount);
}

TEST_P(GraphicsMapperStableCTests, GetPixelFormatRequested) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::PIXEL_FORMAT_REQUESTED>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, buffer->info().format);
}

TEST_P(GraphicsMapperStableCTests, GetPixelFormatFourCC) {
    auto buffer = allocate({
            .name = {"Hello, World!"},
            .width = 64,
            .height = 128,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    {
        auto bufferHandle = buffer->import();
        auto value = getStandardMetadata<StandardMetadataType::PIXEL_FORMAT_FOURCC>(*bufferHandle);
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, DRM_FORMAT_ABGR8888);
    }

    buffer = allocate({
            .name = {"yv12"},
            .width = 64,
            .height = 128,
            .layerCount = 1,
            .format = PixelFormat::YV12,
            .usage = BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN,
            .reservedSize = 0,
    });
    {
        auto bufferHandle = buffer->import();
        auto value = getStandardMetadata<StandardMetadataType::PIXEL_FORMAT_FOURCC>(*bufferHandle);
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, DRM_FORMAT_YVU420);
    }
}

TEST_P(GraphicsMapperStableCTests, GetPixelFormatModifier) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::PIXEL_FORMAT_MODIFIER>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    // Only the upper 8-bits are defined and is just the vendor ID, the lower 56 bits are
    // then vendor specific. So there's not anything useful to assert here beyond just that
    // we successfully queried a value
}

TEST_P(GraphicsMapperStableCTests, GetUsage) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::USAGE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(buffer->info().usage, *value);
}

TEST_P(GraphicsMapperStableCTests, GetUsage64) {
    BufferDescriptorInfo info{
            .name = {"VTS_TEMP"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::FRONT_BUFFER | BufferUsage::GPU_RENDER_TARGET |
                     BufferUsage::COMPOSER_OVERLAY | BufferUsage::GPU_TEXTURE,
            .reservedSize = 0,
    };
    if (!isSupported(info)) {
        GTEST_SKIP();
    }
    auto buffer = allocate(info);
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::USAGE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    using T = std::underlying_type_t<BufferUsage>;
    EXPECT_EQ(static_cast<T>(buffer->info().usage), static_cast<T>(*value));
}

TEST_P(GraphicsMapperStableCTests, GetAllocationSize) {
    auto buffer = allocateGeneric();
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::ALLOCATION_SIZE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    const auto estimatedSize = buffer->stride() * buffer->info().height * 4;
    // This buffer has CPU usage, so we expect at least stride * height * 4 since it should be
    // generally linear uncompressed.
    EXPECT_GE(*value, estimatedSize)
            << "Expected allocation size to be at least stride * height * 4bpp";
    // Might need refining, but hopefully this a generous-enough upper-bound?
    EXPECT_LT(*value, estimatedSize * 2)
            << "Expected allocation size to less than double stride * height * 4bpp";
}

TEST_P(GraphicsMapperStableCTests, GetProtectedContent) {
    const BufferDescriptorInfo info{
            .name = {"prot8888"},
            .width = 64,
            .height = 64,
            .layerCount = 1,
            .format = PixelFormat::RGBA_8888,
            .usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY,
            .reservedSize = 0,
    };
    auto buffer = allocate(info);
    if (!buffer) {
        ASSERT_FALSE(isSupported(info))
                << "Allocation of trivial sized buffer failed, so isSupported() must be false";
        GTEST_SUCCEED() << "PROTECTED RGBA_8888 is unsupported";
        return;
    }
    auto bufferHandle = buffer->import();
    auto value = getStandardMetadata<StandardMetadataType::PROTECTED_CONTENT>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 1);
}

TEST_P(GraphicsMapperStableCTests, GetCompression) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::COMPRESSION>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(gralloc4::Compression_None.name, value->name);
    EXPECT_EQ(gralloc4::Compression_None.value, value->value);
}

TEST_P(GraphicsMapperStableCTests, GetInterlaced) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::INTERLACED>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(gralloc4::Interlaced_None.name, value->name);
    EXPECT_EQ(gralloc4::Interlaced_None.value, value->value);
}

TEST_P(GraphicsMapperStableCTests, GetChromaSiting) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::CHROMA_SITING>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(gralloc4::ChromaSiting_None.name, value->name);
    EXPECT_EQ(gralloc4::ChromaSiting_None.value, value->value);
}

TEST_P(GraphicsMapperStableCTests, GetPlaneLayouts) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::PLANE_LAYOUTS>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    ASSERT_NO_FATAL_FAILURE(verifyRGBA8888PlaneLayouts(*value));
}

TEST_P(GraphicsMapperStableCTests, GetCrop) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::CROP>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(1, value->size());
    const Rect expected{0, 0, buffer->info().width, buffer->info().height};
    EXPECT_EQ(expected, value->at(0));
}

TEST_P(GraphicsMapperStableCTests, GetSetDataspace) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::DATASPACE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Dataspace::UNKNOWN, *value);
    EXPECT_EQ(AIMAPPER_ERROR_NONE, setStandardMetadata<StandardMetadataType::DATASPACE>(
                                           *bufferHandle, Dataspace::DISPLAY_P3));
    value = getStandardMetadata<StandardMetadataType::DATASPACE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(Dataspace::DISPLAY_P3, *value);
}

TEST_P(GraphicsMapperStableCTests, GetSetBlendMode) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::BLEND_MODE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(BlendMode::INVALID, *value);
    EXPECT_EQ(AIMAPPER_ERROR_NONE, setStandardMetadata<StandardMetadataType::BLEND_MODE>(
                                           *bufferHandle, BlendMode::COVERAGE));
    value = getStandardMetadata<StandardMetadataType::BLEND_MODE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(BlendMode::COVERAGE, *value);
}

TEST_P(GraphicsMapperStableCTests, GetSetSmpte2086) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::SMPTE2086>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(value->has_value());

    // TODO: Maybe use something resembling real values, but validation isn't supposed to happen
    // here anyway so :shrug:
    const Smpte2086 awesomeHdr{
            XyColor{1.f, 1.f},      XyColor{2.f, 2.f}, XyColor{3.f, 3.f},
            XyColor{400.f, 1000.f}, 100000.0f,         0.0001f,
    };
    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              setStandardMetadata<StandardMetadataType::SMPTE2086>(*bufferHandle, awesomeHdr));
    value = getStandardMetadata<StandardMetadataType::SMPTE2086>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    ASSERT_TRUE(value->has_value());
    EXPECT_EQ(awesomeHdr, *value);

    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              setStandardMetadata<StandardMetadataType::SMPTE2086>(*bufferHandle, std::nullopt));
    value = getStandardMetadata<StandardMetadataType::SMPTE2086>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(value->has_value());
}

TEST_P(GraphicsMapperStableCTests, GetCta861_3) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::CTA861_3>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(value->has_value());

    const Cta861_3 genericHlgish{1000.f, 140.f};
    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              setStandardMetadata<StandardMetadataType::CTA861_3>(*bufferHandle, genericHlgish));
    value = getStandardMetadata<StandardMetadataType::CTA861_3>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    ASSERT_TRUE(value->has_value());
    EXPECT_EQ(genericHlgish, *value);

    EXPECT_EQ(AIMAPPER_ERROR_NONE,
              setStandardMetadata<StandardMetadataType::CTA861_3>(*bufferHandle, std::nullopt));
    value = getStandardMetadata<StandardMetadataType::CTA861_3>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_FALSE(value->has_value());
}

TEST_P(GraphicsMapperStableCTests, GetSmpte2094_10) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::SMPTE2094_10>(*bufferHandle);
    if (value.has_value()) {
        EXPECT_FALSE(value->has_value());
    }
}

TEST_P(GraphicsMapperStableCTests, GetSmpte2094_40) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::SMPTE2094_40>(*bufferHandle);
    if (value.has_value()) {
        EXPECT_FALSE(value->has_value());
    }
}

TEST_P(GraphicsMapperStableCTests, GetStride) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    auto value = getStandardMetadata<StandardMetadataType::STRIDE>(*bufferHandle);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(buffer->stride(), *value);
}

TEST_P(GraphicsMapperStableCTests, SupportsRequiredGettersSetters) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    const AIMapper_MetadataTypeDescription* descriptions = nullptr;
    size_t descriptionCount = 0;
    ASSERT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.listSupportedMetadataTypes(&descriptions, &descriptionCount));
    std::vector<StandardMetadataType> requiredGetters = {
            StandardMetadataType::BUFFER_ID,
            StandardMetadataType::NAME,
            StandardMetadataType::WIDTH,
            StandardMetadataType::HEIGHT,
            StandardMetadataType::LAYER_COUNT,
            StandardMetadataType::PIXEL_FORMAT_REQUESTED,
            StandardMetadataType::PIXEL_FORMAT_FOURCC,
            StandardMetadataType::PIXEL_FORMAT_MODIFIER,
            StandardMetadataType::USAGE,
            StandardMetadataType::ALLOCATION_SIZE,
            StandardMetadataType::PROTECTED_CONTENT,
            StandardMetadataType::COMPRESSION,
            StandardMetadataType::INTERLACED,
            StandardMetadataType::CHROMA_SITING,
            StandardMetadataType::PLANE_LAYOUTS,
            StandardMetadataType::CROP,
            StandardMetadataType::DATASPACE,
            StandardMetadataType::BLEND_MODE,
            StandardMetadataType::SMPTE2086,
            StandardMetadataType::CTA861_3,
            StandardMetadataType::STRIDE,
    };

    std::vector<StandardMetadataType> requiredSetters = {
            StandardMetadataType::DATASPACE,
            StandardMetadataType::BLEND_MODE,
            StandardMetadataType::SMPTE2086,
            StandardMetadataType::CTA861_3,
    };

    for (int i = 0; i < descriptionCount; i++) {
        const auto& it = descriptions[i];
        if (isStandardMetadata(it.metadataType)) {
            EXPECT_GT(it.metadataType.value, static_cast<int64_t>(StandardMetadataType::INVALID));
            EXPECT_LT(it.metadataType.value,
                      ndk::internal::enum_values<StandardMetadataType>.size());

            if (it.isGettable) {
                std::erase(requiredGetters,
                           static_cast<StandardMetadataType>(it.metadataType.value));
            }
            if (it.isSettable) {
                std::erase(requiredSetters,
                           static_cast<StandardMetadataType>(it.metadataType.value));
            }
        } else {
            EXPECT_NE(nullptr, it.description) << "Non-standard metadata must have a description";
            int len = strlen(it.description);
            EXPECT_GE(len, 0) << "Non-standard metadata must have a description";
        }
    }

    EXPECT_EQ(0, requiredGetters.size()) << "Missing required getters" << toString(requiredGetters);
    EXPECT_EQ(0, requiredSetters.size()) << "Missing required setters" << toString(requiredSetters);
}

/*
 * Test that verifies that if the optional StandardMetadataTypes have getters, they have
 * the required setters as well
 */
TEST_P(GraphicsMapperStableCTests, CheckRequiredSettersIfHasGetters) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    const AIMapper_MetadataTypeDescription* descriptions = nullptr;
    size_t descriptionCount = 0;
    ASSERT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.listSupportedMetadataTypes(&descriptions, &descriptionCount));

    for (int i = 0; i < descriptionCount; i++) {
        const auto& it = descriptions[i];
        if (isStandardMetadata(it.metadataType)) {
            const auto type = static_cast<StandardMetadataType>(it.metadataType.value);
            switch (type) {
                case StandardMetadataType::SMPTE2094_10:
                case StandardMetadataType::SMPTE2094_40:
                    if (it.isGettable) {
                        EXPECT_TRUE(it.isSettable)
                                << "Type " << toString(type) << " must be settable if gettable";
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

TEST_P(GraphicsMapperStableCTests, ListSupportedWorks) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);
    const AIMapper_MetadataTypeDescription* descriptions = nullptr;
    size_t descriptionCount = 0;
    ASSERT_EQ(AIMAPPER_ERROR_NONE,
              mapper()->v5.listSupportedMetadataTypes(&descriptions, &descriptionCount));

    std::vector<uint8_t> metadataBuffer;
    auto get = [&](AIMapper_MetadataType metadataType) -> int32_t {
        int32_t size = mapper()->v5.getMetadata(*bufferHandle, metadataType, nullptr, 0);
        if (size >= 0) {
            metadataBuffer.resize(size);
            size = mapper()->v5.getMetadata(*bufferHandle, metadataType, metadataBuffer.data(),
                                            metadataBuffer.size());
            EXPECT_EQ(size, metadataBuffer.size());
        }
        return size;
    };

    for (int i = 0; i < descriptionCount; i++) {
        const auto& it = descriptions[i];
        if (!isStandardMetadata(it.metadataType)) {
            continue;
        }
        if (!it.isGettable) {
            EXPECT_FALSE(it.isSettable)
                    << "StandardMetadata that isn't gettable must not be settable";
            continue;
        }
        EXPECT_GE(get(it.metadataType), 0)
                << "Get failed for claimed supported getter of "
                << toString(static_cast<StandardMetadataType>(it.metadataType.value));
        if (it.isSettable) {
            EXPECT_EQ(AIMAPPER_ERROR_NONE,
                      mapper()->v5.setMetadata(*bufferHandle, it.metadataType,
                                               metadataBuffer.data(), metadataBuffer.size()))
                    << "Failed to set metadata for "
                    << toString(static_cast<StandardMetadataType>(it.metadataType.value));
        }
    }
}

TEST_P(GraphicsMapperStableCTests, GetMetadataBadValue) {
    auto get = [this](StandardMetadataType type) -> AIMapper_Error {
        // This is a _Nonnull parameter, but this is enough obfuscation to fool the linter
        buffer_handle_t buffer = nullptr;
        int32_t ret =
                mapper()->v5.getStandardMetadata(buffer, static_cast<int64_t>(type), nullptr, 0);
        return (ret < 0) ? (AIMapper_Error)-ret : AIMAPPER_ERROR_NONE;
    };

    for (auto type : ndk::enum_range<StandardMetadataType>()) {
        if (type == StandardMetadataType::INVALID) {
            continue;
        }
        EXPECT_EQ(AIMAPPER_ERROR_BAD_BUFFER, get(type)) << "Wrong error for " << toString(type);
    }
}

TEST_P(GraphicsMapperStableCTests, GetUnsupportedMetadata) {
    auto buffer = allocateGeneric();
    ASSERT_TRUE(buffer);
    auto bufferHandle = buffer->import();
    ASSERT_TRUE(bufferHandle);

    int result = mapper()->v5.getMetadata(*bufferHandle, {"Fake", 1}, nullptr, 0);
    EXPECT_EQ(AIMAPPER_ERROR_UNSUPPORTED, -result);

    result = mapper()->v5.getStandardMetadata(
            *bufferHandle, static_cast<int64_t>(StandardMetadataType::INVALID), nullptr, 0);
    EXPECT_EQ(AIMAPPER_ERROR_UNSUPPORTED, -result);

    constexpr int64_t unknownStandardType = ndk::internal::enum_values<StandardMetadataType>.size();
    result = mapper()->v5.getStandardMetadata(*bufferHandle, unknownStandardType, nullptr, 0);
    EXPECT_EQ(AIMAPPER_ERROR_UNSUPPORTED, -result);
}

std::vector<std::tuple<std::string, std::shared_ptr<IAllocator>>> getIAllocatorsAtLeastVersion(
        int32_t minVersion) {
    auto instanceNames = getAidlHalInstanceNames(IAllocator::descriptor);
    std::vector<std::tuple<std::string, std::shared_ptr<IAllocator>>> filteredInstances;
    filteredInstances.reserve(instanceNames.size());
    for (const auto& name : instanceNames) {
        auto allocator =
                IAllocator::fromBinder(ndk::SpAIBinder(AServiceManager_checkService(name.c_str())));
        int32_t version = 0;
        if (allocator->getInterfaceVersion(&version).isOk()) {
            if (version >= minVersion) {
                filteredInstances.emplace_back(name, std::move(allocator));
            }
        }
    }
    return filteredInstances;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsMapperStableCTests);
INSTANTIATE_TEST_CASE_P(PerInstance, GraphicsMapperStableCTests,
                        testing::ValuesIn(getIAllocatorsAtLeastVersion(2)),
                        [](auto info) -> std::string {
                            std::string name =
                                    std::to_string(info.index) + "/" + std::get<0>(info.param);
                            return Sanitize(name);
                        });