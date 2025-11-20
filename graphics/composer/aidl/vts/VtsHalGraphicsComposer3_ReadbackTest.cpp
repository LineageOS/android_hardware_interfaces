/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "graphics_composer_aidl_hal_readback_tests@3"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <cutils/ashmem.h>
#include <gtest/gtest.h>
#include <ui/DisplayId.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include "ComposerClientWrapper.h"
#include "GraphicsComposerCallback.h"
#include "Readback.h"
#include "RenderEngine.h"

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using namespace ::aidl::android::hardware::graphics::composer3::libhwc_aidl_test;

using ::android::Rect;
using common::Dataspace;
using common::PixelFormat;

class GraphicsCompositionTestBase : public ::testing::Test {
  protected:
    void SetUpBase(const std::string& name) {
        mComposerClient = std::make_shared<ComposerClientWrapper>(name);
        ASSERT_TRUE(mComposerClient->createClient().isOk());

        const auto& [status, displays] = mComposerClient->getDisplays();
        ASSERT_TRUE(status.isOk());
        mAllDisplays = displays;

        setUpDisplayProperties();

        for (const auto& display : mAllDisplays) {
            // explicitly disable vsync
            EXPECT_TRUE(mComposerClient->setVsync(display.getDisplayId(), /*enable*/ false).isOk());
        }

        mComposerClient->setVsyncAllowed(/*isAllowed*/ false);
    }

    void TearDown() override {
        std::unordered_map<int64_t, ComposerClientWriter*> displayWriters;

        ASSERT_FALSE(mAllDisplays.empty());
        for (const auto& display : mAllDisplays) {
            ASSERT_TRUE(
                    mComposerClient->setPowerMode(display.getDisplayId(), PowerMode::OFF).isOk());

            const auto errors = mDisplayProperties.at(display.getDisplayId()).reader.takeErrors();
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId())
                                .reader.takeChangedCompositionTypes(display.getDisplayId())
                                .empty());
            displayWriters[display.getDisplayId()] =
                    &mDisplayProperties.at(display.getDisplayId()).writer;
        }

        ASSERT_TRUE(mComposerClient->tearDown(displayWriters));
        mComposerClient.reset();
    }

    void setUpDisplayProperties() {
        for (const auto& display : mAllDisplays) {
            auto props = ReadbackHelper::setupDisplayProperty(display, mComposerClient);
            mDisplayProperties.emplace(display.getDisplayId(), std::move(props));
        }
    }

    // Gets and Updates the dataspace and check if readback is supported given the default pixel
    // format and the current dataspace. Dataspace can get updated after calls to
    // ComposerClientWrapper::setColorMode so it's essential to get the latest dataspace.
    std::pair<common::Dataspace, bool> GetDataspaceAndIfReadBackSupported(int64_t displayId) {
        auto [status, readBackBufferAttributes] =
                mComposerClient->getReadbackBufferAttributes(displayId);
        if (status.isOk()) {
            auto dataspace = readBackBufferAttributes.dataspace;
            mDisplayProperties.at(displayId).dataspace = dataspace;

            // We are making an assumption that Pixel Format never changes, so assert for this
            // assumption. If this is not the case on any display, then we should stop caching it.
            // The cached format is used in initializing TestRenderEngine.
            assertPixelFormatConsistency(displayId, readBackBufferAttributes.format);

            return std::make_pair(std::move(dataspace),
                                  ReadbackHelper::readbackSupported(
                                          mDisplayProperties.at(displayId).pixelFormat, dataspace));
        }
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
        return {common::Dataspace::UNKNOWN, false};
    }

    int64_t getInvalidDisplayId() const { return mComposerClient->getInvalidDisplayId(); }

    void assertServiceSpecificError(const ScopedAStatus& status, int32_t serviceSpecificError) {
        ASSERT_EQ(status.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(status.getServiceSpecificError(), serviceSpecificError);
    }

    void assertPixelFormatConsistency(int64_t displayId, common::PixelFormat currentPixelFormat) {
        ASSERT_EQ(mDisplayProperties.at(displayId).pixelFormat, currentPixelFormat);
    }

    std::pair<bool, ::android::sp<::android::GraphicBuffer>> allocateBuffer(
            const DisplayWrapper& display, uint32_t usage) {
        const auto width = static_cast<uint32_t>(display.getDisplayWidth());
        const auto height = static_cast<uint32_t>(display.getDisplayHeight());

        const auto& graphicBuffer = ::android::sp<::android::GraphicBuffer>::make(
                width, height, ::android::PIXEL_FORMAT_RGBA_8888,
                /*layerCount*/ 1u, usage, "VtsHalGraphicsComposer3_ReadbackTest");

        if (graphicBuffer && ::android::OK == graphicBuffer->initCheck()) {
            return {true, graphicBuffer};
        }
        return {false, graphicBuffer};
    }

    void writeLayers(const std::vector<std::shared_ptr<TestLayer>>& layers, int64_t displayId) {
        for (const auto& layer : layers) {
            layer->write(mDisplayProperties.at(displayId).writer);
        }
        execute(displayId);
    }

    void execute(int64_t displayId) {
        auto commands = mDisplayProperties.at(displayId).writer.takePendingCommands();
        if (commands.empty()) {
            return;
        }

        auto [status, results] = mComposerClient->executeCommands(commands);
        ASSERT_TRUE(status.isOk()) << "executeCommands failed " << status.getDescription();

        mDisplayProperties.at(displayId).reader.parse(std::move(results));
    }

    std::shared_ptr<ComposerClientWrapper> mComposerClient;
    std::vector<DisplayWrapper> mAllDisplays;
    std::unordered_map<int64_t, DisplayProperties> mDisplayProperties;

    static constexpr uint32_t kClientTargetSlotCount = 64;
};

class GraphicsCompositionTest : public GraphicsCompositionTestBase,
                                public testing::WithParamInterface<std::string> {
  public:
    void SetUp() override { SetUpBase(GetParam()); }
};

TEST_P(GraphicsCompositionTest, SingleSolidColorLayer) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;
        for (ColorMode mode : testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            auto layer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            common::Rect coloredSquare(
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setColor(BLUE);
            layer->setDisplayFrame(coloredSquare);
            layer->setZOrder(10);

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            // expected color for each pixel
            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), coloredSquare,
                                           BLUE);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            // if hwc cannot handle and asks for composition change, just skip the test on this
            // display->
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBuffer) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;
        for (ColorMode mode : testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight() / 4}, RED);
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, display.getDisplayHeight() / 4, display.getDisplayWidth(),
                     display.getDisplayHeight() / 2},
                    GREEN);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, display.getDisplayHeight() / 2,
                                            display.getDisplayWidth(), display.getDisplayHeight()},
                                           BLUE);

            auto layer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                    common::PixelFormat::RGBA_8888,
                    mDisplayProperties.at(display.getDisplayId()).writer);
            layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setZOrder(10);
            layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));
            ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());

            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBufferNoEffect) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;
        for (ColorMode mode : testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            auto& writer = mDisplayProperties.at(display.getDisplayId()).writer;
            auto layer = std::make_shared<TestColorLayer>(mComposerClient, display.getDisplayId(),
                                                          writer);
            common::Rect coloredSquare(
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setColor(BLUE);
            layer->setDisplayFrame(coloredSquare);
            layer->setZOrder(10);
            layer->write(mDisplayProperties.at(display.getDisplayId()).writer);

            // This following buffer call should have no effect
            const auto usage = static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                               static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN);
            const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(display, usage);
            ASSERT_TRUE(graphicBufferStatus);
            const auto& buffer = graphicBuffer->handle;
            mDisplayProperties.at(display.getDisplayId())
                    .writer.setLayerBuffer(display.getDisplayId(), layer->getLayer(), /*slot*/ 0,
                                           buffer,
                                           /*acquireFence*/ -1);

            // expected color for each pixel
            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), coloredSquare,
                                           BLUE);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());

            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto [dataspace, readbackSupported] =
                GetDataspaceAndIfReadBackSupported(display.getDisplayId());
        if (!readbackSupported) {
            continue;
        }

        ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                      display.getDisplayWidth(), display.getDisplayHeight(),
                                      mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                      dataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
    }
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer_BadDisplay) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto [_, readbackSupported] = GetDataspaceAndIfReadBackSupported(display.getDisplayId());
        if (!readbackSupported) {
            continue;
        }

        const auto usage = static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                           static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN);
        const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(display, usage);
        ASSERT_TRUE(graphicBufferStatus);
        const auto& bufferHandle = graphicBuffer->handle;
        ::ndk::ScopedFileDescriptor fence = ::ndk::ScopedFileDescriptor(-1);

        const auto status =
                mComposerClient->setReadbackBuffer(getInvalidDisplayId(), bufferHandle, fence);

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
    }
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer_BadParameter) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto [_, readbackSupported] = GetDataspaceAndIfReadBackSupported(display.getDisplayId());
        if (!readbackSupported) {
            continue;
        }

        const native_handle_t bufferHandle{};
        ndk::ScopedFileDescriptor releaseFence = ndk::ScopedFileDescriptor(-1);
        const auto status = mComposerClient->setReadbackBuffer(display.getDisplayId(),
                                                               &bufferHandle, releaseFence);

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
    }
}

TEST_P(GraphicsCompositionTest, GetReadbackBufferFenceInactive) {
    for (const DisplayWrapper display : mAllDisplays) {
        auto [_, readbackSupported] = GetDataspaceAndIfReadBackSupported(display.getDisplayId());
        if (!readbackSupported) {
            continue;
        }
        const auto& [status, releaseFence] =
                mComposerClient->getReadbackBufferFence(display.getDisplayId());

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
        EXPECT_EQ(-1, releaseFence.get());
    }
}

TEST_P(GraphicsCompositionTest, ClientComposition) {
    for (const DisplayWrapper display : mAllDisplays) {
        EXPECT_TRUE(
                mComposerClient
                        ->setClientTargetSlotCount(display.getDisplayId(), kClientTargetSlotCount)
                        .isOk());

        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight() / 4}, RED);
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, display.getDisplayHeight() / 4, display.getDisplayWidth(),
                     display.getDisplayHeight() / 2},
                    GREEN);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, display.getDisplayHeight() / 2,
                                            display.getDisplayWidth(), display.getDisplayHeight()},
                                           BLUE);

            auto layer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                    PixelFormat::RGBA_8888, mDisplayProperties.at(display.getDisplayId()).writer);
            layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setZOrder(10);
            layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());

            auto changedCompositionTypes =
                    mDisplayProperties.at(display.getDisplayId())
                            .reader.takeChangedCompositionTypes(display.getDisplayId());
            if (!changedCompositionTypes.empty()) {
                ASSERT_EQ(1, changedCompositionTypes.size());
                ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);

                PixelFormat clientFormat = PixelFormat::RGBA_8888;
                auto clientUsage = static_cast<uint32_t>(
                        static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN) |
                        static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                        static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
                Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
                common::Rect damage{0, 0, display.getDisplayWidth(), display.getDisplayHeight()};

                // create client target buffer
                const auto& [graphicBufferStatus, graphicBuffer] =
                        allocateBuffer(display, clientUsage);
                ASSERT_TRUE(graphicBufferStatus);
                const auto& buffer = graphicBuffer->handle;
                void* clientBufData;
                const auto stride = static_cast<uint32_t>(graphicBuffer->stride);
                int bytesPerPixel = -1;
                int bytesPerStride = -1;
                graphicBuffer->lock(clientUsage, layer->getAccessRegion(), &clientBufData,
                                    &bytesPerPixel, &bytesPerStride);

                ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(
                        layer->getWidth(), layer->getHeight(), stride, bytesPerPixel, clientBufData,
                        clientFormat, expectedColors));
                int32_t clientFence;
                const auto unlockStatus = graphicBuffer->unlockAsync(&clientFence);
                ASSERT_EQ(::android::OK, unlockStatus);
                mDisplayProperties.at(display.getDisplayId())
                        .writer.setClientTarget(display.getDisplayId(), /*slot*/ 0, buffer,
                                                clientFence, clientDataspace,
                                                std::vector<common::Rect>(1, damage), 1.f);
                layer->setToClientComposition(mDisplayProperties.at(display.getDisplayId()).writer);
                mDisplayProperties.at(display.getDisplayId())
                        .writer.validateDisplay(display.getDisplayId(),
                                                ComposerClientWriter::kNoTimestamp,
                                                ComposerClientWrapper::kNoFrameIntervalNs);
                execute(display.getDisplayId());
                changedCompositionTypes =
                        mDisplayProperties.at(display.getDisplayId())
                                .reader.takeChangedCompositionTypes(display.getDisplayId());
                ASSERT_TRUE(changedCompositionTypes.empty());
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());

            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        }
    }
}

void generateLuts(Luts* luts, LutProperties::Dimension dimension, int32_t size,
                  LutProperties::SamplingKey key) {
    size_t bufferSize = dimension == LutProperties::Dimension::ONE_D
                                ? static_cast<size_t>(size) * sizeof(float)
                                : static_cast<size_t>(size * size * size) * sizeof(float);
    int32_t fd = ashmem_create_region("lut_shared_mem", bufferSize);
    void* ptr = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    std::vector<float> buffers(static_cast<size_t>(size), 0.5f);
    memcpy(ptr, buffers.data(), bufferSize);
    munmap(ptr, bufferSize);
    luts->pfd = ndk::ScopedFileDescriptor(fd);
    luts->offsets = std::vector<int32_t>{0};
    luts->lutProperties = {LutProperties{dimension, size, {key}}};
}

// @VsrTest = 4.4-016
TEST_P(GraphicsCompositionTest, Luts) {
    const auto& [status, properties] = mComposerClient->getOverlaySupport();
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        ALOGI("getOverlaySupport is not supported");
        GTEST_SKIP();
    }

    if (!properties.lutProperties) {
        ALOGI("lutProperties are not supported");
        GTEST_SKIP();
    }

    bool supportsHlg = false;
    for (const auto& i : properties.combinations) {
        bool supportsBt2020Gamut =
                std::any_of(i.standards.cbegin(), i.standards.cend(), [](const auto& standard) {
                    return standard == common::Dataspace::STANDARD_BT2020;
                });
        bool supportsHlgTransfer = std::any_of(
                i.transfers.cbegin(), i.transfers.cend(),
                [](const auto& transfer) { return transfer == common::Dataspace::TRANSFER_HLG; });
        bool supportsFullRange = std::any_of(
                i.ranges.cbegin(), i.ranges.cend(),
                [](const auto& range) { return range == common::Dataspace::RANGE_FULL; });
        supportsHlg = supportsBt2020Gamut && supportsHlgTransfer && supportsFullRange;
        if (supportsHlg) {
            break;
        }
    }

    for (const DisplayWrapper display : mAllDisplays) {
        ASSERT_TRUE(
                mComposerClient
                        ->setClientTargetSlotCount(display.getDisplayId(), kClientTargetSlotCount)
                        .isOk());
        auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;

        for (const auto& lutProperties : *properties.lutProperties) {
            if (!lutProperties) {
                continue;
            }
            auto& l = *lutProperties;

            for (const auto& key : l.samplingKeys) {
                for (ColorMode mode : testColorModes) {
                    EXPECT_TRUE(mComposerClient
                                        ->setColorMode(display.getDisplayId(), mode,
                                                       RenderIntent::COLORIMETRIC)
                                        .isOk());

                    auto [dataspace, readbackSupported] =
                            GetDataspaceAndIfReadBackSupported(display.getDisplayId());
                    if (!readbackSupported) {
                        continue;
                    }

                    common::Rect coloredSquare(
                            {0, 0, display.getDisplayWidth(), display.getDisplayHeight()});

                    // expected color for each pixel
                    std::vector<Color> expectedColors(static_cast<size_t>(
                            display.getDisplayWidth() * display.getDisplayHeight()));
                    ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                                   coloredSquare, WHITE);

                    auto layer = std::make_shared<TestBufferLayer>(
                            mComposerClient,
                            *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                            display.getDisplayId(), display.getDisplayWidth(),
                            display.getDisplayHeight(), PixelFormat::RGBA_8888,
                            mDisplayProperties.at(display.getDisplayId()).writer);
                    layer->setDisplayFrame(coloredSquare);
                    layer->setZOrder(10);
                    // Fallback to sRGB support if the device doesn't support HLG
                    // This is to accommodate nascent devices that support LUTs but only for HDR
                    // formats, without compromising test coverage of the LUT feature altogether
                    layer->setDataspace(supportsHlg ? Dataspace::BT2020_HLG : Dataspace::SRGB);

                    Luts luts;
                    generateLuts(&luts, l.dimension, l.size, key);
                    layer->setLuts(std::move(luts));

                    ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

                    std::vector<std::shared_ptr<TestLayer>> layers = {layer};

                    ReadbackBuffer readbackBuffer(
                            display.getDisplayId(), mComposerClient, display.getDisplayWidth(),
                            display.getDisplayHeight(),
                            mDisplayProperties.at(display.getDisplayId()).pixelFormat, dataspace);
                    ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

                    writeLayers(layers, display.getDisplayId());
                    ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId())
                                        .reader.takeErrors()
                                        .empty());
                    mDisplayProperties.at(display.getDisplayId())
                            .writer.validateDisplay(display.getDisplayId(),
                                                    ComposerClientWriter::kNoTimestamp,
                                                    ComposerClientWrapper::kNoFrameIntervalNs);
                    execute(display.getDisplayId());

                    // We should be guaranteed to use DPU composition here
                    ASSERT_TRUE(supportsHlg);
                    auto changedCompositionTypes =
                            mDisplayProperties.at(display.getDisplayId())
                                    .reader.takeChangedCompositionTypes(display.getDisplayId());
                    ASSERT_TRUE(changedCompositionTypes.empty());

                    mDisplayProperties.at(display.getDisplayId())
                            .writer.presentDisplay(display.getDisplayId());
                    execute(display.getDisplayId());
                    ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId())
                                        .reader.takeErrors()
                                        .empty());

                    ReadbackHelper::fillColorsArea(
                            expectedColors, display.getDisplayWidth(), coloredSquare,
                            {188.f / 255.f, 188.f / 255.f, 188.f / 255.f, 1.0f});

                    ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
                    auto& testRenderEngine =
                            mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
                    testRenderEngine->setRenderLayers(layers);
                    ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
                    ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
                    mComposerClient->destroyLayer(
                            display.getDisplayId(), layer->getLayer(),
                            &mDisplayProperties.at(display.getDisplayId()).writer);
                }
            }
        }
    }
}

TEST_P(GraphicsCompositionTest, MixedColorSpaces) {
    const auto& [status, properties] = mComposerClient->getOverlaySupport();
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        ALOGI("getOverlaySupport is not supported");
        GTEST_SKIP();
    }

    if (properties.supportMixedColorSpaces == false) {
        ALOGI("supportMixedColorSpaces is not supported");
        GTEST_SKIP();
    }

    for (const DisplayWrapper display : mAllDisplays) {
        ASSERT_TRUE(
                mComposerClient
                        ->setClientTargetSlotCount(display.getDisplayId(), kClientTargetSlotCount)
                        .isOk());

        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            // sRGB layer
            auto srgbLayer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(),
                    display.getDisplayHeight() / 2, PixelFormat::RGBA_8888,
                    mDisplayProperties.at(display.getDisplayId()).writer);
            std::vector<Color> sRgbDeviceColors(srgbLayer->getWidth() * srgbLayer->getHeight());
            ReadbackHelper::fillColorsArea(sRgbDeviceColors, display.getDisplayWidth(),
                                           {0, 0, static_cast<int32_t>(srgbLayer->getWidth()),
                                            static_cast<int32_t>(srgbLayer->getHeight())},
                                           GREEN);
            srgbLayer->setDisplayFrame({0, 0, static_cast<int32_t>(srgbLayer->getWidth()),
                                        static_cast<int32_t>(srgbLayer->getHeight())});
            srgbLayer->setZOrder(10);
            srgbLayer->setDataspace(Dataspace::SRGB);
            ASSERT_NO_FATAL_FAILURE(srgbLayer->setBuffer(sRgbDeviceColors));

            // display P3 layer
            auto displayP3Layer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(),
                    display.getDisplayHeight() / 2, PixelFormat::RGBA_8888,
                    mDisplayProperties.at(display.getDisplayId()).writer);
            std::vector<Color> displayP3DeviceColors(
                    static_cast<size_t>(displayP3Layer->getWidth() * displayP3Layer->getHeight()));
            ReadbackHelper::fillColorsArea(displayP3DeviceColors, display.getDisplayWidth(),
                                           {0, 0, static_cast<int32_t>(displayP3Layer->getWidth()),
                                            static_cast<int32_t>(displayP3Layer->getHeight())},
                                           RED);
            displayP3Layer->setDisplayFrame({0, display.getDisplayHeight() / 2,
                                             display.getDisplayWidth(),
                                             display.getDisplayHeight()});
            displayP3Layer->setZOrder(10);
            displayP3Layer->setDataspace(Dataspace::DISPLAY_P3);
            ASSERT_NO_FATAL_FAILURE(displayP3Layer->setBuffer(displayP3DeviceColors));

            writeLayers({srgbLayer, displayP3Layer}, display.getDisplayId());

            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());

            auto changedCompositionTypes =
                    mDisplayProperties.at(display.getDisplayId())
                            .reader.takeChangedCompositionTypes(display.getDisplayId());
            ASSERT_TRUE(changedCompositionTypes.empty());

            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            changedCompositionTypes =
                    mDisplayProperties.at(display.getDisplayId())
                            .reader.takeChangedCompositionTypes(display.getDisplayId());
            ASSERT_TRUE(changedCompositionTypes.empty());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            mComposerClient->destroyLayer(display.getDisplayId(), srgbLayer->getLayer(),
                    &mDisplayProperties.at(display.getDisplayId()).writer);
            mComposerClient->destroyLayer(display.getDisplayId(), displayP3Layer->getLayer(),
                    &mDisplayProperties.at(display.getDisplayId()).writer);
        }
    }
}

TEST_P(GraphicsCompositionTest, DeviceAndClientComposition) {
    for (const DisplayWrapper display : mAllDisplays) {
        ASSERT_TRUE(
                mComposerClient
                        ->setClientTargetSlotCount(display.getDisplayId(), kClientTargetSlotCount)
                        .isOk());

        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight() / 2}, GREEN);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, display.getDisplayHeight() / 2,
                                            display.getDisplayWidth(), display.getDisplayHeight()},
                                           RED);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            auto deviceLayer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(),
                    display.getDisplayHeight() / 2, PixelFormat::RGBA_8888,
                    mDisplayProperties.at(display.getDisplayId()).writer);
            std::vector<Color> deviceColors(deviceLayer->getWidth() * deviceLayer->getHeight());
            ReadbackHelper::fillColorsArea(deviceColors,
                                           static_cast<int32_t>(deviceLayer->getWidth()),
                                           {0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                            static_cast<int32_t>(deviceLayer->getHeight())},
                                           GREEN);
            deviceLayer->setDisplayFrame({0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                          static_cast<int32_t>(deviceLayer->getHeight())});
            deviceLayer->setZOrder(10);
            deviceLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));
            ASSERT_NO_FATAL_FAILURE(deviceLayer->setBuffer(deviceColors));
            deviceLayer->write(mDisplayProperties.at(display.getDisplayId()).writer);

            PixelFormat clientFormat = PixelFormat::RGBA_8888;
            auto clientUsage = static_cast<uint32_t>(
                    static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
            Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
            int32_t clientWidth = display.getDisplayWidth();
            int32_t clientHeight = display.getDisplayHeight() / 2;

            auto clientLayer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), clientWidth, clientHeight, PixelFormat::RGBA_FP16,
                    mDisplayProperties.at(display.getDisplayId()).writer, Composition::DEVICE);
            common::Rect clientFrame = {0, display.getDisplayHeight() / 2,
                                        display.getDisplayWidth(), display.getDisplayHeight()};
            clientLayer->setDisplayFrame(clientFrame);
            clientLayer->setZOrder(0);
            clientLayer->write(mDisplayProperties.at(display.getDisplayId()).writer);
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());

            auto changedCompositionTypes =
                    mDisplayProperties.at(display.getDisplayId())
                            .reader.takeChangedCompositionTypes(display.getDisplayId());
            if (changedCompositionTypes.size() != 1) {
                continue;
            }
            // create client target buffer
            ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);
            const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(display, clientUsage);
            ASSERT_TRUE(graphicBufferStatus);
            const auto& buffer = graphicBuffer->handle;

            void* clientBufData;
            int bytesPerPixel = -1;
            int bytesPerStride = -1;
            graphicBuffer->lock(clientUsage,
                                {0, 0, display.getDisplayWidth(), display.getDisplayHeight()},
                                &clientBufData, &bytesPerPixel, &bytesPerStride);

            std::vector<Color> clientColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(clientColors, display.getDisplayWidth(), clientFrame,
                                           RED);
            ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(
                    static_cast<uint32_t>(display.getDisplayWidth()),
                    static_cast<uint32_t>(display.getDisplayHeight()), graphicBuffer->getStride(),
                    bytesPerPixel, clientBufData, clientFormat, clientColors));
            int32_t clientFence;
            const auto unlockStatus = graphicBuffer->unlockAsync(&clientFence);
            ASSERT_EQ(::android::OK, unlockStatus);
            mDisplayProperties.at(display.getDisplayId())
                    .writer.setClientTarget(display.getDisplayId(), /*slot*/ 0, buffer, clientFence,
                                            clientDataspace,
                                            std::vector<common::Rect>(1, clientFrame), 1.f);
            clientLayer->setToClientComposition(
                    mDisplayProperties.at(display.getDisplayId()).writer);
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            changedCompositionTypes =
                    mDisplayProperties.at(display.getDisplayId())
                            .reader.takeChangedCompositionTypes(display.getDisplayId());
            ASSERT_TRUE(changedCompositionTypes.empty());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerDamage) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            common::Rect redRect = {0, 0, display.getDisplayWidth() / 4,
                                    display.getDisplayHeight() / 4};

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), redRect, RED);

            auto layer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                    PixelFormat::RGBA_8888, mDisplayProperties.at(display.getDisplayId()).writer);
            layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setZOrder(10);
            layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));
            ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

            // update surface damage and recheck
            redRect = {display.getDisplayWidth() / 4, display.getDisplayHeight() / 4,
                       display.getDisplayWidth() / 2, display.getDisplayHeight() / 2};
            ReadbackHelper::clearColors(expectedColors, display.getDisplayWidth(),
                                        display.getDisplayHeight(), display.getDisplayWidth());
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), redRect, RED);

            ASSERT_NO_FATAL_FAILURE(layer->fillBuffer(expectedColors));
            layer->setSurfaceDamage(std::vector<common::Rect>(
                    1, {0, 0, display.getDisplayWidth() / 2, display.getDisplayWidth() / 2}));

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId())
                                .reader.takeChangedCompositionTypes(display.getDisplayId())
                                .empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerPlaneAlpha) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            auto layer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            layer->setColor(RED);
            layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setZOrder(10);
            layer->setAlpha(0);
            layer->setBlendMode(BlendMode::PREMULTIPLIED);

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerSourceCrop) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight() / 4}, RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, display.getDisplayHeight() / 2,
                                            display.getDisplayWidth(), display.getDisplayHeight()},
                                           BLUE);

            auto layer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                    PixelFormat::RGBA_8888, mDisplayProperties.at(display.getDisplayId()).writer);
            layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            layer->setZOrder(10);
            layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));
            layer->setSourceCrop({0, static_cast<float>(display.getDisplayHeight() / 2),
                                  static_cast<float>(display.getDisplayWidth()),
                                  static_cast<float>(display.getDisplayHeight())});
            ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

            std::vector<std::shared_ptr<TestLayer>> layers = {layer};

            // update expected colors to match crop
            ReadbackHelper::fillColorsArea(
                    expectedColors, display.getDisplayWidth(),
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight()}, BLUE);
            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerZOrder) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            common::Rect redRect = {0, 0, display.getDisplayWidth(),
                                    display.getDisplayHeight() / 2};
            common::Rect blueRect = {0, display.getDisplayHeight() / 4, display.getDisplayWidth(),
                                     display.getDisplayHeight()};
            auto redLayer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            redLayer->setColor(RED);
            redLayer->setDisplayFrame(redRect);

            auto blueLayer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            blueLayer->setColor(BLUE);
            blueLayer->setDisplayFrame(blueRect);
            blueLayer->setZOrder(5);

            std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, blueLayer};
            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            // red in front of blue
            redLayer->setZOrder(10);

            // fill blue first so that red will overwrite on overlap
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), blueRect,
                                           BLUE);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), redRect, RED);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

            redLayer->setZOrder(1);
            ReadbackHelper::clearColors(expectedColors, display.getDisplayWidth(),
                                        display.getDisplayHeight(), display.getDisplayWidth());
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), redRect, RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), blueRect,
                                           BLUE);

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId())
                                .reader.takeChangedCompositionTypes(display.getDisplayId())
                                .empty());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBrightnessDims) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            const common::Rect redRect = {0, 0, display.getDisplayWidth(),
                                          display.getDisplayHeight() / 2};
            const common::Rect dimmerRedRect = {0, display.getDisplayHeight() / 2,
                                                display.getDisplayWidth(),
                                                display.getDisplayHeight()};

            static constexpr float kMaxBrightnessNits = 300.f;

            const auto redLayer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            redLayer->setColor(RED);
            redLayer->setDisplayFrame(redRect);
            redLayer->setWhitePointNits(kMaxBrightnessNits);
            redLayer->setBrightness(1.f);

            const auto dimmerRedLayer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            dimmerRedLayer->setColor(RED);
            dimmerRedLayer->setDisplayFrame(dimmerRedRect);
            // Intentionally use a small dimming ratio as some implementations may be more likely
            // to kick into GPU composition to apply dithering when the dimming ratio is high.
            static constexpr float kDimmingRatio = 0.9f;
            dimmerRedLayer->setWhitePointNits(kMaxBrightnessNits * kDimmingRatio);
            dimmerRedLayer->setBrightness(kDimmingRatio);

            const std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, dimmerRedLayer};
            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), redRect, RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(), dimmerRedRect,
                                           DIM_RED);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                ALOGI(" Readback verification not supported for GPU composition for color mode %d",
                      mode);
                continue;
            }
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

class GraphicsBlendModeCompositionTest
    : public GraphicsCompositionTestBase,
      public testing::WithParamInterface<std::tuple<std::string, std::string>> {
  public:
    void SetUp() override {
        SetUpBase(std::get<0>(GetParam()));
        for (const DisplayWrapper& display : mAllDisplays) {
            // TODO(b/219590743) we should remove the below SRGB color mode
            // once we have the BlendMode test fix for all the versions of the ColorMode
            auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;
            testColorModes.erase(
                    std::remove_if(testColorModes.begin(), testColorModes.end(),
                                   [](ColorMode mode) { return mode != ColorMode::SRGB; }),
                    testColorModes.end());
        }
    }

    void setBackgroundColor(int64_t displayId, Color color) {
        mDisplayGfx[displayId].backgroundColor = color;
    }

    void setTopLayerColor(int64_t displayId, Color color) {
        mDisplayGfx[displayId].topLayerColor = color;
    }

    void setUpLayers(const DisplayWrapper& display, BlendMode blendMode) {
        auto& layers = mDisplayGfx[display.getDisplayId()].layers;
        layers.clear();

        std::vector<Color> topLayerPixelColors(
                static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
        ReadbackHelper::fillColorsArea(
                topLayerPixelColors, display.getDisplayWidth(),
                {0, 0, display.getDisplayWidth(), display.getDisplayHeight()},
                mDisplayGfx[display.getDisplayId()].topLayerColor);

        auto backgroundLayer = std::make_shared<TestColorLayer>(
                mComposerClient, display.getDisplayId(),
                mDisplayProperties.at(display.getDisplayId()).writer);
        backgroundLayer->setDisplayFrame(
                {0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
        backgroundLayer->setZOrder(0);
        backgroundLayer->setColor(mDisplayGfx[display.getDisplayId()].backgroundColor);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                PixelFormat::RGBA_8888, mDisplayProperties.at(display.getDisplayId()).writer);
        layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(Dataspace::UNKNOWN);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(topLayerPixelColors));

        layer->setBlendMode(blendMode);
        layer->setAlpha(std::stof(std::get<1>(GetParam())));

        layers.push_back(backgroundLayer);
        layers.push_back(layer);
    }

    void setExpectedColors(const DisplayWrapper& display, std::vector<Color>& expectedColors) {
        auto& layers = mDisplayGfx[display.getDisplayId()].layers;
        ASSERT_EQ(2, layers.size());
        ReadbackHelper::clearColors(expectedColors, display.getDisplayWidth(),
                                    display.getDisplayHeight(), display.getDisplayWidth());

        auto layer = layers[1];
        BlendMode blendMode = layer->getBlendMode();
        auto& topLayerColor = mDisplayGfx[display.getDisplayId()].topLayerColor;
        auto& backgroundColor = mDisplayGfx[display.getDisplayId()].backgroundColor;
        float alpha = topLayerColor.a * layer->getAlpha();
        if (blendMode == BlendMode::NONE) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r = topLayerColor.r * layer->getAlpha();
                expectedColor.g = topLayerColor.g * layer->getAlpha();
                expectedColor.b = topLayerColor.b * layer->getAlpha();
                expectedColor.a = alpha;
            }
        } else if (blendMode == BlendMode::PREMULTIPLIED) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r =
                        topLayerColor.r * layer->getAlpha() + backgroundColor.r * (1.0f - alpha);
                expectedColor.g =
                        topLayerColor.g * layer->getAlpha() + backgroundColor.g * (1.0f - alpha);
                expectedColor.b =
                        topLayerColor.b * layer->getAlpha() + backgroundColor.b * (1.0f - alpha);
                expectedColor.a = alpha + backgroundColor.a * (1.0f - alpha);
            }
        } else if (blendMode == BlendMode::COVERAGE) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r = topLayerColor.r * alpha + backgroundColor.r * (1.0f - alpha);
                expectedColor.g = topLayerColor.g * alpha + backgroundColor.g * (1.0f - alpha);
                expectedColor.b = topLayerColor.b * alpha + backgroundColor.b * (1.0f - alpha);
                expectedColor.a = topLayerColor.a * alpha + backgroundColor.a * (1.0f - alpha);
            }
        }
    }

  protected:
    struct DisplayGraphics {
        std::vector<std::shared_ptr<TestLayer>> layers;
        Color backgroundColor = BLACK;
        Color topLayerColor = RED;
    };

    std::unordered_map<int64_t, struct DisplayGraphics> mDisplayGfx;
};

TEST_P(GraphicsBlendModeCompositionTest, None) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            setBackgroundColor(display.getDisplayId(), BLACK);
            setTopLayerColor(display.getDisplayId(), TRANSLUCENT_RED);
            setUpLayers(display, BlendMode::NONE);
            setExpectedColors(display, expectedColors);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Coverage) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            setBackgroundColor(display.getDisplayId(), BLACK);
            setTopLayerColor(display.getDisplayId(), TRANSLUCENT_RED);

            setUpLayers(display, BlendMode::COVERAGE);
            setExpectedColors(display, expectedColors);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Premultiplied) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));

            setBackgroundColor(display.getDisplayId(), BLACK);
            setTopLayerColor(display.getDisplayId(), TRANSLUCENT_RED);
            setUpLayers(display, BlendMode::PREMULTIPLIED);
            setExpectedColors(display, expectedColors);

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

class GraphicsTransformCompositionTest : public GraphicsCompositionTest {
  protected:
    void SetUp() override {
        GraphicsCompositionTest::SetUp();

        for (const DisplayWrapper& display : mAllDisplays) {
            auto backgroundLayer = std::make_shared<TestColorLayer>(
                    mComposerClient, display.getDisplayId(),
                    mDisplayProperties.at(display.getDisplayId()).writer);
            backgroundLayer->setColor({0.0f, 0.0f, 0.0f, 0.0f});
            backgroundLayer->setDisplayFrame(
                    {0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
            backgroundLayer->setZOrder(0);

            int& sideLength = mDisplayGfx[display.getDisplayId()].sideLength;
            sideLength = display.getDisplayWidth() < display.getDisplayHeight()
                                 ? display.getDisplayWidth()
                                 : display.getDisplayHeight();
            common::Rect redRect = {0, 0, sideLength / 2, sideLength / 2};
            common::Rect blueRect = {sideLength / 2, sideLength / 2, sideLength, sideLength};

            auto& bufferLayer = mDisplayGfx[display.getDisplayId()].bufferLayer;
            bufferLayer = std::make_shared<TestBufferLayer>(
                    mComposerClient,
                    *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                    display.getDisplayId(), static_cast<uint32_t>(sideLength),
                    static_cast<uint32_t>(sideLength), PixelFormat::RGBA_8888,
                    mDisplayProperties.at(display.getDisplayId()).writer);
            bufferLayer->setDisplayFrame({0, 0, sideLength, sideLength});
            bufferLayer->setZOrder(10);

            std::vector<Color> baseColors(static_cast<size_t>(sideLength * sideLength));
            ReadbackHelper::fillColorsArea(baseColors, sideLength, redRect, RED);
            ReadbackHelper::fillColorsArea(baseColors, sideLength, blueRect, BLUE);
            ASSERT_NO_FATAL_FAILURE(bufferLayer->setBuffer(baseColors));
            mDisplayGfx[display.getDisplayId()].layers = {backgroundLayer, bufferLayer};
        }
    }

  protected:
    struct DisplayGraphics {
        std::shared_ptr<TestBufferLayer> bufferLayer;
        std::vector<std::shared_ptr<TestLayer>> layers;
        int sideLength;
    };

    std::unordered_map<int64_t, struct DisplayGraphics> mDisplayGfx;
};

TEST_P(GraphicsTransformCompositionTest, FLIP_H) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            auto status = mComposerClient->setColorMode(display.getDisplayId(), mode,
                                                        RenderIntent::COLORIMETRIC);

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                (status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED ||
                 status.getServiceSpecificError() == IComposerClient::EX_BAD_PARAMETER)) {
                ALOGI("ColorMode not supported on Display %" PRId64 " for ColorMode %d",
                      display.getDisplayId(), mode);
                continue;
            }

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
            auto& bufferLayer = mDisplayGfx[display.getDisplayId()].bufferLayer;
            bufferLayer->setTransform(Transform::FLIP_H);
            bufferLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            int& sideLength = mDisplayGfx[display.getDisplayId()].sideLength;
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {sideLength / 2, 0, sideLength, sideLength / 2}, RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, sideLength / 2, sideLength / 2, sideLength}, BLUE);

            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsTransformCompositionTest, FLIP_V) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            auto& bufferLayer = mDisplayGfx[display.getDisplayId()].bufferLayer;
            bufferLayer->setTransform(Transform::FLIP_V);
            bufferLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            int& sideLength = mDisplayGfx[display.getDisplayId()].sideLength;
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, sideLength / 2, sideLength / 2, sideLength}, RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {sideLength / 2, 0, sideLength, sideLength / 2}, BLUE);

            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

TEST_P(GraphicsTransformCompositionTest, ROT_180) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            ReadbackBuffer readbackBuffer(display.getDisplayId(), mComposerClient,
                                          display.getDisplayWidth(), display.getDisplayHeight(),
                                          mDisplayProperties.at(display.getDisplayId()).pixelFormat,
                                          dataspace);
            ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

            auto& bufferLayer = mDisplayGfx[display.getDisplayId()].bufferLayer;
            bufferLayer->setTransform(Transform::ROT_180);
            bufferLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode));

            std::vector<Color> expectedColors(
                    static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
            int& sideLength = mDisplayGfx[display.getDisplayId()].sideLength;
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {sideLength / 2, sideLength / 2, sideLength, sideLength},
                                           RED);
            ReadbackHelper::fillColorsArea(expectedColors, display.getDisplayWidth(),
                                           {0, 0, sideLength / 2, sideLength / 2}, BLUE);

            auto& layers = mDisplayGfx[display.getDisplayId()].layers;
            writeLayers(layers, display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.validateDisplay(display.getDisplayId(),
                                            ComposerClientWriter::kNoTimestamp,
                                            ComposerClientWrapper::kNoFrameIntervalNs);
            execute(display.getDisplayId());
            if (!mDisplayProperties.at(display.getDisplayId())
                         .reader.takeChangedCompositionTypes(display.getDisplayId())
                         .empty()) {
                continue;
            }
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
            mDisplayProperties.at(display.getDisplayId())
                    .writer.presentDisplay(display.getDisplayId());
            execute(display.getDisplayId());
            ASSERT_TRUE(mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

            ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
            auto& testRenderEngine = mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
            testRenderEngine->setRenderLayers(layers);
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
            ASSERT_NO_FATAL_FAILURE(testRenderEngine->checkColorBuffer(expectedColors));
        }
    }
}

class GraphicsColorManagementCompositionTest
    : public GraphicsCompositionTestBase,
      public testing::WithParamInterface<std::tuple<std::string, Dataspace, Dataspace, Dataspace>> {
  public:
    void SetUp() override {
        SetUpBase(std::get<0>(GetParam()));
        for (const DisplayWrapper& display : mAllDisplays) {
            // for some reason only sRGB reliably works
            auto& testColorModes = mDisplayProperties.at(display.getDisplayId()).testColorModes;
            testColorModes.erase(
                    std::remove_if(testColorModes.begin(), testColorModes.end(),
                                   [](ColorMode mode) { return mode != ColorMode::SRGB; }),
                    testColorModes.end());
            auto standard = std::get<1>(GetParam());
            auto transfer = std::get<2>(GetParam());
            auto range = std::get<3>(GetParam());

            mDisplayGfx[display.getDisplayId()].layerDataspace = static_cast<Dataspace>(
                    static_cast<int32_t>(standard) | static_cast<int32_t>(transfer) |
                    static_cast<int32_t>(range));
            ALOGD("Invoking test for dataspace: {%s, %s, %s}", toString(standard).c_str(),
                  toString(transfer).c_str(), toString(range).c_str());
        }
    }

    void makeLayer(const DisplayWrapper& display) {
        auto& layer = mDisplayGfx[display.getDisplayId()].layer;
        layer = std::make_shared<TestBufferLayer>(
                mComposerClient, *mDisplayProperties.at(display.getDisplayId()).testRenderEngine,
                display.getDisplayId(), display.getDisplayWidth(), display.getDisplayHeight(),
                common::PixelFormat::RGBA_8888,
                mDisplayProperties.at(display.getDisplayId()).writer);
        layer->setDisplayFrame({0, 0, display.getDisplayWidth(), display.getDisplayHeight()});
        layer->setZOrder(10);
        layer->setAlpha(1.f);
        layer->setDataspace(mDisplayGfx[display.getDisplayId()].layerDataspace);
    }

    void fillColor(const DisplayWrapper& display, Color color) {
        std::vector<Color> baseColors(
                static_cast<size_t>(display.getDisplayWidth() * display.getDisplayHeight()));
        ReadbackHelper::fillColorsArea(baseColors, display.getDisplayWidth(),
                                       common::Rect{.left = 0,
                                                    .top = 0,
                                                    .right = display.getDisplayWidth(),
                                                    .bottom = display.getDisplayHeight()},
                                       color);
        ASSERT_NO_FATAL_FAILURE(mDisplayGfx[display.getDisplayId()].layer->setBuffer(baseColors));
    }

    struct DisplayGraphics {
        Dataspace layerDataspace;
        std::shared_ptr<TestBufferLayer> layer;
    };
    std::unordered_map<int64_t, struct DisplayGraphics> mDisplayGfx;
};

// @VsrTest = 4.4-015
TEST_P(GraphicsColorManagementCompositionTest, ColorConversion) {
    for (const DisplayWrapper display : mAllDisplays) {
        for (ColorMode mode : mDisplayProperties.at(display.getDisplayId()).testColorModes) {
            EXPECT_TRUE(
                    mComposerClient
                            ->setColorMode(display.getDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

            auto [dataspace, readbackSupported] =
                    GetDataspaceAndIfReadBackSupported(display.getDisplayId());
            if (!readbackSupported) {
                continue;
            }

            auto& clientCompositionDisplaySettings =
                    mDisplayProperties.at(display.getDisplayId()).clientCompositionDisplaySettings;
            clientCompositionDisplaySettings.outputDataspace =
                    static_cast<::android::ui::Dataspace>(dataspace);
            mDisplayProperties.at(display.getDisplayId())
                    .testRenderEngine->setDisplaySettings(clientCompositionDisplaySettings);

            makeLayer(display);
            for (auto color : {LIGHT_RED, LIGHT_GREEN, LIGHT_BLUE}) {
                ALOGD("Testing color: %f, %f, %f, %f with color mode: %d", color.r, color.g,
                      color.b, color.a, mode);
                ReadbackBuffer readbackBuffer(
                        display.getDisplayId(), mComposerClient, display.getDisplayWidth(),
                        display.getDisplayHeight(),
                        mDisplayProperties.at(display.getDisplayId()).pixelFormat, dataspace);
                ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
                fillColor(display, color);
                auto& layer = mDisplayGfx[display.getDisplayId()].layer;
                writeLayers({layer}, display.getDisplayId());
                EXPECT_TRUE(mComposerClient->setPowerMode(display.getDisplayId(), PowerMode::ON)
                                    .isOk());

                ASSERT_TRUE(
                        mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
                mDisplayProperties.at(display.getDisplayId())
                        .writer.validateDisplay(display.getDisplayId(),
                                                ComposerClientWriter::kNoTimestamp,
                                                ComposerClientWrapper::kNoFrameIntervalNs);
                execute(display.getDisplayId());
                if (!mDisplayProperties.at(display.getDisplayId())
                             .reader.takeChangedCompositionTypes(display.getDisplayId())
                             .empty()) {
                    continue;
                }
                ASSERT_TRUE(
                        mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());
                mDisplayProperties.at(display.getDisplayId())
                        .writer.presentDisplay(display.getDisplayId());
                execute(display.getDisplayId());
                ASSERT_TRUE(
                        mDisplayProperties.at(display.getDisplayId()).reader.takeErrors().empty());

                auto& testRenderEngine =
                        mDisplayProperties.at(display.getDisplayId()).testRenderEngine;
                testRenderEngine->setRenderLayers({layer});
                ASSERT_NO_FATAL_FAILURE(testRenderEngine->drawLayers());
                ASSERT_NO_FATAL_FAILURE(
                        testRenderEngine->checkColorBuffer(readbackBuffer.getBuffer()));
            }
        }
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsCompositionTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsCompositionTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsBlendModeCompositionTest);
INSTANTIATE_TEST_SUITE_P(BlendMode, GraphicsBlendModeCompositionTest,
                         testing::Combine(testing::ValuesIn(::android::getAidlHalInstanceNames(
                                                  IComposer::descriptor)),
                                          testing::Values("0.2", "1.0")));

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsTransformCompositionTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsTransformCompositionTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsColorManagementCompositionTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, GraphicsColorManagementCompositionTest,
                         testing::Combine(testing::ValuesIn(::android::getAidlHalInstanceNames(
                                                  IComposer::descriptor)),
                                          // Only check sRGB, but verify that extended range
                                          // doesn't trigger any gamma shifts
                                          testing::Values(Dataspace::STANDARD_BT709),
                                          testing::Values(Dataspace::TRANSFER_SRGB),
                                          // Don't test limited range until we send YUV overlays
                                          testing::Values(Dataspace::RANGE_FULL,
                                                          Dataspace::RANGE_EXTENDED)));

}  // namespace
}  // namespace aidl::android::hardware::graphics::composer3::vts
