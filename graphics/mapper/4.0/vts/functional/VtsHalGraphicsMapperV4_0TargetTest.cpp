/*
 * Copyright 2019 The Android Open Source Project
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

#define LOG_TAG "VtsHalGraphicsMapperV4_0TargetTest"

#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>

#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidl/android/hardware/graphics/allocator/AllocationResult.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
#include <aidlcommonsupport/NativeHandle.h>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/unique_fd.h>
#include <android/sync.h>
#include <gralloctypes/Gralloc4.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <mapper-vts/4.0/MapperVts.h>
#include <system/graphics.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
namespace vts {
namespace {

using ::android::base::unique_fd;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using Tolerance = ::android::hardware::graphics::mapper::V4_0::vts::Gralloc::Tolerance;
using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;
using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::Cta861_3;
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::ExtendableType;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using aidl::android::hardware::graphics::common::Smpte2086;
using aidl::android::hardware::graphics::common::StandardMetadataType;

using DecodeFunction = std::function<void(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                          const hidl_vec<uint8_t>& vec)>;

struct YCbCr {
    android_ycbcr yCbCr;
    int64_t horizontalSubSampling;
    int64_t verticalSubSampling;
};

class GraphicsMapperHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(mGralloc = std::make_unique<Gralloc>(std::get<0>(GetParam()),
                                                                     std::get<1>(GetParam())));
        ASSERT_TRUE(mGralloc->hasAllocator());
        ASSERT_NE(nullptr, mGralloc->getMapper().get());

        mDummyDescriptorInfo.name = "dummy";
        mDummyDescriptorInfo.width = 64;
        mDummyDescriptorInfo.height = 64;
        mDummyDescriptorInfo.layerCount = 1;
        mDummyDescriptorInfo.format = PixelFormat::RGBA_8888;
        mDummyDescriptorInfo.usage =
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);
        mDummyDescriptorInfo.reservedSize = 0;
    }

    void TearDown() override {}

    void testGet(const IMapper::BufferDescriptorInfo& descriptorInfo,
                 const MetadataType& metadataType, DecodeFunction decode) {
        const native_handle_t* bufferHandle = nullptr;
        ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(descriptorInfo, true));

        hidl_vec<uint8_t> vec;
        const auto result = mGralloc->get(bufferHandle, metadataType, &vec);

        if (metadataType == gralloc4::MetadataType_Smpte2094_10 && result == Error::UNSUPPORTED) {
            GTEST_SKIP() << "getting metadata for Smpte2094-10 is unsupported";
        }

        ASSERT_EQ(Error::NONE, result);

        ASSERT_NO_FATAL_FAILURE(decode(descriptorInfo, vec));
    }

    void testSet(const IMapper::BufferDescriptorInfo& descriptorInfo,
                 const MetadataType& metadataType, const hidl_vec<uint8_t>& metadata,
                 DecodeFunction decode) {
        const native_handle_t* bufferHandle = nullptr;
        ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(descriptorInfo, true));

        Error err = mGralloc->set(bufferHandle, metadataType, metadata);
        if (err == Error::UNSUPPORTED) {
            GTEST_SUCCEED() << "setting this metadata is unsupported";
            return;
        }
        ASSERT_EQ(err, Error::NONE);

        hidl_vec<uint8_t> vec;
        ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, metadataType, &vec));

        ASSERT_NO_FATAL_FAILURE(decode(descriptorInfo, vec));
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
        EXPECT_EQ(mDummyDescriptorInfo.width, planeLayout.widthInSamples);
        EXPECT_EQ(mDummyDescriptorInfo.height, planeLayout.heightInSamples);
        EXPECT_LE(planeLayout.widthInSamples * planeLayout.heightInSamples * 4,
                  planeLayout.totalSizeInBytes);
        EXPECT_EQ(1, planeLayout.horizontalSubsampling);
        EXPECT_EQ(1, planeLayout.verticalSubsampling);
    }

    void verifyBufferDump(const IMapper::BufferDump& bufferDump,
                          const native_handle_t* bufferHandle = nullptr) {
        std::set<StandardMetadataType> foundMetadataTypes;

        const std::vector<IMapper::MetadataDump> metadataDump = bufferDump.metadataDump;

        for (const auto& dump : metadataDump) {
            const auto& metadataType = dump.metadataType;
            const auto& metadata = dump.metadata;

            if (!gralloc4::isStandardMetadataType(metadataType)) {
                continue;
            }

            StandardMetadataType type = gralloc4::getStandardMetadataTypeValue(metadataType);

            if (sRequiredMetadataTypes.find(type) == sRequiredMetadataTypes.end()) {
                continue;
            }

            ASSERT_EQ(foundMetadataTypes.find(type), foundMetadataTypes.end());
            foundMetadataTypes.insert(type);

            if (!bufferHandle) {
                continue;
            }

            hidl_vec<uint8_t> metadataFromGet;
            ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, metadataType, &metadataFromGet));

            ASSERT_EQ(metadataFromGet, metadata);
        }

        EXPECT_EQ(sRequiredMetadataTypes, foundMetadataTypes);
    }

    void getAndroidYCbCr(const native_handle_t* bufferHandle, uint8_t* data,
                         android_ycbcr* outYCbCr, int64_t* hSubsampling, int64_t* vSubsampling) {
        hidl_vec<uint8_t> vec;
        ASSERT_EQ(Error::NONE,
                  mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
        std::vector<PlaneLayout> planeLayouts;
        ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

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
        hidl_vec<uint8_t> vec;
        EXPECT_EQ(Error::NONE,
                  mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
        std::vector<PlaneLayout> planeLayouts;
        EXPECT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));
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

    void fillRGBA8888(uint8_t* data, uint32_t height, size_t strideInBytes, size_t widthInBytes,
                      uint32_t seed = 0) {
        for (uint32_t y = 0; y < height; y++) {
            memset(data, y + seed, widthInBytes);
            data += strideInBytes;
        }
    }

    void verifyRGBA8888(const native_handle_t* bufferHandle, const uint8_t* data, uint32_t height,
                        size_t strideInBytes, size_t widthInBytes, uint32_t seed = 0) {
        hidl_vec<uint8_t> vec;
        ASSERT_EQ(Error::NONE,
                  mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
        std::vector<PlaneLayout> planeLayouts;
        ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

        verifyRGBA8888PlaneLayouts(planeLayouts);

        for (uint32_t y = 0; y < height; y++) {
            for (size_t i = 0; i < widthInBytes; i++) {
                EXPECT_EQ(static_cast<uint8_t>(y + seed), data[i]);
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

    bool isEqual(float a, float b) { return abs(a - b) < 0.0001; }

    uint64_t bitsToBytes(int64_t bits) { return bits / 8; }

    uint64_t bytesToBits(int64_t bytes) { return bytes * 8; }

    std::unique_ptr<Gralloc> mGralloc;
    IMapper::BufferDescriptorInfo mDummyDescriptorInfo{};
    static const std::set<StandardMetadataType> sRequiredMetadataTypes;
};

const std::set<StandardMetadataType> GraphicsMapperHidlTest::sRequiredMetadataTypes{
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
        StandardMetadataType::DATASPACE,
        StandardMetadataType::BLEND_MODE,
};

/**
 * Test IAllocator::allocate with valid buffer descriptors.
 */
TEST_P(GraphicsMapperHidlTest, AllocatorAllocate) {
    BufferDescriptor descriptor;
    ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(mDummyDescriptorInfo));

    for (uint32_t count = 0; count < 5; count++) {
        std::vector<const native_handle_t*> bufferHandles;
        uint32_t stride;
        ASSERT_NO_FATAL_FAILURE(bufferHandles =
                                        mGralloc->allocate(descriptor, count, false,
                                                           Tolerance::kToleranceStrict, &stride));

        if (count >= 1) {
            EXPECT_LE(mDummyDescriptorInfo.width, stride) << "invalid buffer stride";
        }

        for (auto bufferHandle : bufferHandles) {
            mGralloc->freeBuffer(bufferHandle);
        }
    }
}

/**
 * Test IAllocator::allocate with invalid buffer descriptors.
 */
TEST_P(GraphicsMapperHidlTest, AllocatorAllocateNegative) {
    // this assumes any valid descriptor is non-empty
    BufferDescriptor descriptor;

    mGralloc->rawAllocate(descriptor, 1, [&](const auto& tmpError, const auto&, const auto&) {
        EXPECT_EQ(Error::BAD_DESCRIPTOR, tmpError);
    });
}

/**
 * Test IAllocator::allocate does not leak.
 */
TEST_P(GraphicsMapperHidlTest, AllocatorAllocateNoLeak) {
    auto info = mDummyDescriptorInfo;
    info.width = 1024;
    info.height = 1024;

    for (int i = 0; i < 2048; i++) {
        auto bufferHandle = mGralloc->allocate(info, false);
        mGralloc->freeBuffer(bufferHandle);
    }
}

/**
 * Test that IAllocator::allocate is thread-safe.
 */
TEST_P(GraphicsMapperHidlTest, AllocatorAllocateThreaded) {
    BufferDescriptor descriptor;
    ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(mDummyDescriptorInfo));

    std::atomic<bool> timeUp(false);
    std::atomic<uint64_t> allocationCount(0);
    auto threadLoop = [&]() {
        while (!timeUp) {
            mGralloc->rawAllocate(descriptor, 1, [&](const auto&, const auto&, const auto&) {
                allocationCount++;
            });
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; i++) {
        threads.push_back(std::thread(threadLoop));
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    timeUp = true;
    LOG(VERBOSE) << "Made " << allocationCount << " threaded allocations";

    for (auto& thread : threads) {
        thread.join();
    }
}

/**
 * Test IMapper::createDescriptor with valid descriptor info.
 */
TEST_P(GraphicsMapperHidlTest, CreateDescriptorBasic) {
    ASSERT_NO_FATAL_FAILURE(mGralloc->createDescriptor(mDummyDescriptorInfo));
}

/**
 * Test IMapper::createDescriptor with invalid descriptor info.
 */
TEST_P(GraphicsMapperHidlTest, CreateDescriptorNegative) {
    auto info = mDummyDescriptorInfo;
    info.width = 0;
    mGralloc->getMapper()->createDescriptor(info, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_VALUE, tmpError) << "createDescriptor did not fail with BAD_VALUE";
    });
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer with allocated buffers.
 */
TEST_P(GraphicsMapperHidlTest, ImportFreeBufferBasic) {
    const native_handle_t* bufferHandle;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));
    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(bufferHandle));
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer with cloned buffers.
 */
TEST_P(GraphicsMapperHidlTest, ImportFreeBufferClone) {
    const native_handle_t* clonedBufferHandle;
    ASSERT_NO_FATAL_FAILURE(clonedBufferHandle = mGralloc->allocate(mDummyDescriptorInfo, false));

    // A cloned handle is a raw handle. Check that we can import it multiple
    // times.
    const native_handle_t* importedBufferHandles[2];
    ASSERT_NO_FATAL_FAILURE(importedBufferHandles[0] = mGralloc->importBuffer(clonedBufferHandle));
    ASSERT_NO_FATAL_FAILURE(importedBufferHandles[1] = mGralloc->importBuffer(clonedBufferHandle));
    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(importedBufferHandles[0]));
    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(importedBufferHandles[1]));

    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(clonedBufferHandle));
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer cross mapper instances.
 */
TEST_P(GraphicsMapperHidlTest, ImportFreeBufferSingleton) {
    const native_handle_t* rawHandle;
    ASSERT_NO_FATAL_FAILURE(rawHandle = mGralloc->allocate(mDummyDescriptorInfo, false));

    native_handle_t* importedHandle = nullptr;
    mGralloc->getMapper()->importBuffer(rawHandle, [&](const auto& tmpError, const auto& buffer) {
        ASSERT_EQ(Error::NONE, tmpError);
        importedHandle = static_cast<native_handle_t*>(buffer);
    });

    // free the imported handle with another mapper
    std::unique_ptr<Gralloc> anotherGralloc;
    ASSERT_NO_FATAL_FAILURE(anotherGralloc = std::make_unique<Gralloc>(std::get<0>(GetParam()),
                                                                       std::get<1>(GetParam())));
    Error error = mGralloc->getMapper()->freeBuffer(importedHandle);
    ASSERT_EQ(Error::NONE, error);

    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(rawHandle));
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer do not leak.
 */
TEST_P(GraphicsMapperHidlTest, ImportFreeBufferNoLeak) {
    auto info = mDummyDescriptorInfo;
    info.width = 1024;
    info.height = 1024;

    for (int i = 0; i < 2048; i++) {
        auto bufferHandle = mGralloc->allocate(info, true);
        mGralloc->freeBuffer(bufferHandle);
    }
}

/**
 * Test IMapper::importBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperHidlTest, ImportBufferNegative) {
    native_handle_t* invalidHandle = nullptr;
    mGralloc->getMapper()->importBuffer(invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
                << "importBuffer with nullptr did not fail with BAD_BUFFER";
    });

    invalidHandle = native_handle_create(0, 0);
    mGralloc->getMapper()->importBuffer(invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
                << "importBuffer with invalid handle did not fail with BAD_BUFFER";
    });
    native_handle_delete(invalidHandle);
}

/**
 * Test IMapper::freeBuffer with invalid buffers.
 */
TEST_P(GraphicsMapperHidlTest, FreeBufferNegative) {
    native_handle_t* invalidHandle = nullptr;
    Error error = mGralloc->getMapper()->freeBuffer(invalidHandle);
    EXPECT_EQ(Error::BAD_BUFFER, error) << "freeBuffer with nullptr did not fail with BAD_BUFFER";

    invalidHandle = native_handle_create(0, 0);
    error = mGralloc->getMapper()->freeBuffer(invalidHandle);
    EXPECT_EQ(Error::BAD_BUFFER, error)
            << "freeBuffer with invalid handle did not fail with BAD_BUFFER";
    native_handle_delete(invalidHandle);

    const native_handle_t* clonedBufferHandle;
    ASSERT_NO_FATAL_FAILURE(clonedBufferHandle = mGralloc->allocate(mDummyDescriptorInfo, false));
    error = mGralloc->getMapper()->freeBuffer(invalidHandle);
    EXPECT_EQ(Error::BAD_BUFFER, error)
            << "freeBuffer with un-imported handle did not fail with BAD_BUFFER";

    mGralloc->freeBuffer(clonedBufferHandle);
}

/**
 * Test IMapper::lock and IMapper::unlock.
 */
TEST_P(GraphicsMapperHidlTest, LockUnlockBasic) {
    const auto& info = mDummyDescriptorInfo;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(
            bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceStrict, &stride));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    uint8_t* data;
    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    // RGBA_8888
    fillRGBA8888(data, info.height, stride * 4, info.width * 4);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    ASSERT_NO_FATAL_FAILURE(
            verifyRGBA8888(bufferHandle, data, info.height, stride * 4, info.width * 4));

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

/**
 *  Test multiple operations associated with different color formats
 */
TEST_P(GraphicsMapperHidlTest, Lock_YCRCB_420_SP) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YCRCB_420_SP;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(
                                    info, true, Tolerance::kToleranceUnSupported, &stride));
    if (bufferHandle == nullptr) {
        GTEST_SUCCEED() << "YCRCB_420_SP format is unsupported";
        return;
    }

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    uint8_t* data;

    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    auto cbData = static_cast<uint8_t*>(yCbCr.cb);
    auto crData = static_cast<uint8_t*>(yCbCr.cr);
    ASSERT_EQ(crData + 1, cbData);
    ASSERT_EQ(2, yCbCr.chroma_step);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, YV12SubsampleMetadata) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YV12;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(
            bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceStrict, &stride));

    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    ASSERT_NO_FATAL_FAILURE(mGralloc->lock(bufferHandle, info.usage, region, fence.release()));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

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

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, Lock_YV12) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YV12;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(
            bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceStrict, &stride));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    uint8_t* data;

    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    auto cbData = static_cast<uint8_t*>(yCbCr.cb);
    auto crData = static_cast<uint8_t*>(yCbCr.cr);
    ASSERT_EQ(crData + yCbCr.cstride * info.height / vSubsampling, cbData);
    ASSERT_EQ(1, yCbCr.chroma_step);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, Lock_YCBCR_420_888) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YCBCR_420_888;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(
            bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceStrict, &stride));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    uint8_t* data;

    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    android_ycbcr yCbCr;
    int64_t hSubsampling = 0;
    int64_t vSubsampling = 0;
    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    constexpr uint32_t kCbCrSubSampleFactor = 2;
    ASSERT_EQ(kCbCrSubSampleFactor, hSubsampling);
    ASSERT_EQ(kCbCrSubSampleFactor, vSubsampling);

    fillYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    ASSERT_NO_FATAL_FAILURE(
            getAndroidYCbCr(bufferHandle, data, &yCbCr, &hSubsampling, &vSubsampling));

    verifyYCbCrData(yCbCr, info.width, info.height, hSubsampling, vSubsampling);

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, Lock_RAW10) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::RAW10;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(
                                    info, true, Tolerance::kToleranceUnSupported, &stride));
    if (bufferHandle == nullptr) {
        GTEST_SUCCEED() << "RAW10 format is unsupported";
        return;
    }

    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;

    ASSERT_NO_FATAL_FAILURE(mGralloc->lock(bufferHandle, info.usage, region, fence.release()));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

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

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, Lock_RAW12) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::RAW12;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(
                                    info, true, Tolerance::kToleranceUnSupported, &stride));
    if (bufferHandle == nullptr) {
        GTEST_SUCCEED() << "RAW12 format is unsupported";
        return;
    }

    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;

    ASSERT_NO_FATAL_FAILURE(mGralloc->lock(bufferHandle, info.usage, region, fence.release()));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

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

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

TEST_P(GraphicsMapperHidlTest, Lock_YCBCR_P010) {
    if (base::GetIntProperty("ro.vendor.api_level", __ANDROID_API_FUTURE__) < __ANDROID_API_T__) {
        GTEST_SKIP() << "Old vendor grallocs may not support P010";
    }
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YCBCR_P010;

    uint32_t stride;
    const native_handle_t* bufferHandle =
            mGralloc->allocate(info, true, Tolerance::kToleranceStrict, &stride);

    if (::testing::Test::IsSkipped()) {
        GTEST_SKIP();
    }

    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    unique_fd fence;
    uint8_t* data;

    ASSERT_NO_FATAL_FAILURE(data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage,
                                                                        region, fence.release())));

    YCbCr yCbCr;
    ASSERT_NO_FATAL_FAILURE(yCbCr = getAndroidYCbCr_P010(bufferHandle, data));

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

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(bufferHandle)));
}

/**
 * Test IMapper::unlock with bad access region
 */
TEST_P(GraphicsMapperHidlTest, LockBadAccessRegion) {
    const auto& info = mDummyDescriptorInfo;

    const native_handle_t* bufferHandle;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true));

    const IMapper::Rect accessRegion{0, 0, static_cast<int32_t>(info.width * 2),
                                     static_cast<int32_t>(info.height * 2)};
    int acquireFence = -1;

    NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
    hidl_handle acquireFenceHandle;
    if (acquireFence >= 0) {
        auto h = native_handle_init(acquireFenceStorage, 1, 0);
        h->data[0] = acquireFence;
        acquireFenceHandle = h;
    }

    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    mGralloc->getMapper()->lock(buffer, info.usage, accessRegion, acquireFenceHandle,
                                [&](const auto& tmpError, const auto& /*tmpData*/) {
                                    EXPECT_EQ(Error::BAD_VALUE, tmpError)
                                            << "locking with a bad access region should fail";
                                });

    if (::testing::Test::HasFailure()) {
        if (acquireFence >= 0) {
            close(acquireFence);
        }

        int releaseFence = -1;
        ASSERT_NO_FATAL_FAILURE(releaseFence = mGralloc->unlock(bufferHandle));

        if (releaseFence >= 0) {
            close(releaseFence);
        }
    }
}

/**
 * Test IMapper::unlock with invalid buffers.
 */
TEST_P(GraphicsMapperHidlTest, UnlockNegative) {
    native_handle_t* invalidHandle = nullptr;
    mGralloc->getMapper()->unlock(invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
                << "unlock with nullptr did not fail with BAD_BUFFER";
    });

    invalidHandle = native_handle_create(0, 0);
    mGralloc->getMapper()->unlock(invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
                << "unlock with invalid handle did not fail with BAD_BUFFER";
    });
    native_handle_delete(invalidHandle);

    ASSERT_NO_FATAL_FAILURE(invalidHandle = const_cast<native_handle_t*>(
                                    mGralloc->allocate(mDummyDescriptorInfo, false)));
    mGralloc->getMapper()->unlock(invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
                << "unlock with un-imported handle did not fail with BAD_BUFFER";
    });
    mGralloc->freeBuffer(invalidHandle);

// disabled as it fails on many existing drivers
#if 0
  ASSERT_NO_FATAL_FAILURE(invalidHandle = const_cast<native_handle_t*>(
                              mGralloc->allocate(mDummyDescriptorInfo, true)));
  mGralloc->getMapper()->unlock(
      invalidHandle, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_BUFFER, tmpError)
            << "unlock with unlocked handle did not fail with BAD_BUFFER";
      });
  mGralloc->freeBuffer(invalidHandle);
#endif
}

/**
 * Test IMapper::flush and IMapper::reread.
 */
TEST_P(GraphicsMapperHidlTest, FlushRereadBasic) {
    const auto& info = mDummyDescriptorInfo;

    const native_handle_t* rawHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(rawHandle = mGralloc->allocate(mDummyDescriptorInfo, false,
                                                           Tolerance::kToleranceStrict, &stride));

    const native_handle_t* writeBufferHandle;
    const native_handle_t* readBufferHandle;
    ASSERT_NO_FATAL_FAILURE(writeBufferHandle = mGralloc->importBuffer(rawHandle));
    ASSERT_NO_FATAL_FAILURE(readBufferHandle = mGralloc->importBuffer(rawHandle));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    uint8_t* writeData;
    ASSERT_NO_FATAL_FAILURE(
            writeData = static_cast<uint8_t*>(mGralloc->lock(
                    writeBufferHandle, static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN), region,
                    -1)));

    uint8_t* readData;
    ASSERT_NO_FATAL_FAILURE(
            readData = static_cast<uint8_t*>(mGralloc->lock(
                    readBufferHandle, static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN), region,
                    -1)));

    fillRGBA8888(writeData, info.height, stride * 4, info.width * 4);

    unique_fd fence;
    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->flushLockedBuffer(writeBufferHandle)));
    if (fence >= 0) {
        ASSERT_EQ(0, sync_wait(fence, 3500));
    }

    ASSERT_NO_FATAL_FAILURE(mGralloc->rereadLockedBuffer(readBufferHandle));

    ASSERT_NO_FATAL_FAILURE(
            verifyRGBA8888(readBufferHandle, readData, info.height, stride * 4, info.width * 4));

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(readBufferHandle)));

    ASSERT_NO_FATAL_FAILURE(fence.reset(mGralloc->unlock(writeBufferHandle)));
}

/**
 * Test IMapper::flushLockedBuffer with bad buffer
 */
TEST_P(GraphicsMapperHidlTest, FlushLockedBufferBadBuffer) {
    ASSERT_NO_FATAL_FAILURE(mGralloc->getMapper()->flushLockedBuffer(
            nullptr, [&](const auto& tmpError, const auto& /*tmpReleaseFence*/) {
                ASSERT_EQ(Error::BAD_BUFFER, tmpError);
            }));
}

/**
 * Test IMapper::rereadLockedBuffer with bad buffer
 */
TEST_P(GraphicsMapperHidlTest, RereadLockedBufferBadBuffer) {
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->getMapper()->rereadLockedBuffer(nullptr));
}

/**
 * Test IMapper::isSupported with required format RGBA_8888
 */
TEST_P(GraphicsMapperHidlTest, IsSupportedRGBA8888) {
    const auto& info = mDummyDescriptorInfo;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
    ASSERT_TRUE(supported);
}

/**
 * Test IMapper::isSupported with required format YV12
 */
TEST_P(GraphicsMapperHidlTest, IsSupportedYV12) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YV12;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
    ASSERT_TRUE(supported);
}

/**
 * Test IMapper::isSupported with optional format Y16
 */
TEST_P(GraphicsMapperHidlTest, IsSupportedY16) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::Y16;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
}

/**
 * Test IMapper::isSupported with optional format R_8
 */
TEST_P(GraphicsMapperHidlTest, IsSupportedR8) {
    auto info = mDummyDescriptorInfo;
    info.format = static_cast<android::hardware::graphics::common::V1_2::PixelFormat>(
            aidl::android::hardware::graphics::common::PixelFormat::R_8);
    bool supported = false;

    supported = mGralloc->isSupportedNoFailure(info);

    if (!supported) {
        GTEST_SUCCEED() << "R_8 is optional; unsupported so skipping allocation test";
        return;
    }

    BufferDescriptor descriptor;
    ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(info));

    constexpr uint32_t count = 1;
    std::vector<const native_handle_t*> bufferHandles;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandles =
                                    mGralloc->allocate(descriptor, count, false,
                                                       Tolerance::kToleranceStrict, &stride));

    EXPECT_LE(info.width, stride) << "invalid buffer stride";
    EXPECT_EQ(1u, bufferHandles.size());

    for (auto bufferHandle : bufferHandles) {
        mGralloc->freeBuffer(bufferHandle);
    }
}

/**
 * Test IMapper::get(BufferId)
 */
TEST_P(GraphicsMapperHidlTest, GetBufferId) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_BufferId,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t bufferId = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeBufferId(vec, &bufferId));
            });
}

/**
 * Test IMapper::get(Name)
 */
TEST_P(GraphicsMapperHidlTest, GetName) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Name,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                std::string name;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeName(vec, &name));
                EXPECT_EQ(info.name, name);
            });
}

/**
 * Test IMapper::get(Width)
 */
TEST_P(GraphicsMapperHidlTest, GetWidth) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Width,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                uint64_t width = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeWidth(vec, &width));
                EXPECT_EQ(info.width, width);
            });
}

/**
 * Test IMapper::get(Height)
 */
TEST_P(GraphicsMapperHidlTest, GetHeight) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Height,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                uint64_t height = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeHeight(vec, &height));
                EXPECT_EQ(info.height, height);
            });
}

/**
 * Test IMapper::get(LayerCount)
 */
TEST_P(GraphicsMapperHidlTest, GetLayerCount) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_LayerCount,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                uint64_t layerCount = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeLayerCount(vec, &layerCount));
                EXPECT_EQ(info.layerCount, layerCount);
            });
}

/**
 * Test IMapper::get(PixelFormatRequested)
 */
TEST_P(GraphicsMapperHidlTest, GetPixelFormatRequested) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatRequested,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                PixelFormat pixelFormatRequested = PixelFormat::BLOB;
                ASSERT_EQ(NO_ERROR,
                          gralloc4::decodePixelFormatRequested(vec, &pixelFormatRequested));
                EXPECT_EQ(info.format, pixelFormatRequested);
            });
}

/**
 * Test IMapper::get(PixelFormatFourCC)
 */
TEST_P(GraphicsMapperHidlTest, GetPixelFormatFourCC) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatFourCC,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint32_t pixelFormatFourCC = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatFourCC(vec, &pixelFormatFourCC));
            });
}

/**
 * Test IMapper::get(PixelFormatModifier)
 */
TEST_P(GraphicsMapperHidlTest, GetPixelFormatModifier) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatModifier,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t pixelFormatModifier = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatModifier(vec, &pixelFormatModifier));
            });
}

/**
 * Test IMapper::get(Usage)
 */
TEST_P(GraphicsMapperHidlTest, GetUsage) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Usage,
            [](const IMapper::BufferDescriptorInfo& info, const hidl_vec<uint8_t>& vec) {
                uint64_t usage = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeUsage(vec, &usage));
                EXPECT_EQ(info.usage, usage);
            });
}

/**
 * Test IMapper::get(AllocationSize)
 */
TEST_P(GraphicsMapperHidlTest, GetAllocationSize) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_AllocationSize,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t allocationSize = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeAllocationSize(vec, &allocationSize));
            });
}

/**
 * Test IMapper::get(ProtectedContent)
 */
TEST_P(GraphicsMapperHidlTest, GetProtectedContent) {
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    const native_handle_t* bufferHandle = nullptr;
    bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceAllErrors);
    if (!bufferHandle) {
        GTEST_SUCCEED() << "unable to allocate protected content";
        return;
    }

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_ProtectedContent, &vec));

    uint64_t protectedContent = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeProtectedContent(vec, &protectedContent));
    EXPECT_EQ(1, protectedContent);
}

/**
 * Test IMapper::get(Compression)
 */
TEST_P(GraphicsMapperHidlTest, GetCompression) {
    auto info = mDummyDescriptorInfo;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    testGet(info, gralloc4::MetadataType_Compression,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType compression = gralloc4::Compression_DisplayStreamCompression;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCompression(vec, &compression));

                EXPECT_EQ(gralloc4::Compression_None.name, compression.name);
                EXPECT_EQ(gralloc4::Compression_None.value, compression.value);
            });
}

/**
 * Test IMapper::get(Interlaced)
 */
TEST_P(GraphicsMapperHidlTest, GetInterlaced) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Interlaced,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType interlaced = gralloc4::Interlaced_TopBottom;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeInterlaced(vec, &interlaced));

                EXPECT_EQ(gralloc4::Interlaced_None.name, interlaced.name);
                EXPECT_EQ(gralloc4::Interlaced_None.value, interlaced.value);
            });
}

/**
 * Test IMapper::get(ChromaSiting)
 */
TEST_P(GraphicsMapperHidlTest, GetChromaSiting) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_ChromaSiting,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType chromaSiting = gralloc4::ChromaSiting_Unknown;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeChromaSiting(vec, &chromaSiting));

                EXPECT_EQ(gralloc4::ChromaSiting_None.name, chromaSiting.name);
                EXPECT_EQ(gralloc4::ChromaSiting_None.value, chromaSiting.value);
            });
}

/**
 * Test IMapper::get(PlaneLayouts)
 */
TEST_P(GraphicsMapperHidlTest, GetPlaneLayouts) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));

    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

    ASSERT_NO_FATAL_FAILURE(verifyRGBA8888PlaneLayouts(planeLayouts));
}

/**
 * Test IMapper::get(Crop)
 */
TEST_P(GraphicsMapperHidlTest, GetCrop) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::RGBA_8888;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    testGet(info, gralloc4::MetadataType_Crop,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::vector<aidl::android::hardware::graphics::common::Rect> crops;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCrop(vec, &crops));
                EXPECT_EQ(1, crops.size());
            });
}

/**
 * Test IMapper::get(Dataspace)
 */
TEST_P(GraphicsMapperHidlTest, GetDataspace) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Dataspace,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                Dataspace dataspace = Dataspace::DISPLAY_P3;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeDataspace(vec, &dataspace));
                EXPECT_EQ(Dataspace::UNKNOWN, dataspace);
            });
}

/**
 * Test IMapper::get(BlendMode)
 */
TEST_P(GraphicsMapperHidlTest, GetBlendMode) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_BlendMode,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                BlendMode blendMode = BlendMode::NONE;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeBlendMode(vec, &blendMode));
                EXPECT_EQ(BlendMode::INVALID, blendMode);
            });
}

/**
 * Test IMapper::get(Smpte2086)
 */
TEST_P(GraphicsMapperHidlTest, GetSmpte2086) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2086,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<Smpte2086> smpte2086;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2086(vec, &smpte2086));
                EXPECT_FALSE(smpte2086.has_value());
            });
}

/**
 * Test IMapper::get(Cta861_3)
 */
TEST_P(GraphicsMapperHidlTest, GetCta861_3) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Cta861_3,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<Cta861_3> cta861_3;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCta861_3(vec, &cta861_3));
                EXPECT_FALSE(cta861_3.has_value());
            });
}

/**
 * Test IMapper::get(Smpte2094_40)
 */
TEST_P(GraphicsMapperHidlTest, GetSmpte2094_40) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2094_40,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<std::vector<uint8_t>> smpte2094_40;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_40(vec, &smpte2094_40));
                EXPECT_FALSE(smpte2094_40.has_value());
            });
}

/**
 * Test IMapper::get(Smpte2094_10)
 */
TEST_P(GraphicsMapperHidlTest, GetSmpte2094_10) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2094_10,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<std::vector<uint8_t>> smpte2094_10;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_10(vec, &smpte2094_10));
                EXPECT_FALSE(smpte2094_10.has_value());
            });
}

/**
 * Test IMapper::get(metadata) with a bad buffer
 */
TEST_P(GraphicsMapperHidlTest, GetMetadataBadValue) {
    const native_handle_t* bufferHandle = nullptr;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_BufferId, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->get(bufferHandle, gralloc4::MetadataType_Name, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->get(bufferHandle, gralloc4::MetadataType_Width, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->get(bufferHandle, gralloc4::MetadataType_Height, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_LayerCount, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_PixelFormatRequested, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_PixelFormatFourCC, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_PixelFormatModifier, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->get(bufferHandle, gralloc4::MetadataType_Usage, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_AllocationSize, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_ProtectedContent, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Compression, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Interlaced, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_ChromaSiting, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->get(bufferHandle, gralloc4::MetadataType_Crop, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Dataspace, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_BlendMode, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Smpte2086, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Cta861_3, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Smpte2094_40, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Smpte2094_10, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::get(metadata) for unsupported metadata
 */
TEST_P(GraphicsMapperHidlTest, GetUnsupportedMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    MetadataType metadataTypeFake = {"FAKE", 1};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->get(bufferHandle, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::get(metadata) for unsupported standard metadata
 */
TEST_P(GraphicsMapperHidlTest, GetUnsupportedStandardMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    MetadataType metadataTypeFake = {GRALLOC4_STANDARD_METADATA_TYPE, 9999};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->get(bufferHandle, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::set(PixelFormatFourCC)
 */
TEST_P(GraphicsMapperHidlTest, SetPixelFormatFourCC) {
    uint32_t pixelFormatFourCC = 0x34324142;  // DRM_FORMAT_BGRA8888
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePixelFormatFourCC(pixelFormatFourCC, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatFourCC, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint32_t realPixelFormatFourCC = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatFourCC(vec, &realPixelFormatFourCC));
                EXPECT_EQ(pixelFormatFourCC, realPixelFormatFourCC);
            });
}

/**
 * Test IMapper::set(PixelFormatModifier)
 */
TEST_P(GraphicsMapperHidlTest, SetPixelFormatModifier) {
    uint64_t pixelFormatModifier = 10;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePixelFormatModifier(pixelFormatModifier, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatModifier, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t realPixelFormatModifier = 0;
                ASSERT_EQ(NO_ERROR,
                          gralloc4::decodePixelFormatModifier(vec, &realPixelFormatModifier));
                EXPECT_EQ(pixelFormatModifier, realPixelFormatModifier);
            });
}

/**
 * Test IMapper::set(AllocationSize)
 */
TEST_P(GraphicsMapperHidlTest, SetAllocationSize) {
    uint64_t allocationSize = 1000000;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeAllocationSize(allocationSize, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_AllocationSize, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t realAllocationSize = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeAllocationSize(vec, &realAllocationSize));
                EXPECT_EQ(allocationSize, realAllocationSize);
            });
}

/**
 * Test IMapper::set(ProtectedContent)
 */
TEST_P(GraphicsMapperHidlTest, SetProtectedContent) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    bufferHandle = mGralloc->allocate(info, true, Tolerance::kToleranceAllErrors);
    if (!bufferHandle) {
        GTEST_SUCCEED() << "unable to allocate protected content";
        return;
    }

    uint64_t protectedContent = 0;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeProtectedContent(protectedContent, &vec));

    Error err = mGralloc->set(bufferHandle, gralloc4::MetadataType_ProtectedContent, vec);
    ASSERT_EQ(err, Error::UNSUPPORTED);
    vec.resize(0);

    uint64_t realProtectedContent = 0;
    ASSERT_EQ(Error::NONE,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_ProtectedContent, &vec));
    ASSERT_EQ(NO_ERROR, gralloc4::decodeProtectedContent(vec, &realProtectedContent));
    EXPECT_EQ(1, realProtectedContent);
}

/**
 * Test IMapper::set(Compression)
 */
TEST_P(GraphicsMapperHidlTest, SetCompression) {
    auto info = mDummyDescriptorInfo;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    ExtendableType compression = gralloc4::Compression_DisplayStreamCompression;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeCompression(compression, &vec));

    testSet(info, gralloc4::MetadataType_Compression, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType realCompression = gralloc4::Compression_None;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCompression(vec, &realCompression));

                EXPECT_EQ(compression.name, realCompression.name);
                EXPECT_EQ(compression.value, realCompression.value);
            });
}

/**
 * Test IMapper::set(Interlaced)
 */
TEST_P(GraphicsMapperHidlTest, SetInterlaced) {
    ExtendableType interlaced = gralloc4::Interlaced_RightLeft;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeInterlaced(interlaced, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Interlaced, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType realInterlaced = gralloc4::Interlaced_None;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeInterlaced(vec, &realInterlaced));

                EXPECT_EQ(interlaced.name, realInterlaced.name);
                EXPECT_EQ(interlaced.value, realInterlaced.value);
            });
}

/**
 * Test IMapper::set(ChromaSiting)
 */
TEST_P(GraphicsMapperHidlTest, SetChromaSiting) {
    ExtendableType chromaSiting = gralloc4::ChromaSiting_SitedInterstitial;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeChromaSiting(chromaSiting, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_ChromaSiting, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                ExtendableType realChromaSiting = gralloc4::ChromaSiting_None;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeChromaSiting(vec, &realChromaSiting));

                EXPECT_EQ(chromaSiting.name, realChromaSiting.name);
                EXPECT_EQ(chromaSiting.value, realChromaSiting.value);
            });
}

/**
 * Test IMapper::set(PlaneLayouts)
 */
TEST_P(GraphicsMapperHidlTest, SetPlaneLayouts) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true));

    std::vector<PlaneLayout> planeLayouts;
    PlaneLayout planeLayoutA;
    PlaneLayout planeLayoutRGB;
    PlaneLayoutComponent component;

    planeLayoutA.offsetInBytes = 0;
    planeLayoutA.sampleIncrementInBits = 8;
    planeLayoutA.strideInBytes = info.width + 20;
    planeLayoutA.widthInSamples = info.width;
    planeLayoutA.heightInSamples = info.height;
    planeLayoutA.totalSizeInBytes = planeLayoutA.strideInBytes * info.height;
    planeLayoutA.horizontalSubsampling = 1;
    planeLayoutA.verticalSubsampling = 1;

    component.type = gralloc4::PlaneLayoutComponentType_A;
    component.offsetInBits = 0;
    component.sizeInBits = 8;
    planeLayoutA.components.push_back(component);

    planeLayouts.push_back(planeLayoutA);

    planeLayoutRGB.offsetInBytes = 0;
    planeLayoutRGB.sampleIncrementInBits = 24;
    planeLayoutRGB.strideInBytes = info.width + 20;
    planeLayoutRGB.widthInSamples = info.width;
    planeLayoutRGB.heightInSamples = info.height;
    planeLayoutRGB.totalSizeInBytes = planeLayoutRGB.strideInBytes * info.height;
    planeLayoutRGB.horizontalSubsampling = 1;
    planeLayoutRGB.verticalSubsampling = 1;

    component.type = gralloc4::PlaneLayoutComponentType_R;
    planeLayoutRGB.components.push_back(component);
    component.type = gralloc4::PlaneLayoutComponentType_G;
    planeLayoutRGB.components.push_back(component);
    component.type = gralloc4::PlaneLayoutComponentType_B;
    planeLayoutRGB.components.push_back(component);

    planeLayouts.push_back(planeLayoutRGB);

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodePlaneLayouts(planeLayouts, &vec));

    Error err = mGralloc->set(bufferHandle, gralloc4::MetadataType_PlaneLayouts, vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::vector<PlaneLayout> realPlaneLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &realPlaneLayouts));

    ASSERT_EQ(planeLayouts.size(), realPlaneLayouts.size());

    for (int i = 0; i < realPlaneLayouts.size(); i++) {
        const auto& planeLayout = planeLayouts[i];
        const auto& realPlaneLayout = realPlaneLayouts[i];

        EXPECT_EQ(planeLayout.offsetInBytes, realPlaneLayout.offsetInBytes);
        EXPECT_EQ(planeLayout.sampleIncrementInBits, realPlaneLayout.sampleIncrementInBits);
        EXPECT_EQ(planeLayout.strideInBytes, realPlaneLayout.strideInBytes);
        EXPECT_EQ(planeLayout.widthInSamples, realPlaneLayout.widthInSamples);
        EXPECT_EQ(planeLayout.heightInSamples, realPlaneLayout.heightInSamples);
        EXPECT_LE(planeLayout.totalSizeInBytes, realPlaneLayout.totalSizeInBytes);
        EXPECT_EQ(planeLayout.horizontalSubsampling, realPlaneLayout.horizontalSubsampling);
        EXPECT_EQ(planeLayout.verticalSubsampling, realPlaneLayout.verticalSubsampling);

        ASSERT_EQ(planeLayout.components.size(), realPlaneLayout.components.size());

        for (int j = 0; j < realPlaneLayout.components.size(); j++) {
            const auto& component = planeLayout.components[j];
            const auto& realComponent = realPlaneLayout.components[j];

            EXPECT_EQ(component.type.name, realComponent.type.name);
            EXPECT_EQ(component.type.value, realComponent.type.value);
            EXPECT_EQ(component.sizeInBits, realComponent.sizeInBits);
            EXPECT_EQ(component.offsetInBits, realComponent.offsetInBits);
        }
    }
}

/**
 * Test IMapper::set(Crop)
 */
TEST_P(GraphicsMapperHidlTest, SetCrop) {
    std::vector<aidl::android::hardware::graphics::common::Rect> crops{{0, 0, 32, 32}};
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeCrop(crops, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Crop, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::vector<aidl::android::hardware::graphics::common::Rect> realCrops;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCrop(vec, &realCrops));
                ASSERT_EQ(1, realCrops.size());
                ASSERT_EQ(crops.front().left, realCrops.front().left);
                ASSERT_EQ(crops.front().top, realCrops.front().top);
                ASSERT_EQ(crops.front().right, realCrops.front().right);
                ASSERT_EQ(crops.front().bottom, realCrops.front().bottom);
            });
}

/**
 * Test IMapper::set(Dataspace)
 */
TEST_P(GraphicsMapperHidlTest, SetDataspace) {
    Dataspace dataspace = Dataspace::SRGB_LINEAR;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeDataspace(dataspace, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Dataspace, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                Dataspace realDataspace = Dataspace::UNKNOWN;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeDataspace(vec, &realDataspace));
                EXPECT_EQ(dataspace, realDataspace);
            });
}

/**
 * Test IMapper::set(BlendMode)
 */
TEST_P(GraphicsMapperHidlTest, SetBlendMode) {
    BlendMode blendMode = BlendMode::PREMULTIPLIED;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeBlendMode(blendMode, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_BlendMode, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                BlendMode realBlendMode = BlendMode::INVALID;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeBlendMode(vec, &realBlendMode));
                EXPECT_EQ(blendMode, realBlendMode);
            });
}

/**
 * Test IMapper::set(Smpte2086)
 */
TEST_P(GraphicsMapperHidlTest, SetSmpte2086) {
    /**
     * DISPLAY_P3 is a color space that uses the DCI_P3 primaries,
     * the D65 white point and the SRGB transfer functions.
     * Rendering Intent: Colorimetric
     * Primaries:
     *                  x       y
     *  green           0.265   0.690
     *  blue            0.150   0.060
     *  red             0.680   0.320
     *  white (D65)     0.3127  0.3290
     */
    Smpte2086 smpte2086;
    smpte2086.primaryRed.x = 0.680;
    smpte2086.primaryRed.y = 0.320;
    smpte2086.primaryGreen.x = 0.265;
    smpte2086.primaryGreen.y = 0.690;
    smpte2086.primaryBlue.x = 0.150;
    smpte2086.primaryBlue.y = 0.060;
    smpte2086.whitePoint.x = 0.3127;
    smpte2086.whitePoint.y = 0.3290;
    smpte2086.maxLuminance = 100.0;
    smpte2086.minLuminance = 0.1;

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeSmpte2086(smpte2086, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2086, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<Smpte2086> realSmpte2086;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2086(vec, &realSmpte2086));
                ASSERT_TRUE(realSmpte2086.has_value());
                EXPECT_TRUE(isEqual(smpte2086.primaryRed.x, realSmpte2086->primaryRed.x));
                EXPECT_TRUE(isEqual(smpte2086.primaryRed.y, realSmpte2086->primaryRed.y));
                EXPECT_TRUE(isEqual(smpte2086.primaryGreen.x, realSmpte2086->primaryGreen.x));
                EXPECT_TRUE(isEqual(smpte2086.primaryGreen.y, realSmpte2086->primaryGreen.y));
                EXPECT_TRUE(isEqual(smpte2086.primaryBlue.x, realSmpte2086->primaryBlue.x));
                EXPECT_TRUE(isEqual(smpte2086.primaryBlue.y, realSmpte2086->primaryBlue.y));
                EXPECT_TRUE(isEqual(smpte2086.whitePoint.x, realSmpte2086->whitePoint.x));
                EXPECT_TRUE(isEqual(smpte2086.whitePoint.y, realSmpte2086->whitePoint.y));
                EXPECT_TRUE(isEqual(smpte2086.maxLuminance, realSmpte2086->maxLuminance));
                EXPECT_TRUE(isEqual(smpte2086.minLuminance, realSmpte2086->minLuminance));
            });
}

/**
 * Test IMapper::set(Cta8613)
 */
TEST_P(GraphicsMapperHidlTest, SetCta861_3) {
    Cta861_3 cta861_3;
    cta861_3.maxContentLightLevel = 78.0;
    cta861_3.maxFrameAverageLightLevel = 62.0;

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeCta861_3(cta861_3, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Cta861_3, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<Cta861_3> realCta861_3;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeCta861_3(vec, &realCta861_3));
                ASSERT_TRUE(realCta861_3.has_value());
                EXPECT_TRUE(
                        isEqual(cta861_3.maxContentLightLevel, realCta861_3->maxContentLightLevel));
                EXPECT_TRUE(isEqual(cta861_3.maxFrameAverageLightLevel,
                                    realCta861_3->maxFrameAverageLightLevel));
            });
}

/**
 * Test IMapper::set(Smpte2094_40)
 */
TEST_P(GraphicsMapperHidlTest, SetSmpte2094_40) {
    hidl_vec<uint8_t> vec;

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2094_40, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<std::vector<uint8_t>> realSmpte2094_40;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_40(vec, &realSmpte2094_40));
                EXPECT_FALSE(realSmpte2094_40.has_value());
            });
}

/**
 * Test IMapper::set(Smpte2094_10)
 */
TEST_P(GraphicsMapperHidlTest, SetSmpte2094_10) {
    hidl_vec<uint8_t> vec;

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Smpte2094_10, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                std::optional<std::vector<uint8_t>> realSmpte2094_10;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_10(vec, &realSmpte2094_10));
                EXPECT_FALSE(realSmpte2094_10.has_value());
            });
}

/**
 * Test IMapper::set(metadata) with a bad buffer
 */
TEST_P(GraphicsMapperHidlTest, SetMetadataNullBuffer) {
    const native_handle_t* bufferHandle = nullptr;
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_BufferId, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Name, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Width, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Height, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_LayerCount, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatRequested, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatFourCC, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatModifier, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Usage, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_AllocationSize, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_ProtectedContent, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Compression, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Interlaced, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_ChromaSiting, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PlaneLayouts, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Crop, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Dataspace, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BlendMode, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Smpte2086, vec));
    ASSERT_EQ(Error::BAD_BUFFER, mGralloc->set(bufferHandle, gralloc4::MetadataType_Cta861_3, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Smpte2094_40, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Smpte2094_10, vec));
}

/**
 * Test get::metadata with cloned native_handle
 */
TEST_P(GraphicsMapperHidlTest, GetMetadataClonedHandle) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    const auto dataspace = Dataspace::SRGB_LINEAR;
    {
        hidl_vec<uint8_t> metadata;
        ASSERT_EQ(NO_ERROR, gralloc4::encodeDataspace(dataspace, &metadata));

        Error err = mGralloc->set(bufferHandle, gralloc4::MetadataType_Dataspace, metadata);
        if (err == Error::UNSUPPORTED) {
            GTEST_SUCCEED() << "setting this metadata is unsupported";
            return;
        }
        ASSERT_EQ(Error::NONE, err);
    }

    const native_handle_t* importedHandle;
    {
        auto clonedHandle = native_handle_clone(bufferHandle);
        ASSERT_NO_FATAL_FAILURE(importedHandle = mGralloc->importBuffer(clonedHandle));
        native_handle_close(clonedHandle);
        native_handle_delete(clonedHandle);
    }

    Dataspace realSpace = Dataspace::UNKNOWN;
    {
        hidl_vec<uint8_t> metadata;
        ASSERT_EQ(Error::NONE,
                  mGralloc->get(importedHandle, gralloc4::MetadataType_Dataspace, &metadata));
        ASSERT_NO_FATAL_FAILURE(gralloc4::decodeDataspace(metadata, &realSpace));
    }

    EXPECT_EQ(dataspace, realSpace);
}

/**
 * Test set::metadata with cloned native_handle
 */
TEST_P(GraphicsMapperHidlTest, SetMetadataClonedHandle) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    const native_handle_t* importedHandle;
    {
        auto clonedHandle = native_handle_clone(bufferHandle);
        ASSERT_NO_FATAL_FAILURE(importedHandle = mGralloc->importBuffer(clonedHandle));
        native_handle_close(clonedHandle);
        native_handle_delete(clonedHandle);
    }

    const auto dataspace = Dataspace::SRGB_LINEAR;
    {
        hidl_vec<uint8_t> metadata;
        ASSERT_EQ(NO_ERROR, gralloc4::encodeDataspace(dataspace, &metadata));

        Error err = mGralloc->set(importedHandle, gralloc4::MetadataType_Dataspace, metadata);
        if (err == Error::UNSUPPORTED) {
            GTEST_SUCCEED() << "setting this metadata is unsupported";
            return;
        }
        ASSERT_EQ(Error::NONE, err);
    }

    Dataspace realSpace = Dataspace::UNKNOWN;
    {
        hidl_vec<uint8_t> metadata;
        ASSERT_EQ(Error::NONE,
                  mGralloc->get(bufferHandle, gralloc4::MetadataType_Dataspace, &metadata));
        ASSERT_NO_FATAL_FAILURE(gralloc4::decodeDataspace(metadata, &realSpace));
    }

    EXPECT_EQ(dataspace, realSpace);
}

/**
 * Test IMapper::set(metadata) for constant metadata
 */
TEST_P(GraphicsMapperHidlTest, SetConstantMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    uint64_t bufferId = 2;
    hidl_vec<uint8_t> bufferIdVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeBufferId(bufferId, &bufferIdVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BufferId, bufferIdVec));

    std::string name{"new name"};
    hidl_vec<uint8_t> nameVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeName(name, &nameVec));
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_Name, nameVec));

    uint64_t width = 32;
    hidl_vec<uint8_t> widthVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeWidth(width, &widthVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Width, widthVec));

    uint64_t height = 32;
    hidl_vec<uint8_t> heightVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeHeight(height, &heightVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Height, heightVec));

    uint64_t layerCount = 2;
    hidl_vec<uint8_t> layerCountVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeLayerCount(layerCount, &layerCountVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_LayerCount, layerCountVec));

    hardware::graphics::common::V1_2::PixelFormat pixelFormatRequested = PixelFormat::BLOB;
    hidl_vec<uint8_t> pixelFormatRequestedVec;
    ASSERT_EQ(NO_ERROR,
              gralloc4::encodePixelFormatRequested(pixelFormatRequested, &pixelFormatRequestedVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatRequested,
                            pixelFormatRequestedVec));

    uint64_t usage = 0;
    hidl_vec<uint8_t> usageVec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeUsage(usage, &usageVec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Usage, usageVec));
}

/**
 * Test IMapper::set(metadata) for bad metadata
 */
TEST_P(GraphicsMapperHidlTest, SetBadMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatFourCC, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatModifier, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_AllocationSize, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_ProtectedContent, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Compression, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Interlaced, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_ChromaSiting, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PlaneLayouts, vec));
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->set(bufferHandle, gralloc4::MetadataType_Crop, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Dataspace, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BlendMode, vec));

    // Keep optional metadata types below and populate the encoded metadata vec
    // with some arbitrary different metadata because the common gralloc4::decode*()
    // functions do not distinguish between an empty vec and bad value.
    ASSERT_EQ(NO_ERROR, gralloc4::encodeDataspace(Dataspace::SRGB_LINEAR, &vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Smpte2086, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Cta861_3, vec));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(BufferId)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoBufferId) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                    gralloc4::MetadataType_BufferId, &vec));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Name)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoName) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Name, &vec));

    std::string name;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeName(vec, &name));
    EXPECT_EQ(mDummyDescriptorInfo.name, name);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Width)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoWidth) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Width, &vec));

    uint64_t width = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeWidth(vec, &width));
    EXPECT_EQ(mDummyDescriptorInfo.width, width);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Height)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoHeight) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Height, &vec));

    uint64_t height = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeHeight(vec, &height));
    EXPECT_EQ(mDummyDescriptorInfo.height, height);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PixelFormatRequested)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatRequested) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE,
              mGralloc->getFromBufferDescriptorInfo(
                      mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatRequested, &vec));

    PixelFormat pixelFormatRequested = PixelFormat::BLOB;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatRequested(vec, &pixelFormatRequested));
    EXPECT_EQ(mDummyDescriptorInfo.format, pixelFormatRequested);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PixelFormatFourCC)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatFourCC) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(
            mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatFourCC, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    uint32_t pixelFormatFourCC = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatFourCC(vec, &pixelFormatFourCC));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PixelFormatModifier)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatModifier) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(
            mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatModifier, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    uint64_t pixelFormatModifier = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatModifier(vec, &pixelFormatModifier));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Usage)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUsage) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Usage, &vec));

    uint64_t usage = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeUsage(vec, &usage));
    EXPECT_EQ(mDummyDescriptorInfo.usage, usage);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(AllocationSize)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoAllocationSize) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                      gralloc4::MetadataType_AllocationSize, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    uint64_t allocationSize = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeAllocationSize(vec, &allocationSize));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(ProtectedContent)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoProtectedContent) {
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(info, gralloc4::MetadataType_ProtectedContent,
                                                     &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    uint64_t protectedContent = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeProtectedContent(vec, &protectedContent));
    EXPECT_EQ(1, protectedContent);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Compression)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoCompression) {
    auto info = mDummyDescriptorInfo;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    hidl_vec<uint8_t> vec;
    auto err =
            mGralloc->getFromBufferDescriptorInfo(info, gralloc4::MetadataType_Compression, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    ExtendableType compression = gralloc4::Compression_DisplayStreamCompression;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeCompression(vec, &compression));

    EXPECT_EQ(gralloc4::Compression_None.name, compression.name);
    EXPECT_EQ(gralloc4::Compression_None.value, compression.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Interlaced)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoInterlaced) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Interlaced, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    ExtendableType interlaced = gralloc4::Interlaced_TopBottom;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeInterlaced(vec, &interlaced));

    EXPECT_EQ(gralloc4::Interlaced_None.name, interlaced.name);
    EXPECT_EQ(gralloc4::Interlaced_None.value, interlaced.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(ChromaSiting)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoChromaSiting) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_ChromaSiting, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    ExtendableType chromaSiting = gralloc4::ChromaSiting_CositedHorizontal;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeChromaSiting(vec, &chromaSiting));

    EXPECT_EQ(gralloc4::ChromaSiting_None.name, chromaSiting.name);
    EXPECT_EQ(gralloc4::ChromaSiting_None.value, chromaSiting.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PlaneLayouts)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPlaneLayouts) {
    hidl_vec<uint8_t> vec;
    const auto ret = mGralloc->getFromBufferDescriptorInfo(
            mDummyDescriptorInfo, gralloc4::MetadataType_PlaneLayouts, &vec);
    if (ret == Error::NONE) {
        std::vector<PlaneLayout> planeLayouts;
        ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));
        ASSERT_NO_FATAL_FAILURE(verifyRGBA8888PlaneLayouts(planeLayouts));
    } else {
        ASSERT_EQ(Error::UNSUPPORTED, ret);
    }
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Crop)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoCrop) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::RGBA_8888;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(info, gralloc4::MetadataType_Crop, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::vector<aidl::android::hardware::graphics::common::Rect> crops;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeCrop(vec, &crops));
    EXPECT_EQ(1, crops.size());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Dataspace)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoDataspace) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Dataspace, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    Dataspace dataspace = Dataspace::DISPLAY_P3;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeDataspace(vec, &dataspace));
    EXPECT_EQ(Dataspace::UNKNOWN, dataspace);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(BlendMode)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoBlendMode) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_BlendMode, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    BlendMode blendMode = BlendMode::COVERAGE;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeBlendMode(vec, &blendMode));
    EXPECT_EQ(BlendMode::INVALID, blendMode);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Smpte2086)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoSmpte2086) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Smpte2086, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::optional<Smpte2086> smpte2086;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2086(vec, &smpte2086));
    EXPECT_FALSE(smpte2086.has_value());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Cta861_3)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoCta861_3) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Cta861_3, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::optional<Cta861_3> cta861_3;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeCta861_3(vec, &cta861_3));
    EXPECT_FALSE(cta861_3.has_value());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Smpte2094_40)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoSmpte2094_40) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Smpte2094_40, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::optional<std::vector<uint8_t>> smpte2094_40;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_40(vec, &smpte2094_40));
    EXPECT_FALSE(smpte2094_40.has_value());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Smpte2094_10)
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoSmpte2094_10) {
    hidl_vec<uint8_t> vec;
    auto err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                     gralloc4::MetadataType_Smpte2094_10, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
        return;
    }
    ASSERT_EQ(err, Error::NONE);

    std::optional<std::vector<uint8_t>> smpte2094_10;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeSmpte2094_10(vec, &smpte2094_10));
    EXPECT_FALSE(smpte2094_10.has_value());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(metadata) for unsupported metadata
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUnsupportedMetadata) {
    MetadataType metadataTypeFake = {"FAKE", 1};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(metadata) for unsupported standard metadata
 */
TEST_P(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUnsupportedStandardMetadata) {
    MetadataType metadataTypeFake = {GRALLOC4_STANDARD_METADATA_TYPE, 9999};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::listSupportedMetadataTypes()
 */
TEST_P(GraphicsMapperHidlTest, ListSupportedMetadataTypes) {
    hidl_vec<IMapper::MetadataTypeDescription> descriptions;
    mGralloc->getMapper()->listSupportedMetadataTypes(
            [&](const auto& tmpError, const auto& tmpDescriptions) {
                ASSERT_EQ(Error::NONE, tmpError);
                descriptions = tmpDescriptions;
            });

    std::set<StandardMetadataType> foundMetadataTypes;

    std::set<StandardMetadataType> notSettableMetadataTypes{
            StandardMetadataType::BUFFER_ID,   StandardMetadataType::NAME,
            StandardMetadataType::WIDTH,       StandardMetadataType::HEIGHT,
            StandardMetadataType::LAYER_COUNT, StandardMetadataType::PIXEL_FORMAT_REQUESTED,
            StandardMetadataType::USAGE};

    ASSERT_LE(sRequiredMetadataTypes.size(), descriptions.size());

    for (const auto& description : descriptions) {
        const auto& metadataType = description.metadataType;

        if (!gralloc4::isStandardMetadataType(metadataType)) {
            EXPECT_GT(description.description.size(), 0);
            continue;
        }

        StandardMetadataType type = gralloc4::getStandardMetadataTypeValue(metadataType);

        if (sRequiredMetadataTypes.find(type) == sRequiredMetadataTypes.end()) {
            continue;
        }

        ASSERT_EQ(foundMetadataTypes.find(type), foundMetadataTypes.end());
        foundMetadataTypes.insert(type);

        ASSERT_TRUE(description.isGettable);

        if (notSettableMetadataTypes.find(type) != notSettableMetadataTypes.end()) {
            ASSERT_FALSE(description.isSettable);
        }
    }

    ASSERT_EQ(sRequiredMetadataTypes, foundMetadataTypes);
}

/**
 * Test IMapper::dumpBuffer()
 */
TEST_P(GraphicsMapperHidlTest, DumpBuffer) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    IMapper::BufferDump bufferDump;
    mGralloc->getMapper()->dumpBuffer(buffer, [&](const auto& tmpError, const auto& tmpBufferDump) {
        ASSERT_EQ(Error::NONE, tmpError);
        bufferDump = tmpBufferDump;
    });

    ASSERT_NO_FATAL_FAILURE(verifyBufferDump(bufferDump, buffer));
}

/**
 * Test IMapper::dumpBuffer() with an invalid buffer
 */
TEST_P(GraphicsMapperHidlTest, DumpBufferNullBuffer) {
    native_handle_t* bufferHandle = nullptr;
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    mGralloc->getMapper()->dumpBuffer(buffer,
                                      [&](const auto& tmpError, const auto& /*tmpBufferDump*/) {
                                          ASSERT_EQ(Error::BAD_BUFFER, tmpError);
                                      });
}

/**
 * Test IMapper::dumpBuffer() multiple
 */
TEST_P(GraphicsMapperHidlTest, DumpBuffers) {
    size_t bufferCount = 10;

    for (int i = 0; i < bufferCount; i++) {
        ASSERT_NO_FATAL_FAILURE(mGralloc->allocate(mDummyDescriptorInfo, true));
    }

    hidl_vec<IMapper::BufferDump> bufferDump;
    mGralloc->getMapper()->dumpBuffers([&](const auto& tmpError, const auto& tmpBufferDump) {
        ASSERT_EQ(Error::NONE, tmpError);
        bufferDump = tmpBufferDump;
    });

    ASSERT_EQ(bufferCount, bufferDump.size());

    for (const auto& dump : bufferDump) {
        ASSERT_NO_FATAL_FAILURE(verifyBufferDump(dump));
    }
}

/**
 * Test IMapper::getReservedRegion()
 */
TEST_P(GraphicsMapperHidlTest, GetReservedRegion) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;

    const int pageSize = getpagesize();
    ASSERT_GE(pageSize, 0);
    std::vector<uint64_t> requestedReservedSizes{1, 10, 333, static_cast<uint64_t>(pageSize) / 2,
                                                 static_cast<uint64_t>(pageSize)};

    for (auto requestedReservedSize : requestedReservedSizes) {
        info.reservedSize = requestedReservedSize;

        ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true));

        void* reservedRegion = nullptr;
        uint64_t reservedSize = 0;
        ASSERT_EQ(Error::NONE,
                  mGralloc->getReservedRegion(bufferHandle, &reservedRegion, &reservedSize));
        ASSERT_NE(nullptr, reservedRegion);
        ASSERT_EQ(requestedReservedSize, reservedSize);

        uint8_t testValue = 1;
        memset(reservedRegion, testValue, reservedSize);
        for (uint64_t i = 0; i < reservedSize; i++) {
            ASSERT_EQ(testValue, static_cast<uint8_t*>(reservedRegion)[i]);
        }
    }
}

/**
 * Test IMapper::getReservedRegion() request over a page
 */
TEST_P(GraphicsMapperHidlTest, GetLargeReservedRegion) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;

    const int pageSize = getpagesize();
    ASSERT_GE(pageSize, 0);
    std::vector<uint64_t> requestedReservedSizes{static_cast<uint64_t>(pageSize) * 2,
                                                 static_cast<uint64_t>(pageSize) * 10,
                                                 static_cast<uint64_t>(pageSize) * 1000};

    for (auto requestedReservedSize : requestedReservedSizes) {
        info.reservedSize = requestedReservedSize;

        BufferDescriptor descriptor;
        ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(info));

        Error err = Error::NONE;

        mGralloc->rawAllocate(
                descriptor, 1, [&](const auto& tmpError, const auto&, const auto& tmpBuffers) {
                    err = tmpError;
                    if (err == Error::NONE) {
                        ASSERT_EQ(1, tmpBuffers.size());
                        ASSERT_NO_FATAL_FAILURE(bufferHandle =
                                                        mGralloc->importBuffer(tmpBuffers[0]));
                    }
                });
        if (err == Error::UNSUPPORTED) {
            continue;
        }
        ASSERT_EQ(Error::NONE, err);

        void* reservedRegion = nullptr;
        uint64_t reservedSize = 0;
        err = mGralloc->getReservedRegion(bufferHandle, &reservedRegion, &reservedSize);

        ASSERT_EQ(Error::NONE, err);
        ASSERT_NE(nullptr, reservedRegion);
        ASSERT_EQ(requestedReservedSize, reservedSize);
    }
}

/**
 * Test IMapper::getReservedRegion() across multiple mappers
 */
TEST_P(GraphicsMapperHidlTest, GetReservedRegionMultiple) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;

    const int pageSize = getpagesize();
    ASSERT_GE(pageSize, 0);
    info.reservedSize = pageSize;

    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true));

    void* reservedRegion1 = nullptr;
    uint64_t reservedSize1 = 0;
    ASSERT_EQ(Error::NONE,
              mGralloc->getReservedRegion(bufferHandle, &reservedRegion1, &reservedSize1));
    ASSERT_NE(nullptr, reservedRegion1);
    ASSERT_EQ(info.reservedSize, reservedSize1);

    std::unique_ptr<Gralloc> anotherGralloc;
    ASSERT_NO_FATAL_FAILURE(anotherGralloc = std::make_unique<Gralloc>(std::get<0>(GetParam()),
                                                                       std::get<1>(GetParam())));

    void* reservedRegion2 = nullptr;
    uint64_t reservedSize2 = 0;
    ASSERT_EQ(Error::NONE,
              mGralloc->getReservedRegion(bufferHandle, &reservedRegion2, &reservedSize2));
    ASSERT_EQ(reservedRegion1, reservedRegion2);
    ASSERT_EQ(reservedSize1, reservedSize2);
}

/**
 * Test IMapper::getReservedRegion() with a bad buffer
 */
TEST_P(GraphicsMapperHidlTest, GetReservedRegionBadBuffer) {
    const native_handle_t* bufferHandle = nullptr;

    void* reservedRegion = nullptr;
    uint64_t reservedSize = 0;
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->getReservedRegion(bufferHandle, &reservedRegion, &reservedSize));
    ASSERT_EQ(nullptr, reservedRegion);
    ASSERT_EQ(0, reservedSize);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsMapperHidlTest);

namespace {
std::vector<std::string> getAllocatorInstances() {
    std::vector<std::string> instances;
    for (auto halInstance : android::hardware::getAllHalInstanceNames(IAllocator::descriptor)) {
        instances.emplace_back(std::move(halInstance));
    }

    for (auto aidlInstance : getAidlHalInstanceNames(
                 aidl::android::hardware::graphics::allocator::IAllocator::descriptor)) {
        instances.emplace_back(std::move(aidlInstance));
    }

    return instances;
}
}  // namespace

INSTANTIATE_TEST_CASE_P(
        PerInstance, GraphicsMapperHidlTest,
        testing::Combine(
                testing::ValuesIn(getAllocatorInstances()),
                testing::ValuesIn(android::hardware::getAllHalInstanceNames(IMapper::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

}  // namespace
}  // namespace vts
}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
