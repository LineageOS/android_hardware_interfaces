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

#define LOG_TAG "graphics_composer_hidl_hal_test@2.3"

#include <algorithm>

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <composer-command-buffer/2.3/ComposerCommandBuffer.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.3/ComposerVts.h>
#include <mapper-vts/2.0/MapperVts.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace vts {
namespace {

using common::V1_0::BufferUsage;
using common::V1_1::PixelFormat;
using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using mapper::V2_0::IMapper;
using mapper::V2_0::vts::Gralloc;

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
            mComposer = std::make_unique<Composer>(
                GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());

        mComposerCallback = new V2_1::vts::GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();

        mInvalidDisplayId = GetInvalidDisplayId();

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::vts::TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_EQ(0, mReader->mCompositionChanges.size());
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    Display GetInvalidDisplayId() {
        std::vector<Display> validDisplays = mComposerCallback->getDisplays();
        uint64_t id = std::numeric_limits<uint64_t>::max();
        while (id > 0) {
            if (std::find(validDisplays.begin(), validDisplays.end(), id) == validDisplays.end()) {
                return id;
            }
            id--;
        }

        return 0;
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;

    std::unique_ptr<Composer> mComposer;
    std::unique_ptr<ComposerClient> mComposerClient;
    sp<V2_1::vts::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    Display mInvalidDisplayId;
    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;

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

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::vts::TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown());
    }

    const native_handle_t* allocate() {
        IMapper::BufferDescriptorInfo info{};
        info.width = 64;
        info.height = 64;
        info.layerCount = 1;
        info.format = static_cast<common::V1_0::PixelFormat>(PixelFormat::RGBA_8888);
        info.usage =
            static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);

        return mGralloc->allocate(info);
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;

   private:
    std::unique_ptr<Gralloc> mGralloc;
};

/**
 * Test IComposerClient::getDisplayIdentificationData.
 *
 * TODO: Check that ports are unique for multiple displays.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayIdentificationData) {
    uint8_t port0;
    std::vector<uint8_t> data0;
    if (mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &port0, &data0)) {
        uint8_t port1;
        std::vector<uint8_t> data1;
        ASSERT_TRUE(mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &port1, &data1));

        ASSERT_EQ(port0, port1) << "ports are not stable";
        ASSERT_TRUE(data0.size() == data1.size() &&
                    std::equal(data0.begin(), data0.end(), data1.begin()))
            << "data is not stable";
    }
}

/**
 * Test IComposerClient::Command::SET_LAYER_PER_FRAME_METADATA.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_PER_FRAME_METADATA) {
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
    mWriter->setLayerPerFrameMetadata(hidlMetadata);
    execute();

    if (mReader->mErrors.size() == 1 &&
        static_cast<Error>(mReader->mErrors[0].second) == Error::UNSUPPORTED) {
        mReader->mErrors.clear();
        GTEST_SUCCEED() << "SetLayerPerFrameMetadata is not supported";
        ASSERT_NO_FATAL_FAILURE(mComposerClient->destroyLayer(mPrimaryDisplay, layer));
        return;
    }

    ASSERT_NO_FATAL_FAILURE(mComposerClient->destroyLayer(mPrimaryDisplay, layer));
}

/**
 * Test IComposerClient::getHdrCapabilities_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetHdrCapabilities_2_3) {
    float maxLuminance;
    float maxAverageLuminance;
    float minLuminance;
    ASSERT_NO_FATAL_FAILURE(mComposerClient->getHdrCapabilities_2_3(
        mPrimaryDisplay, &maxLuminance, &maxAverageLuminance, &minLuminance));
    ASSERT_TRUE(maxLuminance >= minLuminance);
}

/**
 * Test IComposerClient::getPerFrameMetadataKeys_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetPerFrameMetadataKeys_2_3) {
    std::vector<IComposerClient::PerFrameMetadataKey> keys;
    mComposerClient->getRaw()->getPerFrameMetadataKeys_2_3(
        mPrimaryDisplay, [&](const auto tmpError, const auto outKeys) {
            if (tmpError != Error::UNSUPPORTED) {
                ASSERT_EQ(Error::NONE, tmpError);
                keys = outKeys;
            }
        });
}

/**
 * TestIComposerClient::getReadbackBufferAttributes_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetReadbackBufferAttributes_2_3) {
    Dataspace dataspace;
    PixelFormat pixelFormat;

    mComposerClient->getRaw()->getReadbackBufferAttributes_2_3(
        mPrimaryDisplay,
        [&](const auto tmpError, const auto outPixelFormat, const auto outDataspace) {
            if (tmpError != Error::UNSUPPORTED) {
                ASSERT_EQ(Error::NONE, tmpError);
                dataspace = outDataspace;
                pixelFormat = outPixelFormat;
            }
        });
}

/**
 * Test IComposerClient::getClientTargetSupport_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport_2_3) {
    std::vector<V2_1::Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        ASSERT_TRUE(mComposerClient->getClientTargetSupport_2_3(
            mPrimaryDisplay, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN));
    }
}
/**
 * Test IComposerClient::getClientTargetSupport_2_3
 *
 * Test that IComposerClient::getClientTargetSupport_2_3 returns
 * Error::BAD_DISPLAY when passed in an invalid display handle
 */

TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport_2_3BadDisplay) {
    std::vector<V2_1::Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        Error error = mComposerClient->getRaw()->getClientTargetSupport_2_3(
            mInvalidDisplayId, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN);

        EXPECT_EQ(Error::BAD_DISPLAY, error);
    }
}

/**
 * Test IComposerClient::getRenderIntents_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntents_2_3) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes_2_3(mPrimaryDisplay);
    for (auto mode : modes) {
        std::vector<RenderIntent> intents =
            mComposerClient->getRenderIntents_2_3(mPrimaryDisplay, mode);

        bool isHdr;
        switch (mode) {
            case ColorMode::BT2100_PQ:
            case ColorMode::BT2100_HLG:
                isHdr = true;
                break;
            default:
                isHdr = false;
                break;
        }
        RenderIntent requiredIntent =
            isHdr ? RenderIntent::TONE_MAP_COLORIMETRIC : RenderIntent::COLORIMETRIC;

        auto iter = std::find(intents.cbegin(), intents.cend(), requiredIntent);
        EXPECT_NE(intents.cend(), iter);
    }
}

/*
 * Test IComposerClient::getRenderIntents_2_3
 *
 * Test that IComposerClient::getRenderIntents_2_3 returns Error::BAD_DISPLAY when
 * passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntents_2_3BadDisplay) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes_2_3(mPrimaryDisplay);
    for (auto mode : modes) {
        mComposerClient->getRaw()->getRenderIntents_2_3(
            mInvalidDisplayId, mode,
            [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::BAD_DISPLAY, tmpError); });
    }
}

/*
 * Test IComposerClient::getRenderIntents_2_3
 *
 * Test that IComposerClient::getRenderIntents_2_3 returns Error::BAD_PARAMETER when
 * pased either an invalid Color mode or an invalid Render Intent
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntents_2_3BadParameter) {
    mComposerClient->getRaw()->getRenderIntents_2_3(
        mPrimaryDisplay, static_cast<ColorMode>(-1),
        [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::BAD_PARAMETER, tmpError); });
}

/**
 * IComposerClient::getColorModes_2_3
 */
TEST_F(GraphicsComposerHidlTest, GetColorModes_2_3) {
    std::vector<ColorMode> colorModes = mComposerClient->getColorModes_2_3(mPrimaryDisplay);

    auto native = std::find(colorModes.cbegin(), colorModes.cend(), ColorMode::NATIVE);
    ASSERT_NE(colorModes.cend(), native);
}

/*
 * Test IComposerClient::getColorModes_2_3
 *
 * Test that IComposerClient::getColorModes_2_3 returns Error::BAD_DISPLAY when
 * passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetColorMode_2_3BadDisplay) {
    mComposerClient->getRaw()->getColorModes_2_3(
        mInvalidDisplayId,
        [&](const auto& tmpError, const auto&) { ASSERT_EQ(Error::BAD_DISPLAY, tmpError); });
}

/**
 * IComposerClient::setColorMode_2_3
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_3) {
    std::vector<ColorMode> colorModes = mComposerClient->getColorModes_2_3(mPrimaryDisplay);
    for (auto mode : colorModes) {
        std::vector<RenderIntent> intents =
            mComposerClient->getRenderIntents_2_3(mPrimaryDisplay, mode);
        for (auto intent : intents) {
            ASSERT_NO_FATAL_FAILURE(
                mComposerClient->setColorMode_2_3(mPrimaryDisplay, mode, intent));
        }
    }

    ASSERT_NO_FATAL_FAILURE(mComposerClient->setColorMode_2_3(mPrimaryDisplay, ColorMode::NATIVE,
                                                              RenderIntent::COLORIMETRIC));
}

/*
 * Test IComposerClient::setColorMode_2_3
 *
 * Test that IComposerClient::setColorMode_2_3 returns an Error::BAD_DISPLAY
 * when passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_3BadDisplay) {
    Error error = mComposerClient->getRaw()->setColorMode_2_3(mInvalidDisplayId, ColorMode::NATIVE,
                                                              RenderIntent::COLORIMETRIC);

    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/*
 * Test IComposerClient::setColorMode_2_3
 *
 * Test that IComposerClient::setColorMode_2_3 returns Error::BAD_PARAMETER when
 * passed an invalid Color mode or an invalid render intent
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_3BadParameter) {
    Error colorModeError = mComposerClient->getRaw()->setColorMode_2_3(
        mPrimaryDisplay, static_cast<ColorMode>(-1), RenderIntent::COLORIMETRIC);
    EXPECT_EQ(Error::BAD_PARAMETER, colorModeError);

    Error renderIntentError = mComposerClient->getRaw()->setColorMode_2_3(
        mPrimaryDisplay, ColorMode::NATIVE, static_cast<RenderIntent>(-1));
    EXPECT_EQ(Error::BAD_PARAMETER, renderIntentError);
}

/**
 * Test IComposerClient::Command::SET_LAYER_COLOR_TRANSFORM.
 * TODO Add color to the layer, use matrix to keep only red component,
 * and check.
 */
TEST_F(GraphicsComposerHidlTest, SetLayerColorTransform) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));
    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);

    // clang-format off
    const std::array<float, 16> matrix = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    // clang-format on

    mWriter->setLayerColorTransform(matrix.data());
    execute();

    if (mReader->mErrors.size() == 1 &&
        static_cast<Error>(mReader->mErrors[0].second) == Error::UNSUPPORTED) {
        mReader->mErrors.clear();
        GTEST_SUCCEED() << "setLayerColorTransform is not supported";
        return;
    }
}

TEST_F(GraphicsComposerHidlTest, GetDisplayedContentSamplingAttributes) {
    using common::V1_1::PixelFormat;
    using common::V1_2::Dataspace;

    int constexpr invalid = -1;
    auto format = static_cast<PixelFormat>(invalid);
    auto dataspace = static_cast<Dataspace>(invalid);
    auto componentMask = static_cast<hidl_bitfield<IComposerClient::FormatColorComponent>>(invalid);
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(mPrimaryDisplay, format,
                                                                        dataspace, componentMask);

    if (error == Error::UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_EQ(error, Error::NONE);
    EXPECT_NE(format, static_cast<PixelFormat>(invalid));
    EXPECT_NE(dataspace, static_cast<Dataspace>(invalid));
    EXPECT_NE(componentMask,
              static_cast<hidl_bitfield<IComposerClient::FormatColorComponent>>(invalid));
};

TEST_F(GraphicsComposerHidlTest, SetDisplayedContentSamplingEnabled) {
    auto const maxFrames = 10;
    auto const enableAllComponents = 0;
    auto error = mComposerClient->setDisplayedContentSamplingEnabled(
        mPrimaryDisplay, IComposerClient::DisplayedContentSampling::ENABLE, enableAllComponents,
        maxFrames);
    if (error == Error::UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }
    EXPECT_EQ(error, Error::NONE);

    error = mComposerClient->setDisplayedContentSamplingEnabled(
        mPrimaryDisplay, IComposerClient::DisplayedContentSampling::DISABLE, enableAllComponents,
        maxFrames);
    EXPECT_EQ(error, Error::NONE);
}

TEST_F(GraphicsComposerHidlTest, GetDisplayedContentSample) {
    int constexpr invalid = -1;
    auto format = static_cast<PixelFormat>(invalid);
    auto dataspace = static_cast<Dataspace>(invalid);
    auto componentMask = static_cast<hidl_bitfield<IComposerClient::FormatColorComponent>>(invalid);
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(mPrimaryDisplay, format,
                                                                        dataspace, componentMask);

    uint64_t maxFrames = 10;
    uint64_t timestamp = 0;
    uint64_t frameCount = 0;
    hidl_array<hidl_vec<uint64_t>, 4> histogram;
    error = mComposerClient->getDisplayedContentSample(mPrimaryDisplay, maxFrames, timestamp,
                                                       frameCount, histogram[0], histogram[1],
                                                       histogram[2], histogram[3]);
    if (error == Error::UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_EQ(error, Error::NONE);
    EXPECT_LE(frameCount, maxFrames);
    for (auto i = 0; i < histogram.size(); i++) {
        if (componentMask & (1 << i)) {
            EXPECT_NE(histogram[i].size(), 0);
        } else {
            EXPECT_EQ(histogram[i].size(), 0);
        }
    }
}

/*
 * getDisplayCapabilities is required in composer 2.3
 * Test some constraints.
 */
TEST_F(GraphicsComposerHidlTest, getDisplayCapabilitiesBasic) {
    auto capabilities = mComposerClient->getDisplayCapabilities(mPrimaryDisplay);
    bool hasDozeSupport = std::find(capabilities.begin(), capabilities.end(),
                                    IComposerClient::DisplayCapability::DOZE) != capabilities.end();
    EXPECT_EQ(mComposerClient->getDozeSupport(mPrimaryDisplay), hasDozeSupport);
}

TEST_F(GraphicsComposerHidlTest, getDisplayCapabilitiesBadDisplay) {
    mComposerClient->getRaw()->getDisplayCapabilities(
        mInvalidDisplayId,
        [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::BAD_DISPLAY, tmpError); });
}

TEST_F(GraphicsComposerHidlTest, SetLayerPerFrameMetadataBlobs) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);

    std::vector<IComposerClient::PerFrameMetadataBlob> metadata;
    metadata.push_back(
        {IComposerClient::PerFrameMetadataKey::HDR10_PLUS_SEI, std::vector<uint8_t>(1, 0xff)});

    mWriter->setLayerPerFrameMetadataBlobs(metadata);
    execute();

    if (mReader->mErrors.size() == 1 &&
        static_cast<Error>(mReader->mErrors[0].second) == Error::UNSUPPORTED) {
        mReader->mErrors.clear();
        GTEST_SUCCEED() << "setLayerDynamicPerFrameMetadata is not supported";
        return;
    }
}

}  // namespace
}  // namespace vts
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::composer::V2_3::vts::GraphicsComposerHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsComposerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsComposerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
