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

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <mapper-vts/2.0/MapperVts.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {
namespace {

using common::V1_0::BufferUsage;
using common::V1_0::ColorTransform;
using common::V1_0::Transform;
using common::V1_1::ColorMode;
using common::V1_1::Dataspace;
using common::V1_1::PixelFormat;
using common::V1_1::RenderIntent;
using mapper::V2_0::IMapper;

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

        Config config = mComposerClient->getActiveConfig(mPrimaryDisplay);
        mDisplayWidth = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        mDisplayHeight = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);

        mComposerClient->getRaw()->getReadbackBufferAttributes(
            mPrimaryDisplay,
            [&](const auto& tmpError, const auto& tmpPixelFormat, const auto& tmpDataspace) {
                mHasReadbackBuffer = tmpError == Error::NONE;
                if (mHasReadbackBuffer) {
                    mReadbackPixelFormat = tmpPixelFormat;
                    mReadbackDataspace = tmpDataspace;
                    ASSERT_LT(static_cast<PixelFormat>(0), mReadbackPixelFormat);
                    ASSERT_NE(Dataspace::UNKNOWN, mReadbackDataspace);
                }
            });

        mInvalidDisplayId = GetInvalidDisplayId();
    }

    void TearDown() override {
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

    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;

    std::unique_ptr<Composer> mComposer;
    std::unique_ptr<ComposerClient> mComposerClient;
    sp<V2_1::vts::GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;

    bool mHasReadbackBuffer;

    uint64_t mInvalidDisplayId;
    PixelFormat mReadbackPixelFormat;
    Dataspace mReadbackDataspace;

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
        uint64_t usage =
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN);
        return mGralloc->allocate(/*width*/ 64, /*height*/ 64, /*layerCount*/ 1,
                                  PixelFormat::RGBA_8888, usage);
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;

   private:
    std::unique_ptr<Gralloc> mGralloc;
};

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
 * Test IComposerClient::getPerFrameMetadataKeys.
 */
TEST_F(GraphicsComposerHidlTest, GetPerFrameMetadataKeys) {
    std::vector<IComposerClient::PerFrameMetadataKey> keys;
    Error error = Error::NONE;
    mComposerClient->getRaw()->getPerFrameMetadataKeys(
            mPrimaryDisplay, [&](const auto& tmpError, const auto& tmpKeys) {
                error = tmpError;
                keys = tmpKeys;
            });
    if (error == Error::UNSUPPORTED) {
        GTEST_SUCCEED() << "getPerFrameMetadataKeys is not supported";
        return;
    }
    ASSERT_EQ(Error::NONE, error);
    ASSERT_TRUE(keys.size() >= 0);
}

/**
 * Test IComposerClient::createVirtualDisplay_2_2 and
 * IComposerClient::destroyVirtualDisplay.
 *
 * Test that virtual displays can be created and has the correct display type.
 */
TEST_F(GraphicsComposerHidlTest, CreateVirtualDisplay_2_2) {
    if (mComposerClient->getMaxVirtualDisplayCount() == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    Display display;
    PixelFormat format;
    ASSERT_NO_FATAL_FAILURE(
        display = mComposerClient->createVirtualDisplay_2_2(
            64, 64, PixelFormat::IMPLEMENTATION_DEFINED, kBufferSlotCount, &format));

    // test display type
    IComposerClient::DisplayType type = mComposerClient->getDisplayType(display);
    EXPECT_EQ(IComposerClient::DisplayType::VIRTUAL, type);

    mComposerClient->destroyVirtualDisplay(display);
}

/**
 * Test IComposerClient::getClientTargetSupport_2_2.
 *
 * Test that IComposerClient::getClientTargetSupport returns true for the
 * required client targets.
 */
TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport_2_2) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        ASSERT_TRUE(mComposerClient->getClientTargetSupport_2_2(
            mPrimaryDisplay, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN));
    }
}

/**
 * Test IComposerClient::getClientTargetSupport_2_2
 *
 * Test that IComposerClient::getClientTargetSupport_2_2 returns
 * Error::BAD_DISPLAY when passed in an invalid display handle
 */

TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport_2_2BadDisplay) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        Error error = mComposerClient->getRaw()->getClientTargetSupport_2_2(
            mInvalidDisplayId, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN);

        EXPECT_EQ(Error::BAD_DISPLAY, error);
    }
}

/**
 * Test IComposerClient::setPowerMode_2_2.
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode_2_2) {
    std::vector<IComposerClient::PowerMode> modes;
    modes.push_back(IComposerClient::PowerMode::OFF);
    modes.push_back(IComposerClient::PowerMode::ON_SUSPEND);
    modes.push_back(IComposerClient::PowerMode::ON);

    for (auto mode : modes) {
        mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode);
    }
}

/**
 * Test IComposerClient::setPowerMode_2_2
 *
 * Test that IComposerClient::setPowerMode_2_2 succeeds for different varations
 * of PowerMode
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode_2_2Variations) {
    std::vector<IComposerClient::PowerMode> modes;

    modes.push_back(IComposerClient::PowerMode::OFF);
    modes.push_back(IComposerClient::PowerMode::OFF);

    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode));
    }

    modes.clear();

    modes.push_back(IComposerClient::PowerMode::ON);
    modes.push_back(IComposerClient::PowerMode::ON);

    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode));
    }

    modes.clear();

    modes.push_back(IComposerClient::PowerMode::ON_SUSPEND);
    modes.push_back(IComposerClient::PowerMode::ON_SUSPEND);

    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode));
    }

    if (mComposerClient->getDozeSupport(mPrimaryDisplay)) {
        modes.clear();

        modes.push_back(IComposerClient::PowerMode::DOZE);
        modes.push_back(IComposerClient::PowerMode::DOZE);

        for (auto mode : modes) {
            ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode));
        }

        modes.clear();

        modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);
        modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);

        for (auto mode : modes) {
            ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode_2_2(mPrimaryDisplay, mode));
        }
    }
}

/**
 * Test IComposerClient::setPowerMode_2_2
 *
 * Tests that IComposerClient::setPowerMode_2_2 returns BAD_DISPLAY when passed an
 * invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode_2_2BadDisplay) {
    Error error = mComposerClient->getRaw()->setPowerMode_2_2(mInvalidDisplayId,
                                                              IComposerClient::PowerMode::ON);
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::setPowerMode_2_2
 *
 * Test that IComposerClient::setPowerMode_2_2 returns BAD_PARAMETER when passed
 * an invalid PowerMode
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode_2_2BadParameter) {
    Error error = mComposerClient->getRaw()->setPowerMode_2_2(
        mPrimaryDisplay, static_cast<IComposerClient::PowerMode>(-1));
    ASSERT_EQ(Error::BAD_PARAMETER, error);
}

/**
 * Test IComposerClient::setPowerMode_2_2
 *
 * Test that IComposerClient::setPowerMode_2_2 returns UNSUPPORTED when passed
 * DOZE or DOZE_SUPPORT on a device that does not support these modes
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode_2_2Unsupported) {
    if (!mComposerClient->getDozeSupport(mPrimaryDisplay)) {
        Error error = mComposerClient->getRaw()->setPowerMode_2_2(mPrimaryDisplay,
                                                                  IComposerClient::PowerMode::DOZE);
        EXPECT_EQ(Error::UNSUPPORTED, error);

        error = mComposerClient->getRaw()->setPowerMode_2_2(
            mPrimaryDisplay, IComposerClient::PowerMode::DOZE_SUSPEND);
        EXPECT_EQ(Error::UNSUPPORTED, error);
    }
}

/**
 * Test IComposerClient::setReadbackBuffer
 *
 * Test IComposerClient::setReadbackBuffer
 */
TEST_F(GraphicsComposerHidlTest, SetReadbackBuffer) {
    if (!mHasReadbackBuffer) {
        return;
    }

    // BufferUsage::COMPOSER_OUTPUT is missing
    uint64_t usage =
            static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY | BufferUsage::CPU_READ_OFTEN);

    std::unique_ptr<Gralloc> gralloc;
    const native_handle_t* buffer;
    ASSERT_NO_FATAL_FAILURE(gralloc = std::make_unique<Gralloc>());
    ASSERT_NO_FATAL_FAILURE(buffer = gralloc->allocate(mDisplayWidth, mDisplayHeight, 1,
                                                       mReadbackPixelFormat, usage));

    mComposerClient->setReadbackBuffer(mPrimaryDisplay, buffer, -1);
}

/**
 * Test IComposerClient::setReadbackBuffer
 *
 * Test that IComposerClient::setReadbackBuffer returns an Error::BAD_DISPLAY
 * when passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, SetReadbackBufferBadDisplay) {
    if (!mHasReadbackBuffer) {
        return;
    }

    uint64_t usage =
            static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY | BufferUsage::CPU_READ_OFTEN);

    std::unique_ptr<Gralloc> gralloc;
    const native_handle_t* buffer;
    ASSERT_NO_FATAL_FAILURE(gralloc = std::make_unique<Gralloc>());
    ASSERT_NO_FATAL_FAILURE(buffer = gralloc->allocate(mDisplayWidth, mDisplayHeight, 1,
                                                       mReadbackPixelFormat, usage));

    Error error = mComposerClient->getRaw()->setReadbackBuffer(mInvalidDisplayId, buffer, nullptr);
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::setReadbackBuffer
 *
 * Test that IComposerClient::setReadbackBuffer returns Error::BAD_PARAMETER
 * when passed an invalid buffer handle
 */
TEST_F(GraphicsComposerHidlTest, SetReadbackBufferBadParameter) {
    if (!mHasReadbackBuffer) {
        return;
    }

    Error error = mComposerClient->getRaw()->setReadbackBuffer(mPrimaryDisplay, nullptr, nullptr);
    ASSERT_EQ(Error::BAD_PARAMETER, error);
}

TEST_F(GraphicsComposerHidlTest, GetReadbackBufferFenceInactive) {
    if (!mHasReadbackBuffer) {
        return;
    }

    mComposerClient->getRaw()->getReadbackBufferFence(
        mPrimaryDisplay, [&](const auto& tmpError, const auto&) {
            ASSERT_EQ(Error::UNSUPPORTED, tmpError) << "readback buffer is active";
        });
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
    mWriter->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
    mWriter->setLayerFloatColor(IComposerClient::FloatColor{1.0, 1.0, 1.0, 1.0});
    mWriter->setLayerFloatColor(IComposerClient::FloatColor{0.0, 0.0, 0.0, 0.0});
    execute();

    if (mReader->mErrors.size() == 2 &&
        static_cast<Error>(mReader->mErrors[0].second) == Error::UNSUPPORTED &&
        static_cast<Error>(mReader->mErrors[1].second) == Error::UNSUPPORTED) {
        mReader->mErrors.clear();
        GTEST_SUCCEED() << "SetLayerFloatColor is not supported";
        return;
    }

    // ensure setting float color on layer with composition type that is not
    // SOLID_COLOR does not fail
    V2_1::Layer clientLayer;
    ASSERT_NO_FATAL_FAILURE(clientLayer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));
    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(clientLayer);
    mWriter->setLayerCompositionType(IComposerClient::Composition::CLIENT);
    mWriter->setLayerFloatColor(IComposerClient::FloatColor{1.0, 1.0, 1.0, 1.0});
    execute();

    // At this point we know that this function is supported so there should be
    // no errors (checked upon TearDown)
}

/**
 * Test IComposerClient::getDataspaceSaturationMatrix.
 */
TEST_F(GraphicsComposerHidlTest, GetDataspaceSaturationMatrix) {
    auto matrix = mComposerClient->getDataspaceSaturationMatrix(Dataspace::SRGB_LINEAR);
    // the last row is known
    ASSERT_EQ(0.0f, matrix[12]);
    ASSERT_EQ(0.0f, matrix[13]);
    ASSERT_EQ(0.0f, matrix[14]);
    ASSERT_EQ(1.0f, matrix[15]);
}

/*
 * Test IComposerClient::getDataspaceSaturationMatrix
 *
 * Test that IComposerClient::getDataspaceSaturationMatrix returns
 * Error::BAD_PARAMETER when passed a dataspace other than
 * Dataspace::SRGB_LINEAR
 */
TEST_F(GraphicsComposerHidlTest, GetDataspaceSaturationMatrixBadParameter) {
    mComposerClient->getRaw()->getDataspaceSaturationMatrix(
        Dataspace::UNKNOWN,
        [&](const auto& tmpError, const auto&) { ASSERT_EQ(Error::BAD_PARAMETER, tmpError); });
}

/**
 * Test IComposerClient::getColorMode_2_2.
 */
TEST_F(GraphicsComposerHidlTest, GetColorMode_2_2) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);

    auto nativeMode = std::find(modes.cbegin(), modes.cend(), ColorMode::NATIVE);
    EXPECT_NE(modes.cend(), nativeMode);
}

/*
 * Test IComposerClient::getColorMode_2_2
 *
 * Test that IComposerClient::getColorMode returns Error::BAD_DISPLAY when
 * passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetColorMode_2_2BadDisplay) {
    mComposerClient->getRaw()->getColorModes_2_2(
        mInvalidDisplayId,
        [&](const auto& tmpError, const auto&) { ASSERT_EQ(Error::BAD_DISPLAY, tmpError); });
}

/**
 * Test IComposerClient::getRenderIntents.
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntents) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    for (auto mode : modes) {
        std::vector<RenderIntent> intents =
            mComposerClient->getRenderIntents(mPrimaryDisplay, mode);

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
 * Test IComposerClient::getRenderIntents
 *
 * Test that IComposerClient::getRenderIntent returns Error::BAD_DISPLAY when
 * passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntentsBadDisplay) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    for (auto mode : modes) {
        mComposerClient->getRaw()->getRenderIntents(
            mInvalidDisplayId, mode,
            [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::BAD_DISPLAY, tmpError); });
    }
}

/*
 * Test IComposerClient::getRenderIntents
 *
 * Test that IComposerClient::getRenderIntents returns Error::BAD_PARAMETER when
 * pased either an invalid Color mode or an invalid Render Intent
 */
TEST_F(GraphicsComposerHidlTest, GetRenderIntentsBadParameter) {
    mComposerClient->getRaw()->getRenderIntents(
        mPrimaryDisplay, static_cast<ColorMode>(-1),
        [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::BAD_PARAMETER, tmpError); });
}

/**
 * Test IComposerClient::setColorMode_2_2.
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_2) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    for (auto mode : modes) {
        std::vector<RenderIntent> intents =
            mComposerClient->getRenderIntents(mPrimaryDisplay, mode);
        for (auto intent : intents) {
            mComposerClient->setColorMode(mPrimaryDisplay, mode, intent);
        }
    }

    mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE, RenderIntent::COLORIMETRIC);
}

/*
 * Test IComposerClient::setColorMode_2_2
 *
 * Test that IComposerClient::setColorMode_2_2 returns an Error::BAD_DISPLAY
 * when passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_2BadDisplay) {
    Error error = mComposerClient->getRaw()->setColorMode_2_2(mInvalidDisplayId, ColorMode::NATIVE,
                                                              RenderIntent::COLORIMETRIC);

    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/*
 * Test IComposerClient::setColorMode_2_2
 *
 * Test that IComposerClient::setColorMode_2_2 returns Error::BAD_PARAMETER when
 * passed an invalid Color mode or an invalid render intent
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode_2_2BadParameter) {
    Error colorModeError = mComposerClient->getRaw()->setColorMode_2_2(
        mPrimaryDisplay, static_cast<ColorMode>(-1), RenderIntent::COLORIMETRIC);
    EXPECT_EQ(Error::BAD_PARAMETER, colorModeError);

    Error renderIntentError = mComposerClient->getRaw()->setColorMode_2_2(
        mPrimaryDisplay, ColorMode::NATIVE, static_cast<RenderIntent>(-1));
    EXPECT_EQ(Error::BAD_PARAMETER, renderIntentError);
}

}  // namespace
}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::composer::V2_2::vts::GraphicsComposerHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsComposerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsComposerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
