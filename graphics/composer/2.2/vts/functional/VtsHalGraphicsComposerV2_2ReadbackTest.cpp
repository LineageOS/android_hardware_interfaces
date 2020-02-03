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

#define LOG_TAG "graphics_composer_hidl_hal_readback_tests@2.2"

#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <composer-vts/2.2/ReadbackVts.h>
#include <composer-vts/2.2/RenderEngineVts.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {
namespace {

using android::GraphicBuffer;
using android::Rect;
using android::hardware::hidl_handle;
using common::V1_1::BufferUsage;
using common::V1_1::Dataspace;
using common::V1_1::PixelFormat;
using mapper::V2_1::IMapper;
using V2_1::Config;
using V2_1::Display;
using V2_1::vts::TestCommandReader;
using vts::Gralloc;

class GraphicsCompositionTestBase : public ::testing::Test {
  protected:
    using PowerMode = V2_1::IComposerClient::PowerMode;
    void SetUpBase(const std::string& service_name) {
        ASSERT_NO_FATAL_FAILURE(
                mComposer = std::make_unique<Composer>(IComposer::getService(service_name)));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());
        mComposerCallback = new V2_1::vts::GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();
        Config activeConfig;
        ASSERT_NO_FATAL_FAILURE(activeConfig = mComposerClient->getActiveConfig(mPrimaryDisplay));
        ASSERT_NO_FATAL_FAILURE(
            mDisplayWidth = mComposerClient->getDisplayAttribute(
                mPrimaryDisplay, activeConfig, IComposerClient::Attribute::WIDTH));
        ASSERT_NO_FATAL_FAILURE(
            mDisplayHeight = mComposerClient->getDisplayAttribute(
                mPrimaryDisplay, activeConfig, IComposerClient::Attribute::HEIGHT));

        setTestColorModes();

        // explicitly disable vsync
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setVsyncEnabled(mPrimaryDisplay, false));
        mComposerCallback->setVsyncAllowed(false);

        // set up command writer/reader and gralloc
        mWriter = std::make_shared<CommandWriterBase>(1024);
        mReader = std::make_unique<TestCommandReader>();
        mGralloc = std::make_shared<Gralloc>();

        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON));

        ASSERT_NO_FATAL_FAILURE(
                mTestRenderEngine = std::unique_ptr<TestRenderEngine>(new TestRenderEngine(
                        renderengine::RenderEngineCreationArgs::Builder()
                            .setPixelFormat(static_cast<int>(ui::PixelFormat::RGBA_8888))
                            .setImageCacheSize(TestRenderEngine::sMaxFrameBufferAcquireBuffers)
                            .setUseColorManagerment(true)
                            .setEnableProtectedContext(false)
                            .setPrecacheToneMapperShaderOnly(false)
                            .setContextPriority(renderengine::RenderEngine::ContextPriority::HIGH)
                            .build())));

        renderengine::DisplaySettings clientCompositionDisplay;
        clientCompositionDisplay.physicalDisplay = Rect(mDisplayWidth, mDisplayHeight);
        clientCompositionDisplay.clip = clientCompositionDisplay.physicalDisplay;
        clientCompositionDisplay.clearRegion = Region(clientCompositionDisplay.physicalDisplay);

        mTestRenderEngine->initGraphicBuffer(
                static_cast<uint32_t>(mDisplayWidth), static_cast<uint32_t>(mDisplayHeight), 1,
                static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN));
        mTestRenderEngine->setDisplaySettings(clientCompositionDisplay);
    }

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::OFF));
        EXPECT_EQ(0, mReader->mErrors.size());
        EXPECT_EQ(0, mReader->mCompositionChanges.size());
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    void clearCommandReaderState() {
        mReader->mCompositionChanges.clear();
        mReader->mErrors.clear();
    }

    void writeLayers(const std::vector<std::shared_ptr<TestLayer>>& layers) {
        for (auto layer : layers) {
            layer->write(mWriter);
        }
        execute();
    }

    void execute() {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->execute(mReader.get(), mWriter.get()));
    }

    std::unique_ptr<Composer> mComposer;
    std::shared_ptr<ComposerClient> mComposerClient;

    sp<V2_1::vts::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
    std::vector<ColorMode> mTestColorModes;
    std::shared_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<TestCommandReader> mReader;
    std::shared_ptr<Gralloc> mGralloc;
    std::unique_ptr<TestRenderEngine> mTestRenderEngine;

    bool mHasReadbackBuffer;
    PixelFormat mPixelFormat;
    Dataspace mDataspace;

    static constexpr uint32_t kClientTargetSlotCount = 64;

   private:
    Display waitForFirstDisplay() {
        while (true) {
            std::vector<Display> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                usleep(5 * 1000);
                continue;
            }
            return displays[0];
        }
    }

    void setTestColorModes() {
        mTestColorModes.clear();
        mComposerClient->getRaw()->getColorModes_2_2(mPrimaryDisplay, [&](const auto& tmpError,
                                                                          const auto& tmpModes) {
            ASSERT_EQ(Error::NONE, tmpError);
            for (ColorMode mode : tmpModes) {
                if (std::find(ReadbackHelper::colorModes.begin(), ReadbackHelper::colorModes.end(),
                              mode) != ReadbackHelper::colorModes.end()) {
                    mTestColorModes.push_back(mode);
                }
            }
        });
    }
};

class GraphicsCompositionTest : public GraphicsCompositionTestBase,
                                public testing::WithParamInterface<std::string> {
  public:
    void SetUp() override { SetUpBase(GetParam()); }
};

TEST_P(GraphicsCompositionTest, SingleSolidColorLayer) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        IComposerClient::Rect coloredSquare({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        // expected color for each pixel
        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        // if hwc cannot handle and asks for composition change,
        // just succeed the test
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBuffer) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight / 2},
                                       GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                                       mDisplayWidth, mDisplayHeight,
                                                       PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();

        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());

        mWriter->presentDisplay();
        execute();

        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBufferNoEffect) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        IComposerClient::Rect coloredSquare({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);
        layer->write(mWriter);

        // This following buffer call should have no effect
        uint64_t usage =
                static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN);
        const native_handle_t* bufferHandle =
                mGralloc->allocate(mDisplayWidth, mDisplayHeight, 1, PixelFormat::RGBA_8888, usage);
        mWriter->setLayerBuffer(0, bufferHandle, -1);

        // expected color for each pixel
        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mWriter->validateDisplay();
        execute();

        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, ClientComposition) {
    ASSERT_NO_FATAL_FAILURE(
            mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kClientTargetSlotCount));

    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight / 2},
                                       GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                                       mDisplayWidth, mDisplayHeight,
                                                       PixelFormat::RGBA_FP16);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();

        if (mReader->mCompositionChanges.size() != 0) {
            ASSERT_EQ(1, mReader->mCompositionChanges.size());
            ASSERT_EQ(1, mReader->mCompositionChanges[0].second);

            PixelFormat clientFormat = PixelFormat::RGBA_8888;
            uint64_t clientUsage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN |
                                                         BufferUsage::CPU_WRITE_OFTEN |
                                                         BufferUsage::COMPOSER_CLIENT_TARGET);
            Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
            IComposerClient::Rect damage{0, 0, mDisplayWidth, mDisplayHeight};

            bool clientTargetSupported = mComposerClient->getClientTargetSupport_2_2(
                    mPrimaryDisplay, layer->mWidth, layer->mHeight, clientFormat, clientDataspace);
            // if the client target format is not supported, skip this
            // configuration
            if (!clientTargetSupported) {
                std::cout << "Client target configuration width: " << layer->mWidth
                          << " height: " << layer->mHeight
                          << " pixel format: PixelFormat::RGBA_8888 dataspace: "
                          << ReadbackHelper::getDataspaceString(clientDataspace)
                          << " unsupported for display" << std::endl;
                continue;
            }

            // create client target buffer
            uint32_t clientStride;
            const native_handle_t* clientBufferHandle =
                    mGralloc->allocate(layer->mWidth, layer->mHeight, layer->mLayerCount,
                                       clientFormat, clientUsage, /*import*/ true, &clientStride);
            ASSERT_NE(nullptr, clientBufferHandle);

            void* clientBufData =
                    mGralloc->lock(clientBufferHandle, clientUsage, layer->mAccessRegion, -1);

            ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(layer->mWidth, layer->mHeight,
                                                               clientStride, clientBufData,
                                                               clientFormat, expectedColors));
            int clientFence = mGralloc->unlock(clientBufferHandle);
            if (clientFence != -1) {
                sync_wait(clientFence, -1);
                close(clientFence);
            }

            mWriter->setClientTarget(0, clientBufferHandle, clientFence, clientDataspace,
                                     std::vector<IComposerClient::Rect>(1, damage));

            layer->setToClientComposition(mWriter);
            mWriter->validateDisplay();
            execute();
            ASSERT_EQ(0, mReader->mCompositionChanges.size());
        }
        ASSERT_EQ(0, mReader->mErrors.size());

        mWriter->presentDisplay();
        execute();

        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, DeviceAndClientComposition) {
    ASSERT_NO_FATAL_FAILURE(
            mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kClientTargetSlotCount));

    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 2}, GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight}, RED);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        auto deviceLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGralloc, mPrimaryDisplay, mDisplayWidth, mDisplayHeight / 2,
                PixelFormat::RGBA_8888);
        std::vector<IComposerClient::Color> deviceColors(deviceLayer->mWidth *
                                                         deviceLayer->mHeight);
        ReadbackHelper::fillColorsArea(deviceColors, deviceLayer->mWidth,
                                       {0, 0, static_cast<int32_t>(deviceLayer->mWidth),
                                        static_cast<int32_t>(deviceLayer->mHeight)},
                                       GREEN);
        deviceLayer->setDisplayFrame({0, 0, static_cast<int32_t>(deviceLayer->mWidth),
                                      static_cast<int32_t>(deviceLayer->mHeight)});
        deviceLayer->setZOrder(10);
        deviceLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(deviceLayer->setBuffer(deviceColors));
        deviceLayer->write(mWriter);

        PixelFormat clientFormat = PixelFormat::RGBA_8888;
        uint64_t clientUsage =
                static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN |
                                      BufferUsage::COMPOSER_CLIENT_TARGET);
        Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
        int32_t clientWidth = mDisplayWidth;
        int32_t clientHeight = mDisplayHeight / 2;

        bool clientTargetSupported = mComposerClient->getClientTargetSupport_2_2(
                mPrimaryDisplay, clientWidth, clientHeight, clientFormat, clientDataspace);
        // if the client target format is not supported, skip this
        // configuration
        if (!clientTargetSupported) {
            std::cout << "Client target configuration width: " << clientWidth
                      << " height: " << clientHeight
                      << " pixel format: PixelFormat::RGBA_8888 dataspace: "
                      << ReadbackHelper::getDataspaceString(clientDataspace)
                      << " unsupported for display" << std::endl;
            continue;
        }

        auto clientLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGralloc, mPrimaryDisplay, clientWidth, clientHeight,
                PixelFormat::RGBA_FP16, IComposerClient::Composition::DEVICE);
        IComposerClient::Rect clientFrame = {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight};
        clientLayer->setDisplayFrame(clientFrame);
        clientLayer->setZOrder(0);
        clientLayer->write(mWriter);
        mWriter->validateDisplay();
        execute();

        if (mReader->mCompositionChanges.size() != 1) {
            std::cout << "HWC asked for none or more than 1 composition change, skipping"
                      << std::endl;
            mReader->mCompositionChanges.clear();
            continue;
        }
        // create client target buffer
        ASSERT_EQ(1, mReader->mCompositionChanges[0].second);
        uint32_t clientStride;
        const native_handle_t* clientBufferHandle =
                mGralloc->allocate(mDisplayWidth, mDisplayHeight, clientLayer->mLayerCount,
                                   clientFormat, clientUsage, /*import*/ true, &clientStride);
        ASSERT_NE(nullptr, clientBufferHandle);

        void* clientBufData = mGralloc->lock(clientBufferHandle, clientUsage,
                                             {0, 0, mDisplayWidth, mDisplayHeight}, -1);

        std::vector<IComposerClient::Color> clientColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(clientColors, mDisplayWidth, clientFrame, RED);
        ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(mDisplayWidth, mDisplayHeight,
                                                           clientStride, clientBufData,
                                                           clientFormat, clientColors));
        int clientFence = mGralloc->unlock(clientBufferHandle);
        if (clientFence != -1) {
            sync_wait(clientFence, -1);
            close(clientFence);
        }

        mWriter->setClientTarget(0, clientBufferHandle, clientFence, clientDataspace,
                                 std::vector<IComposerClient::Rect>(1, clientFrame));
        clientLayer->setToClientComposition(mWriter);
        mWriter->validateDisplay();
        execute();
        ASSERT_EQ(0, mReader->mCompositionChanges.size());
        ASSERT_EQ(0, mReader->mErrors.size());

        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerDamage) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        IComposerClient::Rect redRect = {0, 0, mDisplayWidth / 4, mDisplayHeight / 4};

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                                       mDisplayWidth, mDisplayHeight,
                                                       PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        // update surface damage and recheck
        redRect = {mDisplayWidth / 4, mDisplayHeight / 4, mDisplayWidth / 2, mDisplayHeight / 2};
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        ASSERT_NO_FATAL_FAILURE(layer->fillBuffer(expectedColors));
        layer->setSurfaceDamage(std::vector<IComposerClient::Rect>(
                1, {0, 0, mDisplayWidth / 2, mDisplayWidth / 2}));

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_EQ(0, mReader->mCompositionChanges.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerPlaneAlpha) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        layer->setColor(RED);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setAlpha(0);
        layer->setBlendMode(IComposerClient::BlendMode::PREMULTIPLIED);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());

        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerSourceCrop) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                                       mDisplayWidth, mDisplayHeight,
                                                       PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        layer->setSourceCrop({0, static_cast<float>(mDisplayHeight / 2),
                              static_cast<float>(mDisplayWidth),
                              static_cast<float>(mDisplayHeight)});
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        // update expected colors to match crop
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight}, BLUE);
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerZOrder) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        IComposerClient::Rect redRect = {0, 0, mDisplayWidth, mDisplayHeight / 2};
        IComposerClient::Rect blueRect = {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight};
        auto redLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        redLayer->setColor(RED);
        redLayer->setDisplayFrame(redRect);

        auto blueLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        blueLayer->setColor(BLUE);
        blueLayer->setDisplayFrame(blueRect);
        blueLayer->setZOrder(5);

        std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, blueLayer};
        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);

        // red in front of blue
        redLayer->setZOrder(10);

        // fill blue first so that red will overwrite on overlap
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, blueRect, BLUE);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        redLayer->setZOrder(1);
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, blueRect, BLUE);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        ASSERT_EQ(0, mReader->mCompositionChanges.size());
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

class GraphicsBlendModeCompositionTest
    : public GraphicsCompositionTestBase,
      public testing::WithParamInterface<std::tuple<std::string, std::string>> {
  public:
    void SetUp() override {
        SetUpBase(std::get<0>(GetParam()));
        mTestColorModes = {ColorMode::SRGB};  // TODO: add more color mode support
        mBackgroundColor = BLACK;
        mTopLayerColor = RED;
    }

    void setBackgroundColor(IComposerClient::Color color) { mBackgroundColor = color; }

    void setTopLayerColor(IComposerClient::Color color) { mTopLayerColor = color; }

    void setUpLayers(IComposerClient::BlendMode blendMode) {
        mLayers.clear();
        std::vector<IComposerClient::Color> topLayerPixelColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(topLayerPixelColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight}, mTopLayerColor);

        auto backgroundLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        backgroundLayer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        backgroundLayer->setZOrder(0);
        backgroundLayer->setColor(mBackgroundColor);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                                       mDisplayWidth, mDisplayHeight,
                                                       PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(Dataspace::UNKNOWN, mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(topLayerPixelColors));

        layer->setBlendMode(blendMode);
        layer->setAlpha(std::stof(std::get<1>(GetParam())));

        mLayers.push_back(backgroundLayer);
        mLayers.push_back(layer);
    }

    void setExpectedColors(std::vector<IComposerClient::Color>& expectedColors) {
        ASSERT_EQ(2, mLayers.size());
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);

        auto layer = mLayers[1];
        IComposerClient::BlendMode blendMode = layer->mBlendMode;
        float alpha = mTopLayerColor.a / 255.0 * layer->mAlpha;
        if (blendMode == IComposerClient::BlendMode::NONE) {
            for (int i = 0; i < expectedColors.size(); i++) {
                expectedColors[i].r = mTopLayerColor.r * layer->mAlpha;
                expectedColors[i].g = mTopLayerColor.g * layer->mAlpha;
                expectedColors[i].b = mTopLayerColor.b * layer->mAlpha;
                expectedColors[i].a = alpha * 255.0;
            }
        } else if (blendMode == IComposerClient::BlendMode::PREMULTIPLIED) {
            for (int i = 0; i < expectedColors.size(); i++) {
                expectedColors[i].r =
                    mTopLayerColor.r * layer->mAlpha + mBackgroundColor.r * (1.0 - alpha);
                expectedColors[i].g =
                    mTopLayerColor.g * layer->mAlpha + mBackgroundColor.g * (1.0 - alpha);
                expectedColors[i].b =
                    mTopLayerColor.b * layer->mAlpha + mBackgroundColor.b * (1.0 - alpha);
                expectedColors[i].a = alpha + mBackgroundColor.a * (1.0 - alpha);
            }
        } else if (blendMode == IComposerClient::BlendMode::COVERAGE) {
            for (int i = 0; i < expectedColors.size(); i++) {
                expectedColors[i].r = mTopLayerColor.r * alpha + mBackgroundColor.r * (1.0 - alpha);
                expectedColors[i].g = mTopLayerColor.g * alpha + mBackgroundColor.g * (1.0 - alpha);
                expectedColors[i].b = mTopLayerColor.b * alpha + mBackgroundColor.b * (1.0 - alpha);
                expectedColors[i].a = mTopLayerColor.a * alpha + mBackgroundColor.a * (1.0 - alpha);
            }
        }
    }

   protected:
    std::vector<std::shared_ptr<TestLayer>> mLayers;
    IComposerClient::Color mBackgroundColor;
    IComposerClient::Color mTopLayerColor;
};

// TODO(b/145557764): Re-enable after the bug is fixed.
TEST_P(GraphicsBlendModeCompositionTest, DISABLED_None) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(IComposerClient::BlendMode::NONE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

// TODO: bug 116865056: Readback returns (245, 0, 0) for layer plane
// alpha of .2, expected 10.2
TEST_P(GraphicsBlendModeCompositionTest, DISABLED_Coverage) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        mWriter->selectDisplay(mPrimaryDisplay);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);

        setUpLayers(IComposerClient::BlendMode::COVERAGE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Premultiplied) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        mWriter->selectDisplay(mPrimaryDisplay);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(IComposerClient::BlendMode::PREMULTIPLIED);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

class GraphicsTransformCompositionTest : public GraphicsCompositionTest {
  protected:
    void SetUp() override {
        GraphicsCompositionTest::SetUp();

        mWriter->selectDisplay(mPrimaryDisplay);

        auto backgroundLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        backgroundLayer->setColor({0, 0, 0, 0});
        backgroundLayer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        backgroundLayer->setZOrder(0);

        mSideLength = mDisplayWidth < mDisplayHeight ? mDisplayWidth : mDisplayHeight;
        IComposerClient::Rect redRect = {0, 0, mSideLength / 2, mSideLength / 2};
        IComposerClient::Rect blueRect = {mSideLength / 2, mSideLength / 2, mSideLength,
                                          mSideLength};

        mLayer =
            std::make_shared<TestBufferLayer>(mComposerClient, mGralloc, mPrimaryDisplay,
                                              mSideLength, mSideLength, PixelFormat::RGBA_8888);
        mLayer->setDisplayFrame({0, 0, mSideLength, mSideLength});
        mLayer->setZOrder(10);

        std::vector<IComposerClient::Color> baseColors(mSideLength * mSideLength);
        ReadbackHelper::fillColorsArea(baseColors, mSideLength, redRect, RED);
        ReadbackHelper::fillColorsArea(baseColors, mSideLength, blueRect, BLUE);
        ASSERT_NO_FATAL_FAILURE(mLayer->setBuffer(baseColors));

        mLayers = {backgroundLayer, mLayer};
    }

   protected:
    std::shared_ptr<TestBufferLayer> mLayer;
    std::vector<IComposerClient::Color> baseColors;
    std::vector<std::shared_ptr<TestLayer>> mLayers;
    int mSideLength;
};

TEST_P(GraphicsTransformCompositionTest, FLIP_H) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        mLayer->setTransform(Transform::FLIP_H);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, BLUE);

        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsTransformCompositionTest, FLIP_V) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::FLIP_V);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsTransformCompositionTest, ROT_180) {
    for (ColorMode mode : mTestColorModes) {
        std::cout << "---Testing Color Mode " << ReadbackHelper::getColorModeString(mode) << "---"
                  << std::endl;
        mWriter->selectDisplay(mPrimaryDisplay);
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        mComposerClient->getRaw()->getReadbackBufferAttributes(
                mPrimaryDisplay,
                [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                    mHasReadbackBuffer = ReadbackHelper::readbackSupported(tmpPixelFormat,
                                                                           tmpDataspace, tmpError);
                    mPixelFormat = tmpPixelFormat;
                    mDataspace = tmpDataspace;
                });

        if (!mHasReadbackBuffer) {
            std::cout << "Readback not supported or unsupported pixelFormat/dataspace" << std::endl;
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mGralloc, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::ROT_180);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, mSideLength / 2, mSideLength, mSideLength},
                                       RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mSideLength / 2, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->validateDisplay();
        execute();
        if (mReader->mCompositionChanges.size() != 0) {
            clearCommandReaderState();
            GTEST_SUCCEED();
            return;
        }
        ASSERT_EQ(0, mReader->mErrors.size());
        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsCompositionTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_CASE_P(
        BlendModeTest, GraphicsBlendModeCompositionTest,
        testing::Combine(
                testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
                testing::Values("0.2", "1.0")),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsTransformCompositionTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // anonymous namespace
}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
