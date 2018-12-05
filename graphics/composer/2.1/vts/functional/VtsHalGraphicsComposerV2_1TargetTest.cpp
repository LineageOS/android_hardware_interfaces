/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "graphics_composer_hidl_hal_test"

#include <android-base/logging.h>
#include <composer-vts/2.1/ComposerVts.h>
#include <composer-vts/2.1/GraphicsComposerCallback.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <mapper-vts/2.0/MapperVts.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace vts {
namespace {

using android::hardware::graphics::common::V1_0::BufferUsage;
using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;
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
        VtsHalHidlTargetTestBase::SetUp();
        ASSERT_NO_FATAL_FAILURE(
            mComposer = std::make_unique<Composer>(
                GraphicsComposerHidlEnvironment::Instance()->getServiceName<IComposer>()));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());

        mComposerCallback = new GraphicsComposerCallback;
        mComposerClient->registerCallback(mComposerCallback);

        // assume the first display is primary and is never removed
        mPrimaryDisplay = waitForFirstDisplay();

        // explicitly disable vsync
        mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);
        mComposerCallback->setVsyncAllowed(false);

        mInvalidDisplayId = GetInvalidDisplayId();

        // Although 0 could be an invalid display, a return value of 0
        // from GetInvalidDisplayId means all other ids are in use, a condition which
        // we are assuming a device will never have
        ASSERT_NE(0, mInvalidDisplayId);
    }

    void TearDown() override {
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
        }
        VtsHalHidlTargetTestBase::TearDown();
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
    sp<GraphicsComposerCallback> mComposerCallback;
    // the first display and is assumed never to be removed
    Display mPrimaryDisplay;
    Display mInvalidDisplayId;

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

/**
 * Test IComposer::getCapabilities.
 *
 * Test that IComposer::getCapabilities returns no invalid capabilities.
 */
TEST_F(GraphicsComposerHidlTest, GetCapabilities) {
    auto capabilities = mComposer->getCapabilities();
    ASSERT_EQ(capabilities.end(),
              std::find(capabilities.begin(), capabilities.end(), IComposer::Capability::INVALID));
}

/**
 * Test IComposer::dumpDebugInfo.
 */
TEST_F(GraphicsComposerHidlTest, DumpDebugInfo) {
    mComposer->dumpDebugInfo();
}

/**
 * Test IComposer::createClient.
 *
 * Test that IComposerClient is a singleton.
 */
TEST_F(GraphicsComposerHidlTest, CreateClientSingleton) {
    mComposer->getRaw()->createClient(
        [&](const auto& tmpError, const auto&) { EXPECT_EQ(Error::NO_RESOURCES, tmpError); });
}

/**
 * Test IComposerClient::createVirtualDisplay and
 * IComposerClient::destroyVirtualDisplay.
 *
 * Test that virtual displays can be created and has the correct display type.
 */
TEST_F(GraphicsComposerHidlTest, CreateVirtualDisplay) {
    if (mComposerClient->getMaxVirtualDisplayCount() == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    Display display;
    PixelFormat format;
    ASSERT_NO_FATAL_FAILURE(
        display = mComposerClient->createVirtualDisplay(64, 64, PixelFormat::IMPLEMENTATION_DEFINED,
                                                        kBufferSlotCount, &format));

    // test display type
    IComposerClient::DisplayType type = mComposerClient->getDisplayType(display);
    EXPECT_EQ(IComposerClient::DisplayType::VIRTUAL, type);

    mComposerClient->destroyVirtualDisplay(display);
}

/**
 * Test IComposerClient::destroyVirtualDisplay
 *
 * Test that passing a bad display handle to destroyVirtualDisplay
 * returns a BAD_DISPLAY error
 */
TEST_F(GraphicsComposerHidlTest, DestroyVirtualDisplayBadDisplay) {
    if (mComposerClient->getMaxVirtualDisplayCount() == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    Error error = mComposerClient->getRaw()->destroyVirtualDisplay(mInvalidDisplayId);
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::createLayer and IComposerClient::destroyLayer.
 *
 * Test that layers can be created and destroyed.
 */
TEST_F(GraphicsComposerHidlTest, CreateLayer) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mComposerClient->destroyLayer(mPrimaryDisplay, layer);
}

/**
 * Test IComposerClient::createLayer
 *
 * Test that passing in an invalid display handle to createLayer returns
 * BAD_DISPLAY.
 */
TEST_F(GraphicsComposerHidlTest, CreateLayerBadDisplay) {
    Error error;
    mComposerClient->getRaw()->createLayer(
        mInvalidDisplayId, kBufferSlotCount,
        [&](const auto& tmpOutError, const auto&) { error = tmpOutError; });
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::destroyLayer
 *
 * Test that passing in an invalid display handle to destroyLayer returns
 * BAD_DISPLAY
 */
TEST_F(GraphicsComposerHidlTest, DestroyLayerBadDisplay) {
    Error error;
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    error = mComposerClient->getRaw()->destroyLayer(mInvalidDisplayId, layer);

    EXPECT_EQ(Error::BAD_DISPLAY, error);

    ASSERT_NO_FATAL_FAILURE(mComposerClient->destroyLayer(mPrimaryDisplay, layer));
}

/**
 * Test IComposerClient::destroyLayer
 *
 * Test that passing in an invalid layer handle to destroyLayer returns
 * BAD_LAYER
 */
TEST_F(GraphicsComposerHidlTest, DestroyLayerBadLayerError) {
    // We haven't created any layers yet, so any id should be invalid
    Error error = mComposerClient->getRaw()->destroyLayer(mPrimaryDisplay, 1);

    EXPECT_EQ(Error::BAD_LAYER, error);
}

/**
 * Test IComposerClient::getActiveConfig
 *
 * Test that passing in a bad display handle to getActiveConfig generates a
 * BAD_DISPLAY error
 */
TEST_F(GraphicsComposerHidlTest, GetActiveConfigBadDisplay) {
    Error error;
    mComposerClient->getRaw()->getActiveConfig(
        mInvalidDisplayId, [&](const auto& tmpOutError, const auto&) { error = tmpOutError; });
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::getDisplayConfigs
 *
 * Test IComposerClient::getDisplayConfigs returns no error
 * when passed in a valid display
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayConfig) {
    std::vector<Config> configs;
    ASSERT_NO_FATAL_FAILURE(configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay));
}

/**
 * Test IComposerClient::getDisplayConfigs
 *
 * Test IComposerClient::getDisplayConfigs returns BAD_DISPLAY
 * when passed in an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayConfigBadDisplay) {
    Error error;
    mComposerClient->getRaw()->getDisplayConfigs(
        mInvalidDisplayId, [&](const auto& tmpOutError, const auto&) { error = tmpOutError; });
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::getDisplayName.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayName) {
    mComposerClient->getDisplayName(mPrimaryDisplay);
}

/**
 * Test IComposerClient::getDisplayType.
 *
 * Test that IComposerClient::getDisplayType returns the correct display type
 * for the primary display.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayType) {
    ASSERT_EQ(IComposerClient::DisplayType::PHYSICAL,
              mComposerClient->getDisplayType(mPrimaryDisplay));
}

/**
 * Test IComposerClient::getClientTargetSupport.
 *
 * Test that IComposerClient::getClientTargetSupport returns true for the
 * required client targets.
 */
TEST_F(GraphicsComposerHidlTest, GetClientTargetSupport) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        ASSERT_TRUE(mComposerClient->getClientTargetSupport(
            mPrimaryDisplay, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN));
    }
}

/**
 * Test IComposerClient::getClientTargetSupport
 *
 * Test that IComposerClient::getClientTargetSupport returns BAD_DISPLAY when
 * passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetClientTargetSupportBadDisplay) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        int32_t width = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                             IComposerClient::Attribute::WIDTH);
        int32_t height = mComposerClient->getDisplayAttribute(mPrimaryDisplay, config,
                                                              IComposerClient::Attribute::HEIGHT);
        ASSERT_LT(0, width);
        ASSERT_LT(0, height);

        mComposerClient->setActiveConfig(mPrimaryDisplay, config);

        Error error = mComposerClient->getRaw()->getClientTargetSupport(
            mInvalidDisplayId, width, height, PixelFormat::RGBA_8888, Dataspace::UNKNOWN);
        EXPECT_EQ(Error::BAD_DISPLAY, error);
    }
}

/**
 * Test IComposerClient::getDisplayAttribute.
 *
 * Test that IComposerClient::getDisplayAttribute succeeds for the required
 * formats, and succeeds or fails correctly for optional attributes.
 */
TEST_F(GraphicsComposerHidlTest, GetDisplayAttribute) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        const std::array<IComposerClient::Attribute, 3> requiredAttributes = {{
            IComposerClient::Attribute::WIDTH, IComposerClient::Attribute::HEIGHT,
            IComposerClient::Attribute::VSYNC_PERIOD,
        }};
        for (auto attribute : requiredAttributes) {
            mComposerClient->getDisplayAttribute(mPrimaryDisplay, config, attribute);
        }

        const std::array<IComposerClient::Attribute, 2> optionalAttributes = {{
            IComposerClient::Attribute::DPI_X, IComposerClient::Attribute::DPI_Y,
        }};
        for (auto attribute : optionalAttributes) {
            mComposerClient->getRaw()->getDisplayAttribute(
                mPrimaryDisplay, config, attribute, [&](const auto& tmpError, const auto&) {
                    EXPECT_TRUE(tmpError == Error::NONE || tmpError == Error::UNSUPPORTED);
                });
        }
    }
}

/**
 * Test IComposerClient::getHdrCapabilities.
 */
TEST_F(GraphicsComposerHidlTest, GetHdrCapabilities) {
    float maxLuminance;
    float maxAverageLuminance;
    float minLuminance;
    mComposerClient->getHdrCapabilities(mPrimaryDisplay, &maxLuminance, &maxAverageLuminance,
                                        &minLuminance);
}

/**
 * Test IComposerClient::setClientTargetSlotCount.
 */
TEST_F(GraphicsComposerHidlTest, SetClientTargetSlotCount) {
    mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kBufferSlotCount);
}

/**
 * Test IComposerClient::setActiveConfig.
 *
 * Test that IComposerClient::setActiveConfig succeeds for all display
 * configs.
 */
TEST_F(GraphicsComposerHidlTest, SetActiveConfig) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        mComposerClient->setActiveConfig(mPrimaryDisplay, config);
        ASSERT_EQ(config, mComposerClient->getActiveConfig(mPrimaryDisplay));
    }
}

/**
 * Test IComposerClient::setActiveConfig
 *
 * Test that config set during IComposerClient::setActiveConfig is maintained
 * during a display on/off power cycle
 */
TEST_F(GraphicsComposerHidlTest, SetActiveConfigPowerCycle) {
    ASSERT_NO_FATAL_FAILURE(
        mComposerClient->setPowerMode(mPrimaryDisplay, IComposerClient::PowerMode::OFF));
    ASSERT_NO_FATAL_FAILURE(
        mComposerClient->setPowerMode(mPrimaryDisplay, IComposerClient::PowerMode::ON));

    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        mComposerClient->setActiveConfig(mPrimaryDisplay, config);
        ASSERT_EQ(config, mComposerClient->getActiveConfig(mPrimaryDisplay));

        ASSERT_NO_FATAL_FAILURE(
            mComposerClient->setPowerMode(mPrimaryDisplay, IComposerClient::PowerMode::OFF));
        ASSERT_NO_FATAL_FAILURE(
            mComposerClient->setPowerMode(mPrimaryDisplay, IComposerClient::PowerMode::ON));
        ASSERT_EQ(config, mComposerClient->getActiveConfig(mPrimaryDisplay));
    }
}

/**
 * Test IComposerClient::getColorMode
 *
 * Test that IComposerClient::getColorMode always returns ColorMode::NATIVE
 */
TEST_F(GraphicsComposerHidlTest, GetColorModes) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    auto nativeModeLocation = std::find(modes.begin(), modes.end(), ColorMode::NATIVE);

    ASSERT_NE(modes.end(), nativeModeLocation);
}

/**
 * Test IComposerClient::setColorMode.
 *
 * Test that IComposerClient::setColorMode succeeds for all color modes.
 */
TEST_F(GraphicsComposerHidlTest, SetColorMode) {
    std::unordered_set<ColorMode> validModes;
    for (auto mode : hidl_enum_range<ColorMode>()) {
        validModes.insert(mode);
    }

    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    for (auto mode : modes) {
        if (validModes.count(mode)) {
            mComposerClient->setColorMode(mPrimaryDisplay, mode);
        }
    }
}

/**
 * Test IComposerClient::setColorMode
 *
 * Test that IComposerClient::setColorMode returns BAD_DISPLAY for
 * an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, SetColorModeBadDisplay) {
    std::vector<ColorMode> modes = mComposerClient->getColorModes(mPrimaryDisplay);
    for (auto mode : modes) {
        Error error = mComposerClient->getRaw()->setColorMode(mInvalidDisplayId, mode);
        EXPECT_EQ(Error::BAD_DISPLAY, error);
    }
}

/**
 * Test IComposerClient::setColorMode
 *
 * Test that IComposerClient::setColorMode returns BAD_PARAMETER when passed in
 * an invalid color mode
 */
TEST_F(GraphicsComposerHidlTest, SetColorModeBadParameter) {
    Error error =
        mComposerClient->getRaw()->setColorMode(mPrimaryDisplay, static_cast<ColorMode>(-1));
    ASSERT_EQ(Error::BAD_PARAMETER, error);
}

/**
 * Test IComposerClient::getDozeSupport
 *
 * Test that IComposerClient::getDozeSupport returns
 * BAD_DISPLAY when passed an invalid display handle
 */
TEST_F(GraphicsComposerHidlTest, GetDozeSupportBadDisplay) {
    Error error;
    mComposerClient->getRaw()->getDozeSupport(
        mInvalidDisplayId, [&](const auto& tmpOutError, const auto&) { error = tmpOutError; });
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::setPowerMode.
 *
 * Test that IComposerClient::setPowerMode succeeds for all power modes.
 */
TEST_F(GraphicsComposerHidlTest, SetPowerMode) {
    std::vector<IComposerClient::PowerMode> modes;
    modes.push_back(IComposerClient::PowerMode::OFF);

    if (mComposerClient->getDozeSupport(mPrimaryDisplay)) {
        modes.push_back(IComposerClient::PowerMode::DOZE);
        modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);
    }

    // push ON last
    modes.push_back(IComposerClient::PowerMode::ON);

    for (auto mode : modes) {
        mComposerClient->setPowerMode(mPrimaryDisplay, mode);
    }
}

/**
 * Test IComposerClient::setPowerMode
 *
 * Test IComposerClient::setPowerMode succeeds with different
 * orderings of power modes
 */
TEST_F(GraphicsComposerHidlTest, SetPowerModeVariations) {
    std::vector<IComposerClient::PowerMode> modes;
    modes.push_back(IComposerClient::PowerMode::OFF);
    modes.push_back(IComposerClient::PowerMode::ON);
    modes.push_back(IComposerClient::PowerMode::OFF);
    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, mode));
    }

    modes.clear();

    modes.push_back(IComposerClient::PowerMode::OFF);
    modes.push_back(IComposerClient::PowerMode::OFF);
    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, mode));
    }

    modes.clear();
    if (mComposerClient->getDozeSupport(mPrimaryDisplay)) {
        modes.push_back(IComposerClient::PowerMode::DOZE);
        modes.push_back(IComposerClient::PowerMode::DOZE);

        for (auto mode : modes) {
            ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, mode));
        }

        modes.clear();

        modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);
        modes.push_back(IComposerClient::PowerMode::DOZE_SUSPEND);

        for (auto mode : modes) {
            ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, mode));
        }
    }

    modes.clear();

    modes.push_back(IComposerClient::PowerMode::ON);
    modes.push_back(IComposerClient::PowerMode::ON);
    for (auto mode : modes) {
        ASSERT_NO_FATAL_FAILURE(mComposerClient->setPowerMode(mPrimaryDisplay, mode));
    }
}

/**
 * Test IComposerClient::setPowerMode
 *
 * Test IComposerClient::setPowerMode returns BAD_DISPLAY when passed an invalid
 * display handle
 */
TEST_F(GraphicsComposerHidlTest, SetPowerModeBadDisplay) {
    Error error =
        mComposerClient->getRaw()->setPowerMode(mInvalidDisplayId, IComposerClient::PowerMode::ON);
    ASSERT_EQ(Error::BAD_DISPLAY, error);
}

/**
 * Test IComposerClient::setPowerMode
 *
 * Test that IComposerClient::setPowerMode returns UNSUPPORTED when passed DOZE
 * or DOZE_SUSPEND on devices that do not support DOZE/DOZE_SUSPEND
 */
TEST_F(GraphicsComposerHidlTest, SetPowerModeUnsupported) {
    if (!mComposerClient->getDozeSupport(mPrimaryDisplay)) {
        Error error = mComposerClient->getRaw()->setPowerMode(mPrimaryDisplay,
                                                              IComposerClient::PowerMode::DOZE);
        EXPECT_EQ(Error::UNSUPPORTED, error);

        error = mComposerClient->getRaw()->setPowerMode(mPrimaryDisplay,
                                                        IComposerClient::PowerMode::DOZE_SUSPEND);
        EXPECT_EQ(Error::UNSUPPORTED, error);
    }
}

/**
 * Test IComposerClient::setPowerMode
 *
 * Tests that IComposerClient::setPowerMode returns BAD_PARAMETER when passed an invalid
 * PowerMode
 */
TEST_F(GraphicsComposerHidlTest, SetPowerModeBadParameter) {
    Error error = mComposerClient->getRaw()->setPowerMode(
        mPrimaryDisplay, static_cast<IComposerClient::PowerMode>(-1));
    ASSERT_EQ(Error::BAD_PARAMETER, error);
}

/**
 * Test IComposerClient::setVsyncEnabled.
 *
 * Test that IComposerClient::setVsyncEnabled succeeds and there is no
 * spurious vsync events.
 */
TEST_F(GraphicsComposerHidlTest, SetVsyncEnabled) {
    mComposerCallback->setVsyncAllowed(true);

    mComposerClient->setVsyncEnabled(mPrimaryDisplay, true);
    usleep(60 * 1000);
    mComposerClient->setVsyncEnabled(mPrimaryDisplay, false);

    mComposerCallback->setVsyncAllowed(false);
}

// Tests for IComposerClient::Command.
class GraphicsComposerHidlCommandTest : public GraphicsComposerHidlTest {
   protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::SetUp());

        ASSERT_NO_FATAL_FAILURE(mGralloc = std::make_unique<Gralloc>());

        Config activeConfig = mComposerClient->getActiveConfig(mPrimaryDisplay);
        mDisplayWidth = mComposerClient->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                             IComposerClient::Attribute::WIDTH);
        mDisplayHeight = mComposerClient->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                              IComposerClient::Attribute::HEIGHT);
        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown());
    }

    const native_handle_t* allocate() {
        IMapper::BufferDescriptorInfo info{};
        info.width = mDisplayWidth;
        info.height = mDisplayHeight;
        info.layerCount = 1;
        info.format = PixelFormat::RGBA_8888;
        info.usage =
            static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN |
                                  BufferUsage::COMPOSER_OVERLAY);

        return mGralloc->allocate(info);
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<TestCommandReader> mReader;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;

   private:
    std::unique_ptr<Gralloc> mGralloc;
};

/**
 * Test IComposerClient::Command::SET_COLOR_TRANSFORM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_COLOR_TRANSFORM) {
    const std::array<float, 16> identity = {{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f,
    }};

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->setColorTransform(identity.data(), ColorTransform::IDENTITY);

    execute();
}

/**
 * Test IComposerClient::Command::SET_CLIENT_TARGET.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_CLIENT_TARGET) {
    mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kBufferSlotCount);

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->setClientTarget(0, nullptr, -1, Dataspace::UNKNOWN,
                             std::vector<IComposerClient::Rect>());

    execute();
}

/**
 * Test IComposerClient::Command::SET_OUTPUT_BUFFER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_OUTPUT_BUFFER) {
    if (mComposerClient->getMaxVirtualDisplayCount() == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    Display display;
    PixelFormat format;
    ASSERT_NO_FATAL_FAILURE(
        display = mComposerClient->createVirtualDisplay(64, 64, PixelFormat::IMPLEMENTATION_DEFINED,
                                                        kBufferSlotCount, &format));

    const native_handle_t* handle;
    ASSERT_NO_FATAL_FAILURE(handle = allocate());

    mWriter->selectDisplay(display);
    mWriter->setOutputBuffer(0, handle, -1);
    execute();
}

/**
 * Test IComposerClient::Command::VALIDATE_DISPLAY.
 */
TEST_F(GraphicsComposerHidlCommandTest, VALIDATE_DISPLAY) {
    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->validateDisplay();
    execute();
}

/**
 * Test IComposerClient::Command::ACCEPT_DISPLAY_CHANGES.
 */
TEST_F(GraphicsComposerHidlCommandTest, ACCEPT_DISPLAY_CHANGES) {
    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->validateDisplay();
    mWriter->acceptDisplayChanges();
    execute();
}

/**
 * Test IComposerClient::Command::PRESENT_DISPLAY.
 */
TEST_F(GraphicsComposerHidlCommandTest, PRESENT_DISPLAY) {
    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->validateDisplay();
    mWriter->presentDisplay();
    execute();
}

/**
 * Test IComposerClient::Command::PRESENT_DISPLAY
 *
 * Test that IComposerClient::Command::PRESENT_DISPLAY works without
 * additional call to validateDisplay when only the layer buffer handle and
 * surface damage have been set
 */
TEST_F(GraphicsComposerHidlCommandTest, PRESENT_DISPLAY_NO_LAYER_STATE_CHANGES) {
    mWriter->selectDisplay(mPrimaryDisplay);
    mComposerClient->setPowerMode(mPrimaryDisplay, IComposerClient::PowerMode::ON);
    mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::SRGB);

    auto handle = allocate();
    ASSERT_NE(nullptr, handle);

    IComposerClient::Rect displayFrame{0, 0, mDisplayWidth, mDisplayHeight};

    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));
    mWriter->selectLayer(layer);
    mWriter->setLayerCompositionType(IComposerClient::Composition::DEVICE);
    mWriter->setLayerDisplayFrame(displayFrame);
    mWriter->setLayerPlaneAlpha(1);
    mWriter->setLayerSourceCrop({0, 0, (float)mDisplayWidth, (float)mDisplayHeight});
    mWriter->setLayerTransform(static_cast<Transform>(0));
    mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, displayFrame));
    mWriter->setLayerZOrder(10);
    mWriter->setLayerBlendMode(IComposerClient::BlendMode::NONE);
    mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, displayFrame));
    mWriter->setLayerBuffer(0, handle, -1);
    mWriter->setLayerDataspace(Dataspace::UNKNOWN);

    mWriter->validateDisplay();
    execute();
    if (mReader->mCompositionChanges.size() != 0) {
        GTEST_SUCCEED() << "Composition change requested, skipping test";
        return;
    }

    ASSERT_EQ(0, mReader->mErrors.size());
    mWriter->presentDisplay();
    execute();
    ASSERT_EQ(0, mReader->mErrors.size());

    mWriter->selectLayer(layer);
    auto handle2 = allocate();
    ASSERT_NE(nullptr, handle2);
    mWriter->setLayerBuffer(0, handle2, -1);
    mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, {0, 0, 10, 10}));
    mWriter->presentDisplay();
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_CURSOR_POSITION.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_CURSOR_POSITION) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerCursorPosition(1, 1);
    mWriter->setLayerCursorPosition(0, 0);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_BUFFER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_BUFFER) {
    auto handle = allocate();
    ASSERT_NE(nullptr, handle);

    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerBuffer(0, handle, -1);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_SURFACE_DAMAGE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SURFACE_DAMAGE) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    IComposerClient::Rect empty{0, 0, 0, 0};
    IComposerClient::Rect unit{0, 0, 1, 1};

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, empty));
    mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, unit));
    mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>());
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_BLEND_MODE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_BLEND_MODE) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerBlendMode(IComposerClient::BlendMode::NONE);
    mWriter->setLayerBlendMode(IComposerClient::BlendMode::PREMULTIPLIED);
    mWriter->setLayerBlendMode(IComposerClient::BlendMode::COVERAGE);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_COLOR.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_COLOR) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerColor(IComposerClient::Color{0xff, 0xff, 0xff, 0xff});
    mWriter->setLayerColor(IComposerClient::Color{0, 0, 0, 0});
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_COMPOSITION_TYPE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_COMPOSITION_TYPE) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerCompositionType(IComposerClient::Composition::CLIENT);
    mWriter->setLayerCompositionType(IComposerClient::Composition::DEVICE);
    mWriter->setLayerCompositionType(IComposerClient::Composition::SOLID_COLOR);
    mWriter->setLayerCompositionType(IComposerClient::Composition::CURSOR);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_DATASPACE.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_DATASPACE) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerDataspace(Dataspace::UNKNOWN);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_DISPLAY_FRAME.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_DISPLAY_FRAME) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerDisplayFrame(IComposerClient::Rect{0, 0, 1, 1});
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_PLANE_ALPHA.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_PLANE_ALPHA) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerPlaneAlpha(0.0f);
    mWriter->setLayerPlaneAlpha(1.0f);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_SIDEBAND_STREAM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SIDEBAND_STREAM) {
    if (!mComposer->hasCapability(IComposer::Capability::SIDEBAND_STREAM)) {
        GTEST_SUCCEED() << "no sideband stream support";
        return;
    }

    auto handle = allocate();
    ASSERT_NE(nullptr, handle);

    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerSidebandStream(handle);
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_SOURCE_CROP.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_SOURCE_CROP) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerSourceCrop(IComposerClient::FRect{0.0f, 0.0f, 1.0f, 1.0f});
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_TRANSFORM.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_TRANSFORM) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerTransform(static_cast<Transform>(0));
    mWriter->setLayerTransform(Transform::FLIP_H);
    mWriter->setLayerTransform(Transform::FLIP_V);
    mWriter->setLayerTransform(Transform::ROT_90);
    mWriter->setLayerTransform(Transform::ROT_180);
    mWriter->setLayerTransform(Transform::ROT_270);
    mWriter->setLayerTransform(static_cast<Transform>(Transform::FLIP_H | Transform::ROT_90));
    mWriter->setLayerTransform(static_cast<Transform>(Transform::FLIP_V | Transform::ROT_90));
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_VISIBLE_REGION.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_VISIBLE_REGION) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    IComposerClient::Rect empty{0, 0, 0, 0};
    IComposerClient::Rect unit{0, 0, 1, 1};

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, empty));
    mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>(1, unit));
    mWriter->setLayerVisibleRegion(std::vector<IComposerClient::Rect>());
    execute();
}

/**
 * Test IComposerClient::Command::SET_LAYER_Z_ORDER.
 */
TEST_F(GraphicsComposerHidlCommandTest, SET_LAYER_Z_ORDER) {
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer =
                                mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));

    mWriter->selectDisplay(mPrimaryDisplay);
    mWriter->selectLayer(layer);
    mWriter->setLayerZOrder(10);
    mWriter->setLayerZOrder(0);
    execute();
}

}  // namespace
}  // namespace vts
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using android::hardware::graphics::composer::V2_1::vts::GraphicsComposerHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(GraphicsComposerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GraphicsComposerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
