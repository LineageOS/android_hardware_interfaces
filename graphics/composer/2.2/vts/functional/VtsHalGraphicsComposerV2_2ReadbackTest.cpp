/*
 * Copyright 2018 The Android Open Source Project
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
#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <mapper-vts/2.1/MapperVts.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {
namespace {

using android::hardware::hidl_handle;
using common::V1_1::BufferUsage;
using common::V1_1::Dataspace;
using common::V1_1::PixelFormat;
using mapper::V2_1::IMapper;
using mapper::V2_1::vts::Gralloc;
using V2_1::Display;
using V2_1::Layer;
using V2_1::vts::TestCommandReader;

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

class TestLayer {
   public:
    TestLayer(std::shared_ptr<ComposerClient> const client, Display display)
        : mLayer(client->createLayer(display, kBufferSlotCount)),
          mComposerClient(client),
          mDisplay(display) {}

    virtual ~TestLayer() { mComposerClient->destroyLayer(mDisplay, mLayer); }

    virtual void write(std::shared_ptr<CommandWriterBase> writer) {
        writer->selectLayer(mLayer);
        writer->setLayerDisplayFrame(mDisplayFrame);
        writer->setLayerZOrder(mZOrder);
    }

    void setDisplayFrame(IComposerClient::Rect frame) { mDisplayFrame = frame; }
    void setZOrder(uint32_t z) { mZOrder = z; }

   protected:
    Layer mLayer;
    IComposerClient::Rect mDisplayFrame = {0, 0, 0, 0};
    uint32_t mZOrder = 0;

   private:
    std::shared_ptr<ComposerClient> const mComposerClient;
    const Display mDisplay;
    static constexpr uint32_t kBufferSlotCount = 64;
};

class TestColorLayer : public TestLayer {
   public:
    TestColorLayer(std::shared_ptr<ComposerClient> const client, Display display)
        : TestLayer{client, display} {}

    void write(std::shared_ptr<CommandWriterBase> writer) override {
        TestLayer::write(writer);
        writer->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
        writer->setLayerColor(mColor);
    }

    void setColor(IComposerClient::Color color) { mColor = color; }

   private:
    IComposerClient::Color mColor = {0xff, 0xff, 0xff, 0xff};
};

class GraphicsComposerReadbackTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    using PowerMode = V2_1::IComposerClient::PowerMode;
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
            mComposer = std::make_unique<Composer>(
                GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());
        mComposerCallback = new V2_1::vts::GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();
        Config activeConfig = mComposerClient->getActiveConfig(mPrimaryDisplay);
        mDisplayWidth = mComposerClient->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                             IComposerClient::Attribute::WIDTH);
        mDisplayHeight = mComposerClient->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                              IComposerClient::Attribute::HEIGHT);

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);

        // set up command writer/reader and gralloc
        mWriter = std::make_shared<CommandWriterBase>(1024);
        mReader = std::make_unique<TestCommandReader>();
        mGralloc = std::make_unique<Gralloc>();

        mComposerClient->getRaw()->getReadbackBufferAttributes(
            mPrimaryDisplay, [&](const auto& tmpError, const auto&, const auto&) {
                mHasReadbackBuffer = tmpError == Error::NONE;
            });
    }

    ~GraphicsComposerReadbackTest() override {
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    void render(const std::vector<std::shared_ptr<TestLayer>>& layers) {
        for (auto layer : layers) {
            layer->write(mWriter);
        }
        execute();
        mWriter->validateDisplay();
        mWriter->presentDisplay();
        execute();
    }

    int32_t GetBytesPerPixel(PixelFormat format) {
        switch (format) {
            case PixelFormat::RGBA_8888:
                return 4;
            case PixelFormat::RGB_888:
                return 3;
            default:
                return -1;
        }
    }

    bool readbackSupported(const PixelFormat& pixelFormat, const Dataspace& dataspace,
                           const Error error) {
        if (error == Error::UNSUPPORTED) {
            return false;
        }
        // TODO: add support for RGBA_1010102
        if (pixelFormat != PixelFormat::RGB_888 && pixelFormat != PixelFormat::RGBA_8888) {
            return false;
        }
        if (dataspace != Dataspace::V0_SRGB) {
            return false;
        }
        return true;
    }

    void getReadbackBufferAttributes(Display display, PixelFormat* outPixelFormat,
                                     Dataspace* outDataspace) {
        mComposerClient->getRaw()->getReadbackBufferAttributes(
            display, [&](const auto&, const auto& tmpOutPixelFormat, const auto& tmpOutDataspace) {
                *outPixelFormat = tmpOutPixelFormat;
                *outDataspace = tmpOutDataspace;
            });
    }

    void checkReadbackBuffer(IMapper::BufferDescriptorInfo info, uint32_t stride, void* bufferData,
                             std::vector<IComposerClient::Color> expectedColors) {
        int32_t bytesPerPixel = GetBytesPerPixel(info.format);
        ASSERT_NE(-1, bytesPerPixel)
            << "unexpected pixel format " << static_cast<int32_t>(info.format)
            << "(expected RGBA_8888 or RGB_888)";
        for (int row = 0; row < mDisplayHeight; row++) {
            for (int col = 0; col < mDisplayWidth; col++) {
                int pixel = row * mDisplayWidth + col;
                int offset = (row * stride + col) * bytesPerPixel;
                uint8_t* pixelColor = (uint8_t*)bufferData + offset;

                EXPECT_EQ(expectedColors[pixel].r, pixelColor[0]);
                EXPECT_EQ(expectedColors[pixel].g, pixelColor[1]);
                EXPECT_EQ(expectedColors[pixel].b, pixelColor[2]);
            }
        }
    }

    std::unique_ptr<Composer> mComposer;
    std::shared_ptr<ComposerClient> mComposerClient;

    sp<V2_1::vts::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
    std::shared_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<TestCommandReader> mReader;
    std::unique_ptr<Gralloc> mGralloc;

    bool mHasReadbackBuffer;

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
};

TEST_F(GraphicsComposerReadbackTest, SingleSolidColorLayer) {
    if (!mHasReadbackBuffer) {
        GTEST_SUCCEED() << "Readback not supported or unsupported pixelFormat/dataspace";
        return;
    }
    mWriter->selectDisplay(mPrimaryDisplay);
    mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON);
    mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::SRGB, RenderIntent::COLORIMETRIC);

    auto layer = std::make_shared<TestColorLayer>(mComposerClient, mPrimaryDisplay);
    IComposerClient::Color color({0, 0, 0xff, 0xff});
    IComposerClient::Rect coloredSquare({100, 100, 500, 500});
    layer->setColor(color);
    layer->setDisplayFrame(coloredSquare);
    layer->setZOrder(10);

    std::vector<std::shared_ptr<TestLayer>> layers = {layer};

    // expected color for each pixel
    std::vector<IComposerClient::Color> expectedColors(mDisplayWidth * mDisplayHeight);
    for (int row = 0; row < mDisplayHeight; row++) {
        for (int col = 0; col < mDisplayWidth; col++) {
            int pixel = row * mDisplayWidth + col;
            if (row >= coloredSquare.top && row < coloredSquare.bottom &&
                col >= coloredSquare.left && col < coloredSquare.right) {
                expectedColors[pixel] = color;
            } else {
                expectedColors[pixel] = {0, 0, 0, 0xff};
            }
        }
    }

    PixelFormat pixelFormat;
    Dataspace dataspace;

    getReadbackBufferAttributes(mPrimaryDisplay, &pixelFormat, &dataspace);

    IMapper::BufferDescriptorInfo info;
    info.width = mDisplayWidth;
    info.height = mDisplayHeight;
    info.layerCount = 1;
    info.format = pixelFormat;
    info.usage = static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN | BufferUsage::GPU_TEXTURE);

    uint32_t stride;
    const native_handle_t* buffer = mGralloc->allocate(info, /*import*/ true, &stride);
    mComposerClient->setReadbackBuffer(mPrimaryDisplay, buffer, -1);

    render(layers);

    int32_t fenceHandle;
    mComposerClient->getReadbackBufferFence(mPrimaryDisplay, &fenceHandle);

    // lock buffer
    // Create Rect accessRegion to specify reading the entire buffer
    IMapper::Rect accessRegion;
    accessRegion.left = 0;
    accessRegion.top = 0;
    accessRegion.width = info.width;
    accessRegion.height = info.height;

    void* bufData = mGralloc->lock(buffer, info.usage, accessRegion, fenceHandle);
    checkReadbackBuffer(info, stride, bufData, expectedColors);
    int unlockFence = mGralloc->unlock(buffer);

    if (unlockFence != -1) {
        close(unlockFence);
    }

    mWriter->validateDisplay();
    mWriter->presentDisplay();
    execute();
}

}  // anonymous namespace
}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
