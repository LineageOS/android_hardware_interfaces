/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "graphics_composer_hidl_hal_test@2.2"

#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <mapper-vts/2.0/MapperVts.h>
#include <sync/sync.h>
#include "2.2/VtsHalGraphicsComposerTestUtils.h"
#include "GraphicsComposerCallback.h"
#include "TestCommandReader.h"
#include "VtsHalGraphicsComposerTestUtils.h"

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace tests {
namespace {

using android::hardware::graphics::common::V1_0::BufferUsage;
using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;
using android::hardware::graphics::composer::V2_2::IComposerClient;
using android::hardware::graphics::mapper::V2_0::IMapper;
using android::hardware::graphics::mapper::V2_0::vts::Gralloc;
using GrallocError = android::hardware::graphics::mapper::V2_0::Error;

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

class GraphicsComposerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
            mComposer = std::make_unique<Composer_v2_2>(
                GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient_v2_2());

        mComposerCallback = new V2_1::tests::GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);
    }

    void TearDown() override {
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;

    std::unique_ptr<Composer_v2_2> mComposer;
    std::unique_ptr<ComposerClient_v2_2> mComposerClient;
    sp<V2_1::tests::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;

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

// Tests for IComposerClient::Command.
class GraphicsComposerHidlCommandTest : public GraphicsComposerHidlTest {
   protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::SetUp());

        ASSERT_NO_FATAL_FAILURE(mGralloc = std::make_unique<Gralloc>());

        mWriter = std::make_unique<V2_2::CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::tests::TestCommandReader>();
    }

    void TearDown() override { ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown()); }

    const native_handle_t* allocate() {
        IMapper::BufferDescriptorInfo info{};
        info.width = 64;
        info.height = 64;
        info.layerCount = 1;
        info.format = PixelFormat::RGBA_8888;
        info.usage =
            static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

        return mGralloc->allocate(info);
    }

    void execute() { mComposerClient->execute_v2_2(mReader.get(), mWriter.get()); }

    std::unique_ptr<V2_2::CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::tests::TestCommandReader> mReader;

   private:
    std::unique_ptr<Gralloc> mGralloc;
};

/**
 * Test IComposerClient::Command::SET_PER_FRAME_METADATA.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_PER_FRAME_METADATA) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);

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

    std::vector<IComposerClient::PerFrameMetadata> hidlMetadata;
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X, 0.680});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y, 0.320});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X, 0.265});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y, 0.690});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X, 0.150});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y, 0.060});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::WHITE_POINT_X, 0.3127});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::WHITE_POINT_Y, 0.3290});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::MAX_LUMINANCE, 100.0});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::MIN_LUMINANCE, 0.1});
    hidlMetadata.push_back({IComposerClient::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL, 78.0});
    hidlMetadata.push_back(
        {IComposerClient::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL, 62.0});
    mWriter->setPerFrameMetadata(hidlMetadata);
    execute();
}

/**
 * Test IComposerClient::getPerFrameMetadataKeys.
 */
TEST_F(GraphicsComposerHidlTest, GetPerFrameMetadataKeys) {
    mComposerClient->getPerFrameMetadataKeys(mPrimaryDisplay);
}
/**
 * Test IComposerClient::setPowerMode_2_2.
 */
TEST_F(GraphicsComposerHidlTest, setPowerMode_2_2) {
    std::vector<IComposerClient::PowerMode> modes;
    modes.push_back(IComposerClient::PowerMode::OFF);
    modes.push_back(IComposerClient::PowerMode::ON_SUSPEND);
    modes.push_back(IComposerClient::PowerMode::ON);

    for (auto mode : modes) {
        mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode);
    }
}

TEST_F(GraphicsComposerHidlTest, setReadbackBuffer) {
    mComposerClient->setReadbackBuffer(mPrimaryDisplay, nullptr, -1);
}

TEST_F(GraphicsComposerHidlTest, getReadbackBufferFence) {
    int32_t fence;
    mComposerClient->getReadbackBufferFence(mPrimaryDisplay, &fence);
}

TEST_F(GraphicsComposerHidlTest, getReadbackBufferAttributes) {
    PixelFormat pixelFormat;
    Dataspace dataspace;
    mComposerClient->getReadbackBufferAttributes(mPrimaryDisplay, &pixelFormat, &dataspace);
}

/**
 * Test IComposerClient::Command::SET_LAYER_FLOAT_COLOR.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_FLOAT_COLOR) {
    V2_1::Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerFloatColor(IComposerClient::FloatColor{1.0, 1.0, 1.0, 1.0});
    mWriter->setLayerFloatColor(IComposerClient::FloatColor{0.0, 0.0, 0.0, 0.0});
}

}  // namespace
}  // namespace tests
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::composer::V2_2::tests::GraphicsComposerHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsComposerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsComposerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
