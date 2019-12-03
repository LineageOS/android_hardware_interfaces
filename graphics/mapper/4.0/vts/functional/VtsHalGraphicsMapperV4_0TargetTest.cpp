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

#include <chrono>
#include <thread>
#include <vector>

#include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <gralloctypes/Gralloc4.h>
#include <mapper-vts/4.0/MapperVts.h>
#include <system/graphics.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
namespace vts {
namespace {

using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;
using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::ExtendableType;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using aidl::android::hardware::graphics::common::StandardMetadataType;

using DecodeFunction = std::function<void(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                          const hidl_vec<uint8_t>& vec)>;

// Test environment for graphics.mapper.
class GraphicsMapperHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static GraphicsMapperHidlEnvironment* Instance() {
        static GraphicsMapperHidlEnvironment* instance = new GraphicsMapperHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override {
        registerTestService<IAllocator>();
        registerTestService<IMapper>();
    }
};

class GraphicsMapperHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                mGralloc = std::make_unique<Gralloc>(
                        GraphicsMapperHidlEnvironment::Instance()->getServiceName<IAllocator>(),
                        GraphicsMapperHidlEnvironment::Instance()->getServiceName<IMapper>()));
        ASSERT_NE(nullptr, mGralloc->getAllocator().get());
        ASSERT_NE(nullptr, mGralloc->getMapper().get());

        mDummyDescriptorInfo.name = "dummy";
        mDummyDescriptorInfo.width = 64;
        mDummyDescriptorInfo.height = 64;
        mDummyDescriptorInfo.layerCount = 1;
        mDummyDescriptorInfo.format = PixelFormat::RGBA_8888;
        mDummyDescriptorInfo.usage =
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);
    }

    void TearDown() override {}

    void testGet(const IMapper::BufferDescriptorInfo& descriptorInfo,
                 const MetadataType& metadataType, DecodeFunction decode) {
        const native_handle_t* bufferHandle = nullptr;
        ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(descriptorInfo, true));

        hidl_vec<uint8_t> vec;
        ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, metadataType, &vec));

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
        }
        ASSERT_EQ(err, Error::NONE);

        hidl_vec<uint8_t> vec;
        ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, metadataType, &vec));

        ASSERT_NO_FATAL_FAILURE(decode(descriptorInfo, vec));
    }

    void verifyDummyDescriptorInfoPlaneLayouts(const std::vector<PlaneLayout>& planeLayouts) {
        ASSERT_EQ(1, planeLayouts.size());

        const auto& planeLayout = planeLayouts.front();

        ASSERT_EQ(4, planeLayout.components.size());

        int64_t offsetInBitsR = -1;
        int64_t offsetInBitsG = -1;
        int64_t offsetInBitsB = -1;
        int64_t offsetInBitsA = -1;

        for (const auto& component : planeLayout.components) {
            EXPECT_EQ(GRALLOC4_PLANE_LAYOUT_COMPONENT_TYPE, component.type.name);
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
        EXPECT_EQ(8, planeLayout.sampleIncrementInBits);
        // Skip testing stride because any stride is valid
        EXPECT_EQ(mDummyDescriptorInfo.width, planeLayout.widthInSamples);
        EXPECT_EQ(mDummyDescriptorInfo.height, planeLayout.heightInSamples);
        EXPECT_LE(planeLayout.widthInSamples * planeLayout.heightInSamples * 4,
                  planeLayout.totalSizeInBytes);
        EXPECT_EQ(1, planeLayout.horizontalSubsampling);
        EXPECT_EQ(1, planeLayout.verticalSubsampling);

        EXPECT_EQ(0, planeLayout.crop.left);
        EXPECT_EQ(0, planeLayout.crop.top);
        EXPECT_EQ(planeLayout.widthInSamples, planeLayout.crop.right);
        EXPECT_EQ(planeLayout.heightInSamples, planeLayout.crop.bottom);
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
                         android_ycbcr* outYCbCr) {
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
                std::string componentTypeName = planeLayoutComponent.type.name;
                if (!std::strncmp(componentTypeName.c_str(), GRALLOC4_PLANE_LAYOUT_COMPONENT_TYPE,
                                  componentTypeName.size())) {
                    continue;
                }
                ASSERT_EQ(0, planeLayoutComponent.offsetInBits % 8);

                uint8_t* tmpData =
                        data + planeLayout.offsetInBytes + (planeLayoutComponent.offsetInBits / 8);
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
 * Test IAllocator::dumpDebugInfo by calling it.
 */
TEST_F(GraphicsMapperHidlTest, AllocatorDumpDebugInfo) {
    mGralloc->dumpDebugInfo();
}

/**
 * Test IAllocator::allocate with valid buffer descriptors.
 */
TEST_F(GraphicsMapperHidlTest, AllocatorAllocate) {
    BufferDescriptor descriptor;
    ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(mDummyDescriptorInfo));

    for (uint32_t count = 0; count < 5; count++) {
        std::vector<const native_handle_t*> bufferHandles;
        uint32_t stride;
        ASSERT_NO_FATAL_FAILURE(
                bufferHandles = mGralloc->allocate(descriptor, count, false, false, &stride));

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
TEST_F(GraphicsMapperHidlTest, AllocatorAllocateNegative) {
    // this assumes any valid descriptor is non-empty
    BufferDescriptor descriptor;
    mGralloc->getAllocator()->allocate(descriptor, 1,
                                       [&](const auto& tmpError, const auto&, const auto&) {
                                           EXPECT_EQ(Error::BAD_DESCRIPTOR, tmpError);
                                       });
}

/**
 * Test IAllocator::allocate does not leak.
 */
TEST_F(GraphicsMapperHidlTest, AllocatorAllocateNoLeak) {
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
TEST_F(GraphicsMapperHidlTest, AllocatorAllocateThreaded) {
    BufferDescriptor descriptor;
    ASSERT_NO_FATAL_FAILURE(descriptor = mGralloc->createDescriptor(mDummyDescriptorInfo));

    std::atomic<bool> timeUp(false);
    std::atomic<uint64_t> allocationCount(0);
    auto threadLoop = [&]() {
        while (!timeUp) {
            mGralloc->getAllocator()->allocate(
                    descriptor, 1,
                    [&](const auto&, const auto&, const auto&) { allocationCount++; });
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
TEST_F(GraphicsMapperHidlTest, CreateDescriptorBasic) {
    ASSERT_NO_FATAL_FAILURE(mGralloc->createDescriptor(mDummyDescriptorInfo));
}

/**
 * Test IMapper::createDescriptor with invalid descriptor info.
 */
TEST_F(GraphicsMapperHidlTest, CreateDescriptorNegative) {
    auto info = mDummyDescriptorInfo;
    info.width = 0;
    mGralloc->getMapper()->createDescriptor(info, [&](const auto& tmpError, const auto&) {
        EXPECT_EQ(Error::BAD_VALUE, tmpError) << "createDescriptor did not fail with BAD_VALUE";
    });
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer with allocated buffers.
 */
TEST_F(GraphicsMapperHidlTest, ImportFreeBufferBasic) {
    const native_handle_t* bufferHandle;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));
    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(bufferHandle));
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer with cloned buffers.
 */
TEST_F(GraphicsMapperHidlTest, ImportFreeBufferClone) {
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
TEST_F(GraphicsMapperHidlTest, ImportFreeBufferSingleton) {
    const native_handle_t* rawHandle;
    ASSERT_NO_FATAL_FAILURE(rawHandle = mGralloc->allocate(mDummyDescriptorInfo, false));

    native_handle_t* importedHandle = nullptr;
    mGralloc->getMapper()->importBuffer(rawHandle, [&](const auto& tmpError, const auto& buffer) {
        ASSERT_EQ(Error::NONE, tmpError);
        importedHandle = static_cast<native_handle_t*>(buffer);
    });

    // free the imported handle with another mapper
    std::unique_ptr<Gralloc> anotherGralloc;
    ASSERT_NO_FATAL_FAILURE(
            anotherGralloc = std::make_unique<Gralloc>(
                    GraphicsMapperHidlEnvironment::Instance()->getServiceName<IAllocator>(),
                    GraphicsMapperHidlEnvironment::Instance()->getServiceName<IMapper>()));
    Error error = mGralloc->getMapper()->freeBuffer(importedHandle);
    ASSERT_EQ(Error::NONE, error);

    ASSERT_NO_FATAL_FAILURE(mGralloc->freeBuffer(rawHandle));
}

/**
 * Test IMapper::importBuffer and IMapper::freeBuffer do not leak.
 */
TEST_F(GraphicsMapperHidlTest, ImportFreeBufferNoLeak) {
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
TEST_F(GraphicsMapperHidlTest, ImportBufferNegative) {
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
TEST_F(GraphicsMapperHidlTest, FreeBufferNegative) {
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
TEST_F(GraphicsMapperHidlTest, LockUnlockBasic) {
    const auto& info = mDummyDescriptorInfo;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true, false, &stride));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    int fence = -1;
    uint8_t* data;
    ASSERT_NO_FATAL_FAILURE(
            data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage, region, fence)));

    // RGBA_8888
    size_t strideInBytes = stride * 4;
    size_t writeInBytes = info.width * 4;

    for (uint32_t y = 0; y < info.height; y++) {
        memset(data, y, writeInBytes);
        data += strideInBytes;
    }

    ASSERT_NO_FATAL_FAILURE(fence = mGralloc->unlock(bufferHandle));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(
            data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage, region, fence)));
    for (uint32_t y = 0; y < info.height; y++) {
        for (size_t i = 0; i < writeInBytes; i++) {
            EXPECT_EQ(static_cast<uint8_t>(y), data[i]);
        }
        data += strideInBytes;
    }

    ASSERT_NO_FATAL_FAILURE(fence = mGralloc->unlock(bufferHandle));
    if (fence >= 0) {
        close(fence);
    }
}

TEST_F(GraphicsMapperHidlTest, Lock_YCBCR_420_888) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YCBCR_420_888;

    const native_handle_t* bufferHandle;
    uint32_t stride;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(info, true, false, &stride));

    // lock buffer for writing
    const IMapper::Rect region{0, 0, static_cast<int32_t>(info.width),
                               static_cast<int32_t>(info.height)};
    int fence = -1;
    uint8_t* data;

    ASSERT_NO_FATAL_FAILURE(
            data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage, region, fence)));

    android_ycbcr yCbCr;
    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(bufferHandle, data, &yCbCr));

    auto yData = static_cast<uint8_t*>(yCbCr.y);
    auto cbData = static_cast<uint8_t*>(yCbCr.cb);
    auto crData = static_cast<uint8_t*>(yCbCr.cr);
    auto yStride = yCbCr.ystride;
    auto cStride = yCbCr.cstride;
    auto chromaStep = yCbCr.chroma_step;

    for (uint32_t y = 0; y < info.height; y++) {
        for (uint32_t x = 0; x < info.width; x++) {
            auto val = static_cast<uint8_t>(info.height * y + x);

            yData[yStride * y + x] = val;

            if (y % chromaStep && x % chromaStep == 0) {
                cbData[cStride * y / chromaStep + x / chromaStep] = val;
                crData[cStride * y / chromaStep + x / chromaStep] = val;
            }
        }
    }

    ASSERT_NO_FATAL_FAILURE(fence = mGralloc->unlock(bufferHandle));

    // lock again for reading
    ASSERT_NO_FATAL_FAILURE(
            data = static_cast<uint8_t*>(mGralloc->lock(bufferHandle, info.usage, region, fence)));

    ASSERT_NO_FATAL_FAILURE(getAndroidYCbCr(bufferHandle, data, &yCbCr));

    yData = static_cast<uint8_t*>(yCbCr.y);
    cbData = static_cast<uint8_t*>(yCbCr.cb);
    crData = static_cast<uint8_t*>(yCbCr.cr);
    for (uint32_t y = 0; y < info.height; y++) {
        for (uint32_t x = 0; x < info.width; x++) {
            auto val = static_cast<uint8_t>(info.height * y + x);

            EXPECT_EQ(val, yData[yStride * y + x]);

            if (y % chromaStep == 0 && x % chromaStep == 0) {
                EXPECT_EQ(val, cbData[cStride * y / chromaStep + x / chromaStep]);
                EXPECT_EQ(val, crData[cStride * y / chromaStep + x / chromaStep]);
            }
        }
    }

    ASSERT_NO_FATAL_FAILURE(fence = mGralloc->unlock(bufferHandle));
    if (fence >= 0) {
        close(fence);
    }
}

/**
 * Test IMapper::unlock with bad access region
 */
TEST_F(GraphicsMapperHidlTest, LockBadAccessRegion) {
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
TEST_F(GraphicsMapperHidlTest, UnlockNegative) {
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
 * Test IMapper::isSupported with required format RGBA_8888
 */
TEST_F(GraphicsMapperHidlTest, IsSupportedRGBA8888) {
    const auto& info = mDummyDescriptorInfo;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
    ASSERT_TRUE(supported);
}

/**
 * Test IMapper::isSupported with required format YV12
 */
TEST_F(GraphicsMapperHidlTest, IsSupportedYV12) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::YV12;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
    ASSERT_TRUE(supported);
}

/**
 * Test IMapper::isSupported with optional format Y16
 */
TEST_F(GraphicsMapperHidlTest, IsSupportedY16) {
    auto info = mDummyDescriptorInfo;
    info.format = PixelFormat::Y16;
    bool supported = false;

    ASSERT_NO_FATAL_FAILURE(supported = mGralloc->isSupported(info));
}

/**
 * Test IMapper::get(BufferId)
 */
TEST_F(GraphicsMapperHidlTest, GetBufferId) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_BufferId,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t bufferId = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeBufferId(vec, &bufferId));
            });
}

/**
 * Test IMapper::get(Name)
 */
TEST_F(GraphicsMapperHidlTest, GetName) {
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
TEST_F(GraphicsMapperHidlTest, GetWidth) {
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
TEST_F(GraphicsMapperHidlTest, GetHeight) {
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
TEST_F(GraphicsMapperHidlTest, GetLayerCount) {
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
TEST_F(GraphicsMapperHidlTest, GetPixelFormatRequested) {
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
TEST_F(GraphicsMapperHidlTest, GetPixelFormatFourCC) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatFourCC,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint32_t pixelFormatFourCC = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatFourCC(vec, &pixelFormatFourCC));
            });
}

/**
 * Test IMapper::get(PixelFormatModifier)
 */
TEST_F(GraphicsMapperHidlTest, GetPixelFormatModifier) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatModifier,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t pixelFormatModifier = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatModifier(vec, &pixelFormatModifier));
            });
}

/**
 * Test IMapper::get(Usage)
 */
TEST_F(GraphicsMapperHidlTest, GetUsage) {
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
TEST_F(GraphicsMapperHidlTest, GetAllocationSize) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_AllocationSize,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t allocationSize = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeAllocationSize(vec, &allocationSize));
            });
}

/**
 * Test IMapper::get(ProtectedContent)
 */
TEST_F(GraphicsMapperHidlTest, GetProtectedContent) {
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    const native_handle_t* bufferHandle = nullptr;
    bufferHandle = mGralloc->allocate(info, true, true);
    if (bufferHandle) {
        GTEST_SUCCEED() << "unable to allocate protected content";
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
TEST_F(GraphicsMapperHidlTest, GetCompression) {
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
TEST_F(GraphicsMapperHidlTest, GetInterlaced) {
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
TEST_F(GraphicsMapperHidlTest, GetChromaSiting) {
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
TEST_F(GraphicsMapperHidlTest, GetPlaneLayouts) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_PlaneLayouts, &vec));

    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));

    ASSERT_NO_FATAL_FAILURE(verifyDummyDescriptorInfoPlaneLayouts(planeLayouts));
}

/**
 * Test IMapper::get(Dataspace)
 */
TEST_F(GraphicsMapperHidlTest, GetDataspace) {
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
TEST_F(GraphicsMapperHidlTest, GetBlendMode) {
    testGet(mDummyDescriptorInfo, gralloc4::MetadataType_BlendMode,
            [](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                BlendMode blendMode = BlendMode::NONE;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeBlendMode(vec, &blendMode));
                EXPECT_EQ(BlendMode::INVALID, blendMode);
            });
}

/**
 * Test IMapper::get(metadata) with a bad buffer
 */
TEST_F(GraphicsMapperHidlTest, GetMetadataBadValue) {
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
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_Dataspace, &vec));
    ASSERT_EQ(0, vec.size());
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->get(bufferHandle, gralloc4::MetadataType_BlendMode, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::get(metadata) for unsupported metadata
 */
TEST_F(GraphicsMapperHidlTest, GetUnsupportedMetadata) {
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
TEST_F(GraphicsMapperHidlTest, GetUnsupportedStandardMetadata) {
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
TEST_F(GraphicsMapperHidlTest, SetPixelFormatFourCC) {
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
TEST_F(GraphicsMapperHidlTest, SetPixelFormatModifier) {
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
 * Test IMapper::set(Usage) remove flag
 */
TEST_F(GraphicsMapperHidlTest, SetUsageRemoveBit) {
    uint64_t usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN);
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeUsage(usage, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Usage, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t realUsage = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeUsage(vec, &realUsage));
                EXPECT_EQ(usage, realUsage);
            });
}
/**
 * Test IMapper::set(Usage) add flag
 */
TEST_F(GraphicsMapperHidlTest, SetUsageAddBit) {
    uint64_t usage = mDummyDescriptorInfo.usage | static_cast<uint64_t>(BufferUsage::GPU_TEXTURE);
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeUsage(usage, &vec));

    testSet(mDummyDescriptorInfo, gralloc4::MetadataType_Usage, vec,
            [&](const IMapper::BufferDescriptorInfo& /*info*/, const hidl_vec<uint8_t>& vec) {
                uint64_t realUsage = 0;
                ASSERT_EQ(NO_ERROR, gralloc4::decodeUsage(vec, &realUsage));
                EXPECT_EQ(usage, realUsage);
            });
}

/**
 * Test IMapper::set(Usage) to test protected content
 */
TEST_F(GraphicsMapperHidlTest, SetUsageProtected) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    bufferHandle = mGralloc->allocate(info, true, true);
    if (bufferHandle) {
        GTEST_SUCCEED() << "unable to allocate protected content";
    }

    uint64_t usage = static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY);
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(NO_ERROR, gralloc4::encodeUsage(usage, &vec));

    Error err = mGralloc->set(bufferHandle, gralloc4::MetadataType_Usage, vec);
    ASSERT_EQ(err, Error::UNSUPPORTED);
    vec.resize(0);

    uint64_t realUsage = 0;
    ASSERT_EQ(Error::NONE, mGralloc->get(bufferHandle, gralloc4::MetadataType_Usage, &vec));
    ASSERT_EQ(NO_ERROR, gralloc4::decodeUsage(vec, &realUsage));
    EXPECT_EQ(info.usage, realUsage);
}

/**
 * Test IMapper::set(AllocationSize)
 */
TEST_F(GraphicsMapperHidlTest, SetAllocationSize) {
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
TEST_F(GraphicsMapperHidlTest, SetProtectedContent) {
    const native_handle_t* bufferHandle = nullptr;
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    bufferHandle = mGralloc->allocate(info, true, true);
    if (bufferHandle) {
        GTEST_SUCCEED() << "unable to allocate protected content";
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
TEST_F(GraphicsMapperHidlTest, SetCompression) {
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
TEST_F(GraphicsMapperHidlTest, SetInterlaced) {
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
TEST_F(GraphicsMapperHidlTest, SetChromaSiting) {
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
TEST_F(GraphicsMapperHidlTest, SetPlaneLayouts) {
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
    planeLayoutA.crop.left = 0;
    planeLayoutA.crop.top = 0;
    planeLayoutA.crop.right = info.width;
    planeLayoutA.crop.bottom = info.height;

    component.type = gralloc4::PlaneLayoutComponentType_A;
    component.offsetInBits = 0;
    component.sizeInBits = 8;
    planeLayoutA.components.push_back(component);

    planeLayouts.push_back(planeLayoutA);

    planeLayoutRGB.offsetInBytes = 0;
    planeLayoutRGB.sampleIncrementInBits = 32;
    planeLayoutRGB.strideInBytes = info.width + 20;
    planeLayoutRGB.widthInSamples = info.width;
    planeLayoutRGB.heightInSamples = info.height;
    planeLayoutRGB.totalSizeInBytes = planeLayoutRGB.strideInBytes * info.height;
    planeLayoutRGB.horizontalSubsampling = 1;
    planeLayoutRGB.verticalSubsampling = 1;
    planeLayoutRGB.crop.left = 0;
    planeLayoutRGB.crop.top = 0;
    planeLayoutRGB.crop.right = info.width;
    planeLayoutRGB.crop.bottom = info.height;

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

        EXPECT_EQ(planeLayout.crop.left, realPlaneLayout.crop.left);
        EXPECT_EQ(planeLayout.crop.top, realPlaneLayout.crop.top);
        EXPECT_EQ(planeLayout.crop.right, realPlaneLayout.crop.right);
        EXPECT_EQ(planeLayout.crop.bottom, realPlaneLayout.crop.bottom);

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
 * Test IMapper::set(Dataspace)
 */
TEST_F(GraphicsMapperHidlTest, SetDataspace) {
    Dataspace dataspace = Dataspace::V0_SRGB_LINEAR;
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
TEST_F(GraphicsMapperHidlTest, SetBlendMode) {
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
 * Test IMapper::set(metadata) with a bad buffer
 */
TEST_F(GraphicsMapperHidlTest, SetMetadataNullBuffer) {
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
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Dataspace, vec));
    ASSERT_EQ(Error::BAD_BUFFER,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BlendMode, vec));
}

/**
 * Test IMapper::set(metadata) for constant metadata
 */
TEST_F(GraphicsMapperHidlTest, SetConstantMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_BufferId, vec));
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_Name, vec));
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_Width, vec));
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_Height, vec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_LayerCount, vec));
    ASSERT_EQ(Error::BAD_VALUE,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatRequested, vec));
    ASSERT_EQ(Error::BAD_VALUE, mGralloc->set(bufferHandle, gralloc4::MetadataType_Usage, vec));
}

/**
 * Test IMapper::set(metadata) for bad metadata
 */
TEST_F(GraphicsMapperHidlTest, SetBadMetadata) {
    const native_handle_t* bufferHandle = nullptr;
    ASSERT_NO_FATAL_FAILURE(bufferHandle = mGralloc->allocate(mDummyDescriptorInfo, true));

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BufferId, vec));
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->set(bufferHandle, gralloc4::MetadataType_Name, vec));
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->set(bufferHandle, gralloc4::MetadataType_Width, vec));
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->set(bufferHandle, gralloc4::MetadataType_Height, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_LayerCount, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatRequested, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatFourCC, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_PixelFormatModifier, vec));
    ASSERT_EQ(Error::UNSUPPORTED, mGralloc->set(bufferHandle, gralloc4::MetadataType_Usage, vec));
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
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_Dataspace, vec));
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->set(bufferHandle, gralloc4::MetadataType_BlendMode, vec));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(BufferId)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoBufferId) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                    gralloc4::MetadataType_BufferId, &vec));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Name)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoName) {
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
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoWidth) {
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
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoHeight) {
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
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatRequested) {
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
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatFourCC) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(
            mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatFourCC, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
    }
    ASSERT_EQ(err, Error::NONE);

    uint32_t pixelFormatFourCC = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatFourCC(vec, &pixelFormatFourCC));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PixelFormatModifier)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPixelFormatModifier) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(
            mDummyDescriptorInfo, gralloc4::MetadataType_PixelFormatModifier, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
    }
    ASSERT_EQ(err, Error::NONE);

    uint64_t pixelFormatModifier = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePixelFormatModifier(vec, &pixelFormatModifier));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Usage)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUsage) {
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
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoAllocationSize) {
    hidl_vec<uint8_t> vec;
    Error err = mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                      gralloc4::MetadataType_AllocationSize, &vec);
    if (err == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "setting this metadata is unsupported";
    }
    ASSERT_EQ(err, Error::NONE);

    uint64_t allocationSize = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeAllocationSize(vec, &allocationSize));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(ProtectedContent)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoProtectedContent) {
    auto info = mDummyDescriptorInfo;
    info.usage = BufferUsage::PROTECTED | BufferUsage::COMPOSER_OVERLAY;

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   info, gralloc4::MetadataType_ProtectedContent, &vec));

    uint64_t protectedContent = 0;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeProtectedContent(vec, &protectedContent));
    EXPECT_EQ(1, protectedContent);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Compression)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoCompression) {
    auto info = mDummyDescriptorInfo;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   info, gralloc4::MetadataType_Compression, &vec));

    ExtendableType compression = gralloc4::Compression_DisplayStreamCompression;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeCompression(vec, &compression));

    EXPECT_EQ(gralloc4::Compression_None.name, compression.name);
    EXPECT_EQ(gralloc4::Compression_None.value, compression.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Interlaced)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoInterlaced) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Interlaced, &vec));

    ExtendableType interlaced = gralloc4::Interlaced_TopBottom;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeInterlaced(vec, &interlaced));

    EXPECT_EQ(gralloc4::Interlaced_None.name, interlaced.name);
    EXPECT_EQ(gralloc4::Interlaced_None.value, interlaced.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(ChromaSiting)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoChromaSiting) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                    gralloc4::MetadataType_ChromaSiting, &vec));

    ExtendableType chromaSiting = gralloc4::ChromaSiting_CositedHorizontal;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeChromaSiting(vec, &chromaSiting));

    EXPECT_EQ(gralloc4::ChromaSiting_None.name, chromaSiting.name);
    EXPECT_EQ(gralloc4::ChromaSiting_None.value, chromaSiting.value);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(PlaneLayouts)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoPlaneLayouts) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo,
                                                    gralloc4::MetadataType_PlaneLayouts, &vec));

    std::vector<PlaneLayout> planeLayouts;
    ASSERT_EQ(NO_ERROR, gralloc4::decodePlaneLayouts(vec, &planeLayouts));
    ASSERT_NO_FATAL_FAILURE(verifyDummyDescriptorInfoPlaneLayouts(planeLayouts));
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(Dataspace)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoDataspace) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_Dataspace, &vec));

    Dataspace dataspace = Dataspace::DISPLAY_P3;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeDataspace(vec, &dataspace));
    EXPECT_EQ(Dataspace::UNKNOWN, dataspace);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(BlendMode)
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoBlendMode) {
    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::NONE, mGralloc->getFromBufferDescriptorInfo(
                                   mDummyDescriptorInfo, gralloc4::MetadataType_BlendMode, &vec));

    BlendMode blendMode = BlendMode::COVERAGE;
    ASSERT_EQ(NO_ERROR, gralloc4::decodeBlendMode(vec, &blendMode));
    EXPECT_EQ(BlendMode::INVALID, blendMode);
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(metadata) for unsupported metadata
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUnsupportedMetadata) {
    MetadataType metadataTypeFake = {"FAKE", 1};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::getFromBufferDescriptorInfo(metadata) for unsupported standard metadata
 */
TEST_F(GraphicsMapperHidlTest, GetFromBufferDescriptorInfoUnsupportedStandardMetadata) {
    MetadataType metadataTypeFake = {GRALLOC4_STANDARD_METADATA_TYPE, 9999};

    hidl_vec<uint8_t> vec;
    ASSERT_EQ(Error::UNSUPPORTED,
              mGralloc->getFromBufferDescriptorInfo(mDummyDescriptorInfo, metadataTypeFake, &vec));
    ASSERT_EQ(0, vec.size());
}

/**
 * Test IMapper::listSupportedMetadataTypes()
 */
TEST_F(GraphicsMapperHidlTest, ListSupportedMetadataTypes) {
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
            EXPECT_GT(0, description.description.size());
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
TEST_F(GraphicsMapperHidlTest, DumpBuffer) {
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
TEST_F(GraphicsMapperHidlTest, DumpBufferNullBuffer) {
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
TEST_F(GraphicsMapperHidlTest, DumpBuffers) {
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

}  // namespace
}  // namespace vts
}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::mapper::V4_0::vts::GraphicsMapperHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsMapperHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsMapperHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
