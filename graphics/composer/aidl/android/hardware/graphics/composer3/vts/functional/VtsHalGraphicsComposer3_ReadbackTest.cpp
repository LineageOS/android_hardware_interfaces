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
#include <android/binder_manager.h>
#include <composer-vts/include/ReadbackVts.h>
#include <composer-vts/include/RenderEngineVts.h>
#include <gtest/gtest.h>
#include <ui/DisplayId.h>
#include <ui/DisplayIdentification.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>

// tinyxml2 does implicit conversions >:(
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <tinyxml2.h>
#pragma clang diagnostic pop
#include "composer-vts/include/GraphicsComposerCallback.h"

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using ::android::Rect;
using common::Dataspace;
using common::PixelFormat;

class GraphicsCompositionTestBase : public ::testing::Test {
  protected:
    void SetUpBase(const std::string& name) {
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer = IComposer::fromBinder(binder));
        ASSERT_NE(mComposer, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer->createClient(&mComposerClient));
        mComposerCallback = ::ndk::SharedRefBase::make<GraphicsComposerCallback>();
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();

        ASSERT_NO_FATAL_FAILURE(mInvalidDisplayId = GetInvalidDisplayId());

        int32_t activeConfig;
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &activeConfig).isOk());
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                  DisplayAttribute::WIDTH, &mDisplayWidth)
                            .isOk());
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                  DisplayAttribute::HEIGHT, &mDisplayHeight)
                            .isOk());

        setTestColorModes();

        // explicitly disable vsync
        EXPECT_TRUE(mComposerClient->setVsyncEnabled(mPrimaryDisplay, false).isOk());
        mComposerCallback->setVsyncAllowed(false);

        // set up gralloc
        mGraphicBuffer = allocate();

        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON));

        ASSERT_NO_FATAL_FAILURE(
                mTestRenderEngine = std::unique_ptr<TestRenderEngine>(new TestRenderEngine(
                        ::android::renderengine::RenderEngineCreationArgs::Builder()
                                .setPixelFormat(static_cast<int>(common::PixelFormat::RGBA_8888))
                                .setImageCacheSize(TestRenderEngine::sMaxFrameBufferAcquireBuffers)
                                .setUseColorManagerment(true)
                                .setEnableProtectedContext(false)
                                .setPrecacheToneMapperShaderOnly(false)
                                .setContextPriority(::android::renderengine::RenderEngine::
                                                            ContextPriority::HIGH)
                                .build())));

        ::android::renderengine::DisplaySettings clientCompositionDisplay;
        clientCompositionDisplay.physicalDisplay = Rect(mDisplayWidth, mDisplayHeight);
        clientCompositionDisplay.clip = clientCompositionDisplay.physicalDisplay;

        mTestRenderEngine->initGraphicBuffer(
                static_cast<uint32_t>(mDisplayWidth), static_cast<uint32_t>(mDisplayHeight), 1,
                static_cast<uint64_t>(
                        static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::GPU_RENDER_TARGET)));
        mTestRenderEngine->setDisplaySettings(clientCompositionDisplay);
    }

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::OFF));
        const auto errors = mReader.takeErrors();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty());

        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    ::android::sp<::android::GraphicBuffer> allocate() {
        const auto width = static_cast<uint32_t>(mDisplayWidth);
        const auto height = static_cast<uint32_t>(mDisplayHeight);
        const auto usage = static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                           static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN);

        return ::android::sp<::android::GraphicBuffer>::make(
                width, height, ::android::PIXEL_FORMAT_RGBA_8888,
                /*layerCount*/ 1u, usage, "VtsHalGraphicsComposer3_ReadbackTest");
    }

    uint64_t getStableDisplayId(int64_t display) {
        DisplayIdentification identification;
        const auto error = mComposerClient->getDisplayIdentificationData(display, &identification);
        EXPECT_TRUE(error.isOk());

        if (const auto info = ::android::parseDisplayIdentificationData(
                    static_cast<uint8_t>(identification.port), identification.data)) {
            return info->id.value;
        }

        return ::android::PhysicalDisplayId::fromPort(static_cast<uint8_t>(identification.port))
                .value;
    }

    // Gets the per-display XML config
    std::unique_ptr<tinyxml2::XMLDocument> getDisplayConfigXml(int64_t display) {
        std::stringstream pathBuilder;
        pathBuilder << "/vendor/etc/displayconfig/display_id_" << getStableDisplayId(display)
                    << ".xml";
        const std::string path = pathBuilder.str();
        auto document = std::make_unique<tinyxml2::XMLDocument>();
        const tinyxml2::XMLError error = document->LoadFile(path.c_str());
        if (error == tinyxml2::XML_SUCCESS) {
            return document;
        } else {
            return nullptr;
        }
    }

    // Gets the max display brightness for this display.
    // If the display config xml does not exist, then assume that the display is not well-configured
    // enough to provide a display brightness, so return nullopt.
    std::optional<float> getMaxDisplayBrightnessNits(int64_t display) {
        const auto document = getDisplayConfigXml(display);
        if (!document) {
            // Assume the device doesn't support display brightness
            return std::nullopt;
        }

        const auto root = document->RootElement();
        if (!root) {
            // If there's somehow no root element, then this isn't a valid config
            return std::nullopt;
        }

        const auto screenBrightnessMap = root->FirstChildElement("screenBrightnessMap");
        if (!screenBrightnessMap) {
            // A valid display config must have a screen brightness map
            return std::nullopt;
        }

        auto point = screenBrightnessMap->FirstChildElement("point");
        float maxNits = -1.f;
        while (point != nullptr) {
            const auto nits = point->FirstChildElement("nits");
            if (nits) {
                maxNits = std::max(maxNits, nits->FloatText(-1.f));
            }
            point = point->NextSiblingElement("point");
        }

        if (maxNits < 0.f) {
            // If we got here, then there were no point elements containing a nit value, so this
            // config isn't valid
            return std::nullopt;
        }

        return maxNits;
    }

    void writeLayers(const std::vector<std::shared_ptr<TestLayer>>& layers) {
        for (auto layer : layers) {
            layer->write(mWriter);
        }
        execute();
    }

    void execute() {
        const auto& commands = mWriter.getPendingCommands();
        if (commands.empty()) {
            mWriter.reset();
            return;
        }

        std::vector<CommandResultPayload> results;
        auto status = mComposerClient->executeCommands(commands, &results);
        ASSERT_TRUE(status.isOk()) << "executeCommands failed " << status.getDescription();

        mReader.parse(std::move(results));
        mWriter.reset();
    }

    bool getHasReadbackBuffer() {
        ReadbackBufferAttributes readBackBufferAttributes;
        const auto error = mComposerClient->getReadbackBufferAttributes(mPrimaryDisplay,
                                                                        &readBackBufferAttributes);
        mPixelFormat = readBackBufferAttributes.format;
        mDataspace = readBackBufferAttributes.dataspace;
        return error.isOk() && ReadbackHelper::readbackSupported(mPixelFormat, mDataspace);
    }

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;

    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    int64_t mPrimaryDisplay;
    int64_t mInvalidDisplayId;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
    std::vector<ColorMode> mTestColorModes;
    ComposerClientWriter mWriter;
    ComposerClientReader mReader;
    ::android::sp<::android::GraphicBuffer> mGraphicBuffer;
    std::unique_ptr<TestRenderEngine> mTestRenderEngine;

    common::PixelFormat mPixelFormat;
    common::Dataspace mDataspace;

    static constexpr uint32_t kClientTargetSlotCount = 64;

  private:
    int64_t waitForFirstDisplay() {
        while (true) {
            std::vector<int64_t> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                usleep(5 * 1000);
                continue;
            }
            return displays[0];
        }
    }

    void setTestColorModes() {
        mTestColorModes.clear();
        std::vector<ColorMode> modes;
        EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &modes).isOk());

        for (ColorMode mode : modes) {
            if (std::find(ReadbackHelper::colorModes.begin(), ReadbackHelper::colorModes.end(),
                          mode) != ReadbackHelper::colorModes.end()) {
                mTestColorModes.push_back(mode);
            }
        }
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    int64_t GetInvalidDisplayId() {
        int64_t id = std::numeric_limits<int64_t>::max();
        std::vector<int64_t> displays = mComposerCallback->getDisplays();
        while (id > 0) {
            if (std::none_of(displays.begin(), displays.end(),
                             [&](const auto& display) { return id == display; })) {
                return id;
            }
            id--;
        }

        // Although 0 could be an invalid display, a return value of 0
        // from GetInvalidDisplayId means all other ids are in use, a condition which
        // we are assuming a device will never have
        EXPECT_NE(0, id);
        return id;
    }
};

class GraphicsCompositionTest : public GraphicsCompositionTestBase,
                                public testing::WithParamInterface<std::string> {
  public:
    void SetUp() override { SetUpBase(GetParam()); }
};

TEST_P(GraphicsCompositionTest, SingleSolidColorLayer) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        common::Rect coloredSquare({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        // expected color for each pixel
        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        // if hwc cannot handle and asks for composition change,
        // just succeed the test
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBuffer) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight / 2},
                                       GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight, common::PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();

        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter.presentDisplay(mPrimaryDisplay);
        execute();

        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBufferNoEffect) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        common::Rect coloredSquare({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);
        layer->write(mWriter);

        // This following buffer call should have no effect
        uint64_t usage =
                static_cast<uint64_t>(static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                                      static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN));

        mGraphicBuffer->reallocate(static_cast<uint32_t>(mDisplayWidth),
                                   static_cast<uint32_t>(mDisplayHeight), 1,
                                   static_cast<uint32_t>(common::PixelFormat::RGBA_8888), usage);
        mWriter.setLayerBuffer(mPrimaryDisplay, layer->getLayer(), 0, mGraphicBuffer->handle, -1);

        // expected color for each pixel
        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();

        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer) {
    if (!getHasReadbackBuffer()) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth, mDisplayHeight,
                                  mPixelFormat, mDataspace);

    ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
}

TEST_P(GraphicsCompositionTest, SetReadbackBufferBadDisplay) {
    if (!getHasReadbackBuffer()) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    ASSERT_NE(nullptr, mGraphicBuffer);
    ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
    aidl::android::hardware::common::NativeHandle bufferHandle =
            ::android::dupToAidl(mGraphicBuffer->handle);
    ::ndk::ScopedFileDescriptor fence = ::ndk::ScopedFileDescriptor(-1);

    const auto error = mComposerClient->setReadbackBuffer(mInvalidDisplayId, bufferHandle, fence);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsCompositionTest, SetReadbackBufferBadParameter) {
    if (!getHasReadbackBuffer()) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    aidl::android::hardware::common::NativeHandle bufferHandle;
    {
        ::android::sp<::android::GraphicBuffer> buffer = allocate();
        ASSERT_EQ(::android::OK, mGraphicBuffer->initCheck());
        ::android::makeToAidl(mGraphicBuffer->handle);
    }

    ndk::ScopedFileDescriptor releaseFence = ndk::ScopedFileDescriptor(-1);
    const auto error =
            mComposerClient->setReadbackBuffer(mPrimaryDisplay, bufferHandle, releaseFence);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

TEST_P(GraphicsCompositionTest, GetReadbackBufferFenceInactive) {
    if (!getHasReadbackBuffer()) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    ndk::ScopedFileDescriptor releaseFence;
    const auto error = mComposerClient->getReadbackBufferFence(mPrimaryDisplay, &releaseFence);

    EXPECT_TRUE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
}

TEST_P(GraphicsCompositionTest, ClientComposition) {
    EXPECT_TRUE(mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kClientTargetSlotCount)
                        .isOk());

    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight / 2},
                                       GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight, PixelFormat::RGBA_FP16);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();

        auto changedCompositionTypes = mReader.takeChangedCompositionTypes(mPrimaryDisplay);
        if (!changedCompositionTypes.empty()) {
            ASSERT_EQ(1, changedCompositionTypes.size());
            ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);

            PixelFormat clientFormat = PixelFormat::RGBA_8888;
            auto clientUsage = static_cast<uint32_t>(
                    static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
            Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
            common::Rect damage{0, 0, mDisplayWidth, mDisplayHeight};

            // create client target buffer
            mGraphicBuffer->reallocate(layer->getWidth(), layer->getHeight(),
                                       static_cast<int32_t>(common::PixelFormat::RGBA_8888),
                                       layer->getLayerCount(), clientUsage);

            ASSERT_NE(nullptr, mGraphicBuffer->handle);

            void* clientBufData;
            mGraphicBuffer->lock(clientUsage, layer->getAccessRegion(), &clientBufData);

            ASSERT_NO_FATAL_FAILURE(
                    ReadbackHelper::fillBuffer(layer->getWidth(), layer->getHeight(),
                                               static_cast<uint32_t>(mGraphicBuffer->stride),
                                               clientBufData, clientFormat, expectedColors));
            EXPECT_EQ(::android::OK, mGraphicBuffer->unlock());

            ndk::ScopedFileDescriptor fenceHandle;
            EXPECT_TRUE(
                    mComposerClient->getReadbackBufferFence(mPrimaryDisplay, &fenceHandle).isOk());

            layer->setToClientComposition(mWriter);
            mWriter.acceptDisplayChanges(mPrimaryDisplay);
            mWriter.setClientTarget(mPrimaryDisplay, 0, mGraphicBuffer->handle, fenceHandle.get(),
                                    clientDataspace, std::vector<common::Rect>(1, damage));
            execute();
            changedCompositionTypes = mReader.takeChangedCompositionTypes(mPrimaryDisplay);
            ASSERT_TRUE(changedCompositionTypes.empty());
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter.presentDisplay(mPrimaryDisplay);
        execute();

        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, DeviceAndClientComposition) {
    ASSERT_NO_FATAL_FAILURE(
            mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kClientTargetSlotCount));

    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 2}, GREEN);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight}, RED);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        auto deviceLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight / 2, PixelFormat::RGBA_8888);
        std::vector<Color> deviceColors(deviceLayer->getWidth() * deviceLayer->getHeight());
        ReadbackHelper::fillColorsArea(deviceColors, static_cast<int32_t>(deviceLayer->getWidth()),
                                       {0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                        static_cast<int32_t>(deviceLayer->getHeight())},
                                       GREEN);
        deviceLayer->setDisplayFrame({0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                      static_cast<int32_t>(deviceLayer->getHeight())});
        deviceLayer->setZOrder(10);
        deviceLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(deviceLayer->setBuffer(deviceColors));
        deviceLayer->write(mWriter);

        PixelFormat clientFormat = PixelFormat::RGBA_8888;
        auto clientUsage = static_cast<uint32_t>(
                static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
        Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
        int32_t clientWidth = mDisplayWidth;
        int32_t clientHeight = mDisplayHeight / 2;

        auto clientLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, clientWidth,
                clientHeight, PixelFormat::RGBA_FP16, Composition::DEVICE);
        common::Rect clientFrame = {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight};
        clientLayer->setDisplayFrame(clientFrame);
        clientLayer->setZOrder(0);
        clientLayer->write(mWriter);
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();

        auto changedCompositionTypes = mReader.takeChangedCompositionTypes(mPrimaryDisplay);
        if (changedCompositionTypes.size() != 1) {
            continue;
        }
        // create client target buffer
        ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);
        mGraphicBuffer->reallocate(static_cast<uint32_t>(mDisplayWidth),
                                   static_cast<uint32_t>(mDisplayHeight),
                                   static_cast<int32_t>(common::PixelFormat::RGBA_8888),
                                   clientLayer->getLayerCount(), clientUsage);
        ASSERT_NE(nullptr, mGraphicBuffer->handle);

        void* clientBufData;
        mGraphicBuffer->lock(clientUsage, {0, 0, mDisplayWidth, mDisplayHeight}, &clientBufData);

        std::vector<Color> clientColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(clientColors, mDisplayWidth, clientFrame, RED);
        ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(
                static_cast<uint32_t>(mDisplayWidth), static_cast<uint32_t>(mDisplayHeight),
                mGraphicBuffer->getStride(), clientBufData, clientFormat, clientColors));
        EXPECT_EQ(::android::OK, mGraphicBuffer->unlock());

        ndk::ScopedFileDescriptor fenceHandle;
        EXPECT_TRUE(mComposerClient->getReadbackBufferFence(mPrimaryDisplay, &fenceHandle).isOk());

        clientLayer->setToClientComposition(mWriter);
        mWriter.acceptDisplayChanges(mPrimaryDisplay);
        mWriter.setClientTarget(mPrimaryDisplay, 0, mGraphicBuffer->handle, fenceHandle.get(),
                                clientDataspace, std::vector<common::Rect>(1, clientFrame));
        execute();
        changedCompositionTypes = mReader.takeChangedCompositionTypes(mPrimaryDisplay);
        ASSERT_TRUE(changedCompositionTypes.empty());
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerDamage) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        common::Rect redRect = {0, 0, mDisplayWidth / 4, mDisplayHeight / 4};

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight, PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        // update surface damage and recheck
        redRect = {mDisplayWidth / 4, mDisplayHeight / 4, mDisplayWidth / 2, mDisplayHeight / 2};
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        ASSERT_NO_FATAL_FAILURE(layer->fillBuffer(expectedColors));
        layer->setSurfaceDamage(
                std::vector<common::Rect>(1, {0, 0, mDisplayWidth / 2, mDisplayWidth / 2}));

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerPlaneAlpha) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        layer->setColor(RED);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setAlpha(0);
        layer->setBlendMode(BlendMode::PREMULTIPLIED);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerSourceCrop) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight / 4}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight},
                                       BLUE);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight, PixelFormat::RGBA_8888);
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
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerZOrder) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        common::Rect redRect = {0, 0, mDisplayWidth, mDisplayHeight / 2};
        common::Rect blueRect = {0, mDisplayHeight / 4, mDisplayWidth, mDisplayHeight};
        auto redLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        redLayer->setColor(RED);
        redLayer->setDisplayFrame(redRect);

        auto blueLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        blueLayer->setColor(BLUE);
        blueLayer->setDisplayFrame(blueRect);
        blueLayer->setZOrder(5);

        std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, blueLayer};
        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        // red in front of blue
        redLayer->setZOrder(10);

        // fill blue first so that red will overwrite on overlap
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, blueRect, BLUE);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        redLayer->setZOrder(1);
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, blueRect, BLUE);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty());
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerWhitePointDims) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());

    const bool brightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                             DisplayCapability::BRIGHTNESS) != capabilities.end();

    if (!brightnessSupport) {
        GTEST_SUCCEED() << "Cannot verify dimming behavior without brightness support";
        return;
    }

    const std::optional<float> maxBrightnessNitsOptional =
            getMaxDisplayBrightnessNits(mPrimaryDisplay);

    ASSERT_TRUE(maxBrightnessNitsOptional.has_value());

    const float maxBrightnessNits = *maxBrightnessNitsOptional;

    // Preconditions to successfully run are knowing the max brightness and successfully applying
    // the max brightness
    ASSERT_GT(maxBrightnessNits, 0.f);
    mWriter.setDisplayBrightness(mPrimaryDisplay, 1.f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace for "
                               "color mode: "
                            << toString(mode);
            continue;
        }
        const common::Rect redRect = {0, 0, mDisplayWidth, mDisplayHeight / 2};
        const common::Rect dimmerRedRect = {0, mDisplayHeight / 2, mDisplayWidth, mDisplayHeight};
        const auto redLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        redLayer->setColor(RED);
        redLayer->setDisplayFrame(redRect);
        redLayer->setWhitePointNits(maxBrightnessNits);

        const auto dimmerRedLayer =
                std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        dimmerRedLayer->setColor(RED);
        dimmerRedLayer->setDisplayFrame(dimmerRedRect);
        // Intentionally use a small dimming ratio as some implementations may be more likely to
        // kick into GPU composition to apply dithering when the dimming ratio is high.
        static constexpr float kDimmingRatio = 0.9f;
        dimmerRedLayer->setWhitePointNits(maxBrightnessNits * kDimmingRatio);

        const std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, dimmerRedLayer};
        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, redRect, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth, dimmerRedRect, DIM_RED);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED()
                    << "Readback verification not supported for GPU composition for color mode: "
                    << toString(mode);
            continue;
        }
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

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
        mBackgroundColor = BLACK;
        mTopLayerColor = RED;
    }

    void setBackgroundColor(Color color) { mBackgroundColor = color; }

    void setTopLayerColor(Color color) { mTopLayerColor = color; }

    void setUpLayers(BlendMode blendMode) {
        mLayers.clear();
        std::vector<Color> topLayerPixelColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(topLayerPixelColors, mDisplayWidth,
                                       {0, 0, mDisplayWidth, mDisplayHeight}, mTopLayerColor);

        auto backgroundLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        backgroundLayer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        backgroundLayer->setZOrder(0);
        backgroundLayer->setColor(mBackgroundColor);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, mGraphicBuffer, *mTestRenderEngine, mPrimaryDisplay, mDisplayWidth,
                mDisplayHeight, PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        layer->setZOrder(10);
        layer->setDataspace(Dataspace::UNKNOWN, mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(topLayerPixelColors));

        layer->setBlendMode(blendMode);
        layer->setAlpha(std::stof(std::get<1>(GetParam())));

        mLayers.push_back(backgroundLayer);
        mLayers.push_back(layer);
    }

    void setExpectedColors(std::vector<Color>& expectedColors) {
        ASSERT_EQ(2, mLayers.size());
        ReadbackHelper::clearColors(expectedColors, mDisplayWidth, mDisplayHeight, mDisplayWidth);

        auto layer = mLayers[1];
        BlendMode blendMode = layer->getBlendMode();
        float alpha = mTopLayerColor.a * layer->getAlpha();
        if (blendMode == BlendMode::NONE) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r = mTopLayerColor.r * layer->getAlpha();
                expectedColor.g = mTopLayerColor.g * layer->getAlpha();
                expectedColor.b = mTopLayerColor.b * layer->getAlpha();
                expectedColor.a = alpha;
            }
        } else if (blendMode == BlendMode::PREMULTIPLIED) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r =
                        mTopLayerColor.r * layer->getAlpha() + mBackgroundColor.r * (1.0f - alpha);
                expectedColor.g =
                        mTopLayerColor.g * layer->getAlpha() + mBackgroundColor.g * (1.0f - alpha);
                expectedColor.b =
                        mTopLayerColor.b * layer->getAlpha() + mBackgroundColor.b * (1.0f - alpha);
                expectedColor.a = alpha + mBackgroundColor.a * (1.0f - alpha);
            }
        } else if (blendMode == BlendMode::COVERAGE) {
            for (auto& expectedColor : expectedColors) {
                expectedColor.r = mTopLayerColor.r * alpha + mBackgroundColor.r * (1.0f - alpha);
                expectedColor.g = mTopLayerColor.g * alpha + mBackgroundColor.g * (1.0f - alpha);
                expectedColor.b = mTopLayerColor.b * alpha + mBackgroundColor.b * (1.0f - alpha);
                expectedColor.a = mTopLayerColor.a * alpha + mBackgroundColor.a * (1.0f - alpha);
            }
        }
    }

  protected:
    std::vector<std::shared_ptr<TestLayer>> mLayers;
    Color mBackgroundColor;
    Color mTopLayerColor;
};

TEST_P(GraphicsBlendModeCompositionTest, None) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(BlendMode::NONE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Coverage) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);

        setUpLayers(BlendMode::COVERAGE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Premultiplied) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(BlendMode::PREMULTIPLIED);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
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

        auto backgroundLayer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
        backgroundLayer->setColor({0.0f, 0.0f, 0.0f, 0.0f});
        backgroundLayer->setDisplayFrame({0, 0, mDisplayWidth, mDisplayHeight});
        backgroundLayer->setZOrder(0);

        mSideLength = mDisplayWidth < mDisplayHeight ? mDisplayWidth : mDisplayHeight;
        common::Rect redRect = {0, 0, mSideLength / 2, mSideLength / 2};
        common::Rect blueRect = {mSideLength / 2, mSideLength / 2, mSideLength, mSideLength};

        mLayer = std::make_shared<TestBufferLayer>(mComposerClient, mGraphicBuffer,
                                                   *mTestRenderEngine, mPrimaryDisplay, mSideLength,
                                                   mSideLength, PixelFormat::RGBA_8888);
        mLayer->setDisplayFrame({0, 0, mSideLength, mSideLength});
        mLayer->setZOrder(10);

        std::vector<Color> baseColors(static_cast<size_t>(mSideLength * mSideLength));
        ReadbackHelper::fillColorsArea(baseColors, mSideLength, redRect, RED);
        ReadbackHelper::fillColorsArea(baseColors, mSideLength, blueRect, BLUE);
        ASSERT_NO_FATAL_FAILURE(mLayer->setBuffer(baseColors));
        mLayers = {backgroundLayer, mLayer};
    }

  protected:
    std::shared_ptr<TestBufferLayer> mLayer;
    std::vector<std::shared_ptr<TestLayer>> mLayers;
    int mSideLength;
};

TEST_P(GraphicsTransformCompositionTest, FLIP_H) {
    for (ColorMode mode : mTestColorModes) {
        auto error =
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC);
        if (!error.isOk() &&
            (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED ||
             error.getServiceSpecificError() == IComposerClient::EX_BAD_PARAMETER)) {
            SUCCEED() << "ColorMode not supported, skip test";
            return;
        }

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        mLayer->setTransform(Transform::FLIP_H);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsTransformCompositionTest, FLIP_V) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::FLIP_V);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsTransformCompositionTest, ROT_180) {
    for (ColorMode mode : mTestColorModes) {
        ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode(mPrimaryDisplay, mode, RenderIntent::COLORIMETRIC));

        if (!getHasReadbackBuffer()) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(mPrimaryDisplay, mComposerClient, mDisplayWidth,
                                      mDisplayHeight, mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::ROT_180);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), mWriter);

        std::vector<Color> expectedColors(static_cast<size_t>(mDisplayWidth * mDisplayHeight));
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {mSideLength / 2, mSideLength / 2, mSideLength, mSideLength},
                                       RED);
        ReadbackHelper::fillColorsArea(expectedColors, mDisplayWidth,
                                       {0, 0, mSideLength / 2, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(mPrimaryDisplay).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(mLayers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
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

}  // namespace
}  // namespace aidl::android::hardware::graphics::composer3::vts
