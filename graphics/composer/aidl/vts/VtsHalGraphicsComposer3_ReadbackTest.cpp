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
#include <gtest/gtest.h>
#include <ui/DisplayId.h>
#include <ui/DisplayIdentification.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include "GraphicsComposerCallback.h"
#include "ReadbackVts.h"
#include "RenderEngineVts.h"
#include "VtsComposerClient.h"

// tinyxml2 does implicit conversions >:(
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <tinyxml2.h>
#pragma clang diagnostic pop

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using ::android::Rect;
using common::Dataspace;
using common::PixelFormat;

class GraphicsCompositionTestBase : public ::testing::Test {
  protected:
    void SetUpBase(const std::string& name) {
        mComposerClient = std::make_shared<VtsComposerClient>(name);
        ASSERT_TRUE(mComposerClient->createClient().isOk());

        const auto& [status, displays] = mComposerClient->getDisplays();
        ASSERT_TRUE(status.isOk());
        mDisplays = displays;
        mWriter.reset(new ComposerClientWriter(getPrimaryDisplayId()));

        setTestColorModes();

        // explicitly disable vsync
        for (const auto& display : mDisplays) {
            EXPECT_TRUE(mComposerClient->setVsync(display.getDisplayId(), /*enable*/ false).isOk());
        }
        mComposerClient->setVsyncAllowed(/*isAllowed*/ false);

        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());

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
        clientCompositionDisplay.physicalDisplay = Rect(getDisplayWidth(), getDisplayHeight());
        clientCompositionDisplay.clip = clientCompositionDisplay.physicalDisplay;

        mTestRenderEngine->initGraphicBuffer(
                static_cast<uint32_t>(getDisplayWidth()), static_cast<uint32_t>(getDisplayHeight()),
                /*layerCount*/ 1U,
                static_cast<uint64_t>(
                        static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::GPU_RENDER_TARGET)));
        mTestRenderEngine->setDisplaySettings(clientCompositionDisplay);
    }

    void TearDown() override {
        ASSERT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::OFF).isOk());
        ASSERT_TRUE(mComposerClient->tearDown());
        mComposerClient.reset();
        const auto errors = mReader.takeErrors();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty());
    }

    const VtsDisplay& getPrimaryDisplay() const { return mDisplays[0]; }

    int64_t getPrimaryDisplayId() const { return getPrimaryDisplay().getDisplayId(); }

    int64_t getInvalidDisplayId() const { return mComposerClient->getInvalidDisplayId(); }

    int32_t getDisplayWidth() const { return getPrimaryDisplay().getDisplayWidth(); }

    int32_t getDisplayHeight() const { return getPrimaryDisplay().getDisplayHeight(); }

    void assertServiceSpecificError(const ScopedAStatus& status, int32_t serviceSpecificError) {
        ASSERT_EQ(status.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(status.getServiceSpecificError(), serviceSpecificError);
    }

    std::pair<bool, ::android::sp<::android::GraphicBuffer>> allocateBuffer(uint32_t usage) {
        const auto width = static_cast<uint32_t>(getDisplayWidth());
        const auto height = static_cast<uint32_t>(getDisplayHeight());

        const auto& graphicBuffer = ::android::sp<::android::GraphicBuffer>::make(
                width, height, ::android::PIXEL_FORMAT_RGBA_8888,
                /*layerCount*/ 1u, usage, "VtsHalGraphicsComposer3_ReadbackTest");

        if (graphicBuffer && ::android::OK == graphicBuffer->initCheck()) {
            return {true, graphicBuffer};
        }
        return {false, graphicBuffer};
    }

    // Gets the per-display XML config
    std::unique_ptr<tinyxml2::XMLDocument> getDisplayConfigXml(int64_t display) {

        if (auto document = getDisplayConfigXmlByStableId(getStableDisplayId(display));
                document != nullptr) {
            return document;
        }

        // Fallback to looking up a per-port config if no config exists for the full ID
        if (auto document = getDisplayConfigXmlByPort(getPort(display)); document != nullptr) {
            return document;
        }

        return nullptr;
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
        for (const auto& layer : layers) {
            layer->write(*mWriter);
        }
        execute();
    }

    void execute() {
        auto commands = mWriter->takePendingCommands();
        if (commands.empty()) {
            return;
        }

        auto [status, results] = mComposerClient->executeCommands(commands);
        ASSERT_TRUE(status.isOk()) << "executeCommands failed " << status.getDescription();

        mReader.parse(std::move(results));
    }

    bool getHasReadbackBuffer() {
        auto [status, readBackBufferAttributes] =
                mComposerClient->getReadbackBufferAttributes(getPrimaryDisplayId());
        if (status.isOk()) {
            mPixelFormat = readBackBufferAttributes.format;
            mDataspace = readBackBufferAttributes.dataspace;
            return ReadbackHelper::readbackSupported(mPixelFormat, mDataspace);
        }
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
        return false;
    }

    std::shared_ptr<VtsComposerClient> mComposerClient;
    std::vector<VtsDisplay> mDisplays;
    // use the slot count usually set by SF
    std::vector<ColorMode> mTestColorModes;
    std::unique_ptr<ComposerClientWriter> mWriter;
    ComposerClientReader mReader;
    std::unique_ptr<TestRenderEngine> mTestRenderEngine;
    common::PixelFormat mPixelFormat;
    common::Dataspace mDataspace;

    static constexpr uint32_t kClientTargetSlotCount = 64;

  private:
    void setTestColorModes() {
        mTestColorModes.clear();
        const auto& [status, modes] = mComposerClient->getColorModes(getPrimaryDisplayId());
        ASSERT_TRUE(status.isOk());

        for (ColorMode mode : modes) {
            if (std::find(ReadbackHelper::colorModes.begin(), ReadbackHelper::colorModes.end(),
                          mode) != ReadbackHelper::colorModes.end()) {
                mTestColorModes.push_back(mode);
            }
        }
    }

    uint8_t getPort(int64_t display) {
        const auto& [status, identification] =
                mComposerClient->getDisplayIdentificationData(display);
        EXPECT_TRUE(status.isOk());
        return static_cast<uint8_t>(identification.port);
    }

    uint64_t getStableDisplayId(int64_t display) {
        const auto& [status, identification] =
                mComposerClient->getDisplayIdentificationData(display);
        EXPECT_TRUE(status.isOk());

        if (const auto info = ::android::parseDisplayIdentificationData(
                    static_cast<uint8_t>(identification.port), identification.data)) {
            return info->id.value;
        }

        return ::android::PhysicalDisplayId::fromPort(static_cast<uint8_t>(identification.port))
                .value;
    }

    std::unique_ptr<tinyxml2::XMLDocument> loadXml(const std::string& path) {
        auto document = std::make_unique<tinyxml2::XMLDocument>();
        const tinyxml2::XMLError error = document->LoadFile(path.c_str());
        if (error != tinyxml2::XML_SUCCESS) {
            ALOGD("%s: Failed to load config file: %s", __func__, path.c_str());
            return nullptr;
        }

        ALOGD("%s: Successfully loaded config file: %s", __func__, path.c_str());
        return document;
    }

    std::unique_ptr<tinyxml2::XMLDocument> getDisplayConfigXmlByPort(uint8_t port) {
        std::stringstream pathBuilder;
        pathBuilder << "/vendor/etc/displayconfig/display_port_" << static_cast<uint32_t>(port)
                    << ".xml";
        return loadXml(pathBuilder.str());
    }

    std::unique_ptr<tinyxml2::XMLDocument> getDisplayConfigXmlByStableId(uint64_t stableId) {
        std::stringstream pathBuilder;
        pathBuilder << "/vendor/etc/displayconfig/display_id_" << stableId
                    << ".xml";
       return loadXml(pathBuilder.str());
    }
};

class GraphicsCompositionTest : public GraphicsCompositionTestBase,
                                public testing::WithParamInterface<std::string> {
  public:
    void SetUp() override { SetUpBase(GetParam()); }
};

TEST_P(GraphicsCompositionTest, SingleSolidColorLayer) {
    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        common::Rect coloredSquare({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        // expected color for each pixel
        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        // if hwc cannot handle and asks for composition change,
        // just succeed the test
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight() / 4}, RED);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 4, getDisplayWidth(), getDisplayHeight() / 2}, GREEN);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 2, getDisplayWidth(), getDisplayHeight()}, BLUE);

        auto layer = std::make_shared<TestBufferLayer>(
                mComposerClient, *mTestRenderEngine, getPrimaryDisplayId(), getDisplayWidth(),
                getDisplayHeight(), common::PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();

        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        common::Rect coloredSquare({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setColor(BLUE);
        layer->setDisplayFrame(coloredSquare);
        layer->setZOrder(10);
        layer->write(*mWriter);

        // This following buffer call should have no effect
        const auto usage = static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                           static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN);
        const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(usage);
        ASSERT_TRUE(graphicBufferStatus);
        const auto& buffer = graphicBuffer->handle;
        mWriter->setLayerBuffer(getPrimaryDisplayId(), layer->getLayer(), /*slot*/ 0, buffer,
                                /*acquireFence*/ -1);

        // expected color for each pixel
        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), coloredSquare, BLUE);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();

        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer) {
    bool isSupported;
    ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
    if (!isSupported) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                  getDisplayHeight(), mPixelFormat, mDataspace);

    ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer_BadDisplay) {
    bool isSupported;
    ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
    if (!isSupported) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    const auto usage = static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                       static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN);
    const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(usage);
    ASSERT_TRUE(graphicBufferStatus);
    const auto& bufferHandle = graphicBuffer->handle;
    ::ndk::ScopedFileDescriptor fence = ::ndk::ScopedFileDescriptor(-1);

    const auto status =
            mComposerClient->setReadbackBuffer(getInvalidDisplayId(), bufferHandle, fence);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsCompositionTest, SetReadbackBuffer_BadParameter) {
    bool isSupported;
    ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
    if (!isSupported) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    const native_handle_t bufferHandle{};
    ndk::ScopedFileDescriptor releaseFence = ndk::ScopedFileDescriptor(-1);
    const auto status =
            mComposerClient->setReadbackBuffer(getPrimaryDisplayId(), &bufferHandle, releaseFence);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

TEST_P(GraphicsCompositionTest, GetReadbackBufferFenceInactive) {
    bool isSupported;
    ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
    if (!isSupported) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }

    const auto& [status, releaseFence] =
            mComposerClient->getReadbackBufferFence(getPrimaryDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
    EXPECT_EQ(-1, releaseFence.get());
}

TEST_P(GraphicsCompositionTest, ClientComposition) {
    EXPECT_TRUE(
            mComposerClient->setClientTargetSlotCount(getPrimaryDisplayId(), kClientTargetSlotCount)
                    .isOk());

    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight() / 4}, RED);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 4, getDisplayWidth(), getDisplayHeight() / 2}, GREEN);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 2, getDisplayWidth(), getDisplayHeight()}, BLUE);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, *mTestRenderEngine,
                                                       getPrimaryDisplayId(), getDisplayWidth(),
                                                       getDisplayHeight(), PixelFormat::RGBA_FP16);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();

        auto changedCompositionTypes = mReader.takeChangedCompositionTypes(getPrimaryDisplayId());
        if (!changedCompositionTypes.empty()) {
            ASSERT_EQ(1, changedCompositionTypes.size());
            ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);

            PixelFormat clientFormat = PixelFormat::RGBA_8888;
            auto clientUsage = static_cast<uint32_t>(
                    static_cast<uint32_t>(common::BufferUsage::CPU_READ_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                    static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
            Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
            common::Rect damage{0, 0, getDisplayWidth(), getDisplayHeight()};

            // create client target buffer
            const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(clientUsage);
            ASSERT_TRUE(graphicBufferStatus);
            const auto& buffer = graphicBuffer->handle;
            void* clientBufData;
            const auto stride = static_cast<uint32_t>(graphicBuffer->stride);
            graphicBuffer->lock(clientUsage, layer->getAccessRegion(), &clientBufData);

            ASSERT_NO_FATAL_FAILURE(
                    ReadbackHelper::fillBuffer(layer->getWidth(), layer->getHeight(), stride,
                                               clientBufData, clientFormat, expectedColors));
            int32_t clientFence;
            const auto unlockStatus = graphicBuffer->unlockAsync(&clientFence);
            ASSERT_EQ(::android::OK, unlockStatus);
            mWriter->setClientTarget(getPrimaryDisplayId(), /*slot*/ 0, buffer, clientFence,
                                     clientDataspace, std::vector<common::Rect>(1, damage));
            layer->setToClientComposition(*mWriter);
            mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
            execute();
            changedCompositionTypes = mReader.takeChangedCompositionTypes(getPrimaryDisplayId());
            ASSERT_TRUE(changedCompositionTypes.empty());
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();

        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, DeviceAndClientComposition) {
    ASSERT_TRUE(
            mComposerClient->setClientTargetSlotCount(getPrimaryDisplayId(), kClientTargetSlotCount)
                    .isOk());

    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight() / 2}, GREEN);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 2, getDisplayWidth(), getDisplayHeight()}, RED);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        auto deviceLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, *mTestRenderEngine, getPrimaryDisplayId(), getDisplayWidth(),
                getDisplayHeight() / 2, PixelFormat::RGBA_8888);
        std::vector<Color> deviceColors(deviceLayer->getWidth() * deviceLayer->getHeight());
        ReadbackHelper::fillColorsArea(deviceColors, static_cast<int32_t>(deviceLayer->getWidth()),
                                       {0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                        static_cast<int32_t>(deviceLayer->getHeight())},
                                       GREEN);
        deviceLayer->setDisplayFrame({0, 0, static_cast<int32_t>(deviceLayer->getWidth()),
                                      static_cast<int32_t>(deviceLayer->getHeight())});
        deviceLayer->setZOrder(10);
        deviceLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);
        ASSERT_NO_FATAL_FAILURE(deviceLayer->setBuffer(deviceColors));
        deviceLayer->write(*mWriter);

        PixelFormat clientFormat = PixelFormat::RGBA_8888;
        auto clientUsage = static_cast<uint32_t>(
                static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                static_cast<uint32_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                static_cast<uint32_t>(common::BufferUsage::COMPOSER_CLIENT_TARGET));
        Dataspace clientDataspace = ReadbackHelper::getDataspaceForColorMode(mode);
        int32_t clientWidth = getDisplayWidth();
        int32_t clientHeight = getDisplayHeight() / 2;

        auto clientLayer = std::make_shared<TestBufferLayer>(
                mComposerClient, *mTestRenderEngine, getPrimaryDisplayId(), clientWidth,
                clientHeight, PixelFormat::RGBA_FP16, Composition::DEVICE);
        common::Rect clientFrame = {0, getDisplayHeight() / 2, getDisplayWidth(),
                                    getDisplayHeight()};
        clientLayer->setDisplayFrame(clientFrame);
        clientLayer->setZOrder(0);
        clientLayer->write(*mWriter);
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();

        auto changedCompositionTypes = mReader.takeChangedCompositionTypes(getPrimaryDisplayId());
        if (changedCompositionTypes.size() != 1) {
            continue;
        }
        // create client target buffer
        ASSERT_EQ(Composition::CLIENT, changedCompositionTypes[0].composition);
        const auto& [graphicBufferStatus, graphicBuffer] = allocateBuffer(clientUsage);
        ASSERT_TRUE(graphicBufferStatus);
        const auto& buffer = graphicBuffer->handle;

        void* clientBufData;
        graphicBuffer->lock(clientUsage, {0, 0, getDisplayWidth(), getDisplayHeight()},
                            &clientBufData);

        std::vector<Color> clientColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(clientColors, getDisplayWidth(), clientFrame, RED);
        ASSERT_NO_FATAL_FAILURE(ReadbackHelper::fillBuffer(
                static_cast<uint32_t>(getDisplayWidth()), static_cast<uint32_t>(getDisplayHeight()),
                graphicBuffer->getStride(), clientBufData, clientFormat, clientColors));
        int32_t clientFence;
        const auto unlockStatus = graphicBuffer->unlockAsync(&clientFence);
        ASSERT_EQ(::android::OK, unlockStatus);
        mWriter->setClientTarget(getPrimaryDisplayId(), /*slot*/ 0, buffer, clientFence,
                                 clientDataspace, std::vector<common::Rect>(1, clientFrame));
        clientLayer->setToClientComposition(*mWriter);
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        changedCompositionTypes = mReader.takeChangedCompositionTypes(getPrimaryDisplayId());
        ASSERT_TRUE(changedCompositionTypes.empty());
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerDamage) {
    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        common::Rect redRect = {0, 0, getDisplayWidth() / 4, getDisplayHeight() / 4};

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), redRect, RED);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, *mTestRenderEngine,
                                                       getPrimaryDisplayId(), getDisplayWidth(),
                                                       getDisplayHeight(), PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        // update surface damage and recheck
        redRect = {getDisplayWidth() / 4, getDisplayHeight() / 4, getDisplayWidth() / 2,
                   getDisplayHeight() / 2};
        ReadbackHelper::clearColors(expectedColors, getDisplayWidth(), getDisplayHeight(),
                                    getDisplayWidth());
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), redRect, RED);

        ASSERT_NO_FATAL_FAILURE(layer->fillBuffer(expectedColors));
        layer->setSurfaceDamage(
                std::vector<common::Rect>(1, {0, 0, getDisplayWidth() / 2, getDisplayWidth() / 2}));

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerPlaneAlpha) {
    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        auto layer = std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        layer->setColor(RED);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setAlpha(0);
        layer->setBlendMode(BlendMode::PREMULTIPLIED);

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());

        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerSourceCrop) {
    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight() / 4}, RED);
        ReadbackHelper::fillColorsArea(
                expectedColors, getDisplayWidth(),
                {0, getDisplayHeight() / 2, getDisplayWidth(), getDisplayHeight()}, BLUE);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, *mTestRenderEngine,
                                                       getPrimaryDisplayId(), getDisplayWidth(),
                                                       getDisplayHeight(), PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);
        layer->setSourceCrop({0, static_cast<float>(getDisplayHeight() / 2),
                              static_cast<float>(getDisplayWidth()),
                              static_cast<float>(getDisplayHeight())});
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(expectedColors));

        std::vector<std::shared_ptr<TestLayer>> layers = {layer};

        // update expected colors to match crop
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight()}, BLUE);
        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        common::Rect redRect = {0, 0, getDisplayWidth(), getDisplayHeight() / 2};
        common::Rect blueRect = {0, getDisplayHeight() / 4, getDisplayWidth(), getDisplayHeight()};
        auto redLayer = std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        redLayer->setColor(RED);
        redLayer->setDisplayFrame(redRect);

        auto blueLayer = std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        blueLayer->setColor(BLUE);
        blueLayer->setDisplayFrame(blueRect);
        blueLayer->setZOrder(5);

        std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, blueLayer};
        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        // red in front of blue
        redLayer->setZOrder(10);

        // fill blue first so that red will overwrite on overlap
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), blueRect, BLUE);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), redRect, RED);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));

        redLayer->setZOrder(1);
        ReadbackHelper::clearColors(expectedColors, getDisplayWidth(), getDisplayHeight(),
                                    getDisplayWidth());
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), redRect, RED);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), blueRect, BLUE);

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty());
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
        mTestRenderEngine->setRenderLayers(layers);
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->drawLayers());
        ASSERT_NO_FATAL_FAILURE(mTestRenderEngine->checkColorBuffer(expectedColors));
    }
}

TEST_P(GraphicsCompositionTest, SetLayerBrightnessDims) {
    const auto& [status, capabilities] =
            mComposerClient->getDisplayCapabilities(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    const bool brightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                             DisplayCapability::BRIGHTNESS) != capabilities.end();

    if (!brightnessSupport) {
        GTEST_SUCCEED() << "Cannot verify dimming behavior without brightness support";
        return;
    }

    const std::optional<float> maxBrightnessNitsOptional =
            getMaxDisplayBrightnessNits(getPrimaryDisplayId());

    ASSERT_TRUE(maxBrightnessNitsOptional.has_value());

    const float maxBrightnessNits = *maxBrightnessNitsOptional;

    // Preconditions to successfully run are knowing the max brightness and successfully applying
    // the max brightness
    ASSERT_GT(maxBrightnessNits, 0.f);
    mWriter->setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 1.f, maxBrightnessNits);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace for "
                               "color mode: "
                            << toString(mode);
            continue;
        }
        const common::Rect redRect = {0, 0, getDisplayWidth(), getDisplayHeight() / 2};
        const common::Rect dimmerRedRect = {0, getDisplayHeight() / 2, getDisplayWidth(),
                                            getDisplayHeight()};
        const auto redLayer =
                std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        redLayer->setColor(RED);
        redLayer->setDisplayFrame(redRect);
        redLayer->setWhitePointNits(maxBrightnessNits);
        redLayer->setBrightness(1.f);

        const auto dimmerRedLayer =
                std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        dimmerRedLayer->setColor(RED);
        dimmerRedLayer->setDisplayFrame(dimmerRedRect);
        // Intentionally use a small dimming ratio as some implementations may be more likely to
        // kick into GPU composition to apply dithering when the dimming ratio is high.
        static constexpr float kDimmingRatio = 0.9f;
        dimmerRedLayer->setWhitePointNits(maxBrightnessNits * kDimmingRatio);
        dimmerRedLayer->setBrightness(kDimmingRatio);

        const std::vector<std::shared_ptr<TestLayer>> layers = {redLayer, dimmerRedLayer};
        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), redRect, RED);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(), dimmerRedRect, DIM_RED);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        writeLayers(layers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED()
                    << "Readback verification not supported for GPU composition for color mode: "
                    << toString(mode);
            continue;
        }
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        // TODO(b/219590743) we should remove the below SRGB color mode
        // once we have the BlendMode test fix for all the versions of the ColorMode
        mTestColorModes.erase(
                std::remove_if(mTestColorModes.begin(), mTestColorModes.end(),
                               [](ColorMode mode) { return mode != ColorMode::SRGB; }),
                mTestColorModes.end());
        mBackgroundColor = BLACK;
        mTopLayerColor = RED;
    }

    void setBackgroundColor(Color color) { mBackgroundColor = color; }

    void setTopLayerColor(Color color) { mTopLayerColor = color; }

    void setUpLayers(BlendMode blendMode) {
        mLayers.clear();
        std::vector<Color> topLayerPixelColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(topLayerPixelColors, getDisplayWidth(),
                                       {0, 0, getDisplayWidth(), getDisplayHeight()},
                                       mTopLayerColor);

        auto backgroundLayer =
                std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        backgroundLayer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        backgroundLayer->setZOrder(0);
        backgroundLayer->setColor(mBackgroundColor);

        auto layer = std::make_shared<TestBufferLayer>(mComposerClient, *mTestRenderEngine,
                                                       getPrimaryDisplayId(), getDisplayWidth(),
                                                       getDisplayHeight(), PixelFormat::RGBA_8888);
        layer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        layer->setZOrder(10);
        layer->setDataspace(Dataspace::UNKNOWN, *mWriter);
        ASSERT_NO_FATAL_FAILURE(layer->setBuffer(topLayerPixelColors));

        layer->setBlendMode(blendMode);
        layer->setAlpha(std::stof(std::get<1>(GetParam())));

        mLayers.push_back(backgroundLayer);
        mLayers.push_back(layer);
    }

    void setExpectedColors(std::vector<Color>& expectedColors) {
        ASSERT_EQ(2, mLayers.size());
        ReadbackHelper::clearColors(expectedColors, getDisplayWidth(), getDisplayHeight(),
                                    getDisplayWidth());

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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(BlendMode::NONE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);

        setUpLayers(BlendMode::COVERAGE);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.checkReadbackBuffer(expectedColors));
    }
}

TEST_P(GraphicsBlendModeCompositionTest, Premultiplied) {
    for (ColorMode mode : mTestColorModes) {
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));

        setBackgroundColor(BLACK);
        setTopLayerColor(TRANSLUCENT_RED);
        setUpLayers(BlendMode::PREMULTIPLIED);
        setExpectedColors(expectedColors);

        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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

        auto backgroundLayer =
                std::make_shared<TestColorLayer>(mComposerClient, getPrimaryDisplayId());
        backgroundLayer->setColor({0.0f, 0.0f, 0.0f, 0.0f});
        backgroundLayer->setDisplayFrame({0, 0, getDisplayWidth(), getDisplayHeight()});
        backgroundLayer->setZOrder(0);

        mSideLength =
                getDisplayWidth() < getDisplayHeight() ? getDisplayWidth() : getDisplayHeight();
        common::Rect redRect = {0, 0, mSideLength / 2, mSideLength / 2};
        common::Rect blueRect = {mSideLength / 2, mSideLength / 2, mSideLength, mSideLength};

        mLayer = std::make_shared<TestBufferLayer>(mComposerClient, *mTestRenderEngine,
                                                   getPrimaryDisplayId(), mSideLength, mSideLength,
                                                   PixelFormat::RGBA_8888);
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
        auto status = mComposerClient->setColorMode(getPrimaryDisplayId(), mode,
                                                    RenderIntent::COLORIMETRIC);
        if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
            (status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED ||
             status.getServiceSpecificError() == IComposerClient::EX_BAD_PARAMETER)) {
            SUCCEED() << "ColorMode not supported, skip test";
            return;
        }

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());
        mLayer->setTransform(Transform::FLIP_H);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::FLIP_V);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, mSideLength / 2, mSideLength / 2, mSideLength}, RED);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {mSideLength / 2, 0, mSideLength, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(getPrimaryDisplayId(), mode, RenderIntent::COLORIMETRIC)
                            .isOk());

        bool isSupported;
        ASSERT_NO_FATAL_FAILURE(isSupported = getHasReadbackBuffer());
        if (!isSupported) {
            GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
            return;
        }
        ReadbackBuffer readbackBuffer(getPrimaryDisplayId(), mComposerClient, getDisplayWidth(),
                                      getDisplayHeight(), mPixelFormat, mDataspace);
        ASSERT_NO_FATAL_FAILURE(readbackBuffer.setReadbackBuffer());

        mLayer->setTransform(Transform::ROT_180);
        mLayer->setDataspace(ReadbackHelper::getDataspaceForColorMode(mode), *mWriter);

        std::vector<Color> expectedColors(
                static_cast<size_t>(getDisplayWidth() * getDisplayHeight()));
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {mSideLength / 2, mSideLength / 2, mSideLength, mSideLength},
                                       RED);
        ReadbackHelper::fillColorsArea(expectedColors, getDisplayWidth(),
                                       {0, 0, mSideLength / 2, mSideLength / 2}, BLUE);

        writeLayers(mLayers);
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED();
            return;
        }
        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter->presentDisplay(getPrimaryDisplayId());
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
