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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <composer-vts/2.2/ReadbackVts.h>
#include <composer-vts/2.2/RenderEngineVts.h>
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

// Test environment for graphics.composer
class GraphicsComposerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static GraphicsComposerHidlEnvironment* Instance() {
        static GraphicsComposerHidlEnvironment* instance = new GraphicsComposerHidlEnvironment;
        return instance;
    }
    virtual void registerTestServices() override { registerTestService<IComposer>(); }

  private:
    GraphicsComposerHidlEnvironment() {}
    GTEST_DISALLOW_COPY_AND_ASSIGN_(GraphicsComposerHidlEnvironment);
};

class GraphicsCompositionComparisonTest : public ::testing::VtsHalHidlTargetTestBase {
  protected:
    using PowerMode = V2_1::IComposerClient::PowerMode;
    void SetUp() override {
        VtsHalHidlTargetTestBase::SetUp();
        ASSERT_NO_FATAL_FAILURE(
                mComposer = std::make_unique<Composer>(
                        GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
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
                        PixelFormat::RGBA_8888,
                        renderengine::RenderEngine::USE_COLOR_MANAGEMENT |
                                renderengine::RenderEngine::USE_HIGH_PRIORITY_CONTEXT)));

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
        VtsHalHidlTargetTestBase::TearDown();
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

TEST_F(GraphicsCompositionComparisonTest, SingleSolidColorLayer) {
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

}  // namespace
}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
