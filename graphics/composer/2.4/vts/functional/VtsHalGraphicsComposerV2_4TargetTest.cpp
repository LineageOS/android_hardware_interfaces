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

#define LOG_TAG "graphics_composer_hidl_hal_test@2.4"

#include <algorithm>
#include <thread>

#include <android-base/logging.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <composer-vts/2.1/TestCommandReader.h>
#include <composer-vts/2.4/ComposerVts.h>
#include <composer-vts/2.4/GraphicsComposerCallback.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <mapper-vts/2.0/MapperVts.h>
#include <mapper-vts/3.0/MapperVts.h>
#include <mapper-vts/4.0/MapperVts.h>
#include <utils/Timers.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {
namespace {

using common::V1_0::BufferUsage;
using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::PixelFormat;
using mapper::V2_0::IMapper;
using V2_1::Layer;
using V2_2::Transform;
using V2_2::vts::Gralloc;

using ContentType = IComposerClient::ContentType;
using DisplayCapability = IComposerClient::DisplayCapability;

class GraphicsComposerHidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                mComposer = std::make_unique<Composer>(IComposer::getService(GetParam())));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());

        mComposerCallback = new GraphicsComposerCallback;
        mComposerClient->registerCallback_2_4(mComposerCallback);

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
            EXPECT_EQ(0, mComposerCallback->getInvalidVsync_2_4Count());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncPeriodChangeCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidSeamlessPossibleCount());
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

    // returns an invalid config id (one that has not been registered to a
    // display).  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() configs registered while running tests
    Display GetInvalidConfigId(Display display) {
        std::vector<Config> validConfigs = mComposerClient->getDisplayConfigs(display);
        uint64_t id = std::numeric_limits<uint64_t>::max();
        while (id > 0) {
            if (std::find(validConfigs.begin(), validConfigs.end(), id) == validConfigs.end()) {
                return id;
            }
            id--;
        }

        return 0;
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    void forEachTwoConfigs(Display display, std::function<void(Config, Config)> func) {
        const auto displayConfigs = mComposerClient->getDisplayConfigs(display);
        for (const Config config1 : displayConfigs) {
            for (const Config config2 : displayConfigs) {
                if (config1 != config2) {
                    func(config1, config2);
                }
            }
        }
    }

    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;

    void Test_setContentType(const ContentType& contentType, const char* contentTypeStr);
    void Test_setContentTypeForDisplay(const Display& display,
                                       const std::vector<ContentType>& capabilities,
                                       const ContentType& contentType, const char* contentTypeStr);

    std::unique_ptr<Composer> mComposer;
    std::unique_ptr<ComposerClient> mComposerClient;
    sp<GraphicsComposerCallback> mComposerCallback;
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

        const Config activeConfig = mComposerClient->getActiveConfig(mPrimaryDisplay);
        mDisplayWidth = mComposerClient->getDisplayAttribute_2_4(mPrimaryDisplay, activeConfig,
                                                                 IComposerClient::Attribute::WIDTH);
        mDisplayHeight = mComposerClient->getDisplayAttribute_2_4(
                mPrimaryDisplay, activeConfig, IComposerClient::Attribute::HEIGHT);

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<V2_1::vts::TestCommandReader>();
    }

    void TearDown() override {
        ASSERT_EQ(0, mReader->mErrors.size());
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerHidlTest::TearDown());
    }

    const native_handle_t* allocate() {
        return mGralloc->allocate(
                /*width*/ 64, /*height*/ 64, /*layerCount*/ 1,
                static_cast<common::V1_1::PixelFormat>(PixelFormat::RGBA_8888),
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN));
    }

    void execute() { mComposerClient->execute(mReader.get(), mWriter.get()); }

    void Test_setActiveConfigWithConstraints(
            const IComposerClient::VsyncPeriodChangeConstraints& constraints, bool refreshMiss);

    void sendRefreshFrame(const VsyncPeriodChangeTimeline&);

    void waitForVsyncPeriodChange(Display display, const VsyncPeriodChangeTimeline& timeline,
                                  int64_t desiredTimeNanos, int64_t oldPeriodNanos,
                                  int64_t newPeriodNanos);

    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<V2_1::vts::TestCommandReader> mReader;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;

  private:
    std::unique_ptr<Gralloc> mGralloc;
};

TEST_P(GraphicsComposerHidlTest, getDisplayCapabilitiesBadDisplay) {
    std::vector<IComposerClient::DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mInvalidDisplayId, &capabilities);
    EXPECT_EQ(Error::BAD_DISPLAY, error);
}

TEST_P(GraphicsComposerHidlTest, getDisplayCapabilities) {
    for (Display display : mComposerCallback->getDisplays()) {
        std::vector<IComposerClient::DisplayCapability> capabilities;
        EXPECT_EQ(Error::NONE, mComposerClient->getDisplayCapabilities(display, &capabilities));
    }
}

TEST_P(GraphicsComposerHidlTest, getDisplayConnectionType) {
    IComposerClient::DisplayConnectionType type;
    EXPECT_EQ(Error::BAD_DISPLAY,
              mComposerClient->getDisplayConnectionType(mInvalidDisplayId, &type));

    for (Display display : mComposerCallback->getDisplays()) {
        EXPECT_EQ(Error::NONE, mComposerClient->getDisplayConnectionType(display, &type));
    }
}

TEST_P(GraphicsComposerHidlTest, GetDisplayAttribute_2_4) {
    std::vector<Config> configs = mComposerClient->getDisplayConfigs(mPrimaryDisplay);
    for (auto config : configs) {
        const std::array<IComposerClient::Attribute, 4> requiredAttributes = {{
                IComposerClient::Attribute::WIDTH,
                IComposerClient::Attribute::HEIGHT,
                IComposerClient::Attribute::VSYNC_PERIOD,
                IComposerClient::Attribute::CONFIG_GROUP,
        }};
        for (auto attribute : requiredAttributes) {
            mComposerClient->getRaw()->getDisplayAttribute_2_4(
                    mPrimaryDisplay, config, attribute,
                    [&](const auto& tmpError, const auto& value) {
                        EXPECT_EQ(Error::NONE, tmpError);
                        EXPECT_NE(-1, value);
                    });
        }

        const std::array<IComposerClient::Attribute, 2> optionalAttributes = {{
                IComposerClient::Attribute::DPI_X,
                IComposerClient::Attribute::DPI_Y,
        }};
        for (auto attribute : optionalAttributes) {
            mComposerClient->getRaw()->getDisplayAttribute_2_4(
                    mPrimaryDisplay, config, attribute, [&](const auto& tmpError, const auto&) {
                        EXPECT_TRUE(tmpError == Error::NONE || tmpError == Error::UNSUPPORTED);
                    });
        }
    }
}

TEST_P(GraphicsComposerHidlTest, getDisplayVsyncPeriod_BadDisplay) {
    VsyncPeriodNanos vsyncPeriodNanos;
    EXPECT_EQ(Error::BAD_DISPLAY,
              mComposerClient->getDisplayVsyncPeriod(mInvalidDisplayId, &vsyncPeriodNanos));
}

TEST_P(GraphicsComposerHidlTest, getDisplayVsyncPeriod) {
    for (Display display : mComposerCallback->getDisplays()) {
        for (Config config : mComposerClient->getDisplayConfigs(display)) {
            VsyncPeriodNanos expectedVsyncPeriodNanos = mComposerClient->getDisplayAttribute_2_4(
                    display, config, IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);

            mComposerClient->setActiveConfig(display, config);
            VsyncPeriodNanos vsyncPeriodNanos;
            int retryCount = 100;
            do {
                std::this_thread::sleep_for(10ms);
                EXPECT_EQ(Error::NONE,
                          mComposerClient->getDisplayVsyncPeriod(display, &vsyncPeriodNanos));
                --retryCount;
            } while (vsyncPeriodNanos != expectedVsyncPeriodNanos && retryCount > 0);

            EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);
        }
    }
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_BadDisplay) {
    VsyncPeriodChangeTimeline timeline;
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setActiveConfigWithConstraints(
                                          mInvalidDisplayId, Config(0), constraints, &timeline));
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_BadConfig) {
    VsyncPeriodChangeTimeline timeline;
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    for (Display display : mComposerCallback->getDisplays()) {
        Config invalidConfigId = GetInvalidConfigId(display);
        EXPECT_EQ(Error::BAD_CONFIG, mComposerClient->setActiveConfigWithConstraints(
                                             display, invalidConfigId, constraints, &timeline));
    }
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_SeamlessNotAllowed) {
    VsyncPeriodChangeTimeline timeline;
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = true;
    constraints.desiredTimeNanos = systemTime();

    for (Display display : mComposerCallback->getDisplays()) {
        forEachTwoConfigs(display, [&](Config config1, Config config2) {
            const auto configGroup1 = mComposerClient->getDisplayAttribute_2_4(
                    display, config1, IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);
            const auto configGroup2 = mComposerClient->getDisplayAttribute_2_4(
                    display, config2, IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);
            if (configGroup1 != configGroup2) {
                mComposerClient->setActiveConfig(display, config1);
                EXPECT_EQ(Error::SEAMLESS_NOT_ALLOWED,
                          mComposerClient->setActiveConfigWithConstraints(display, config2,
                                                                          constraints, &timeline));
            }
        });
    }
}

static inline auto toTimePoint(nsecs_t time) {
    return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(time));
}

void GraphicsComposerHidlCommandTest::sendRefreshFrame(const VsyncPeriodChangeTimeline& timeline) {
    // Refresh time should be before newVsyncAppliedTimeNanos
    EXPECT_LT(timeline.refreshTimeNanos, timeline.newVsyncAppliedTimeNanos);

    std::this_thread::sleep_until(toTimePoint(timeline.refreshTimeNanos));

    mWriter->selectDisplay(mPrimaryDisplay);
    mComposerClient->setPowerMode(mPrimaryDisplay, V2_1::IComposerClient::PowerMode::ON);
    mComposerClient->setColorMode_2_3(mPrimaryDisplay, ColorMode::NATIVE,
                                      RenderIntent::COLORIMETRIC);

    auto handle = allocate();
    ASSERT_NE(nullptr, handle);

    IComposerClient::Rect displayFrame{0, 0, mDisplayWidth, mDisplayHeight};

    Layer layer;
    ASSERT_NO_FATAL_FAILURE(
            layer = mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount));
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

void GraphicsComposerHidlCommandTest::waitForVsyncPeriodChange(
        Display display, const VsyncPeriodChangeTimeline& timeline, int64_t desiredTimeNanos,
        int64_t oldPeriodNanos, int64_t newPeriodNanos) {
    const auto CHANGE_DEADLINE = toTimePoint(timeline.newVsyncAppliedTimeNanos) + 100ms;
    while (std::chrono::steady_clock::now() <= CHANGE_DEADLINE) {
        VsyncPeriodNanos vsyncPeriodNanos;
        EXPECT_EQ(Error::NONE, mComposerClient->getDisplayVsyncPeriod(display, &vsyncPeriodNanos));
        if (systemTime() <= desiredTimeNanos) {
            EXPECT_EQ(vsyncPeriodNanos, oldPeriodNanos);
        } else if (vsyncPeriodNanos == newPeriodNanos) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(oldPeriodNanos));
    }
}

void GraphicsComposerHidlCommandTest::Test_setActiveConfigWithConstraints(
        const IComposerClient::VsyncPeriodChangeConstraints& constraints, bool refreshMiss) {
    VsyncPeriodChangeTimeline timeline = {};

    for (Display display : mComposerCallback->getDisplays()) {
        forEachTwoConfigs(display, [&](Config config1, Config config2) {
            mComposerClient->setActiveConfig(display, config1);

            int32_t vsyncPeriod1 = mComposerClient->getDisplayAttribute_2_4(
                    display, config1, IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);
            int32_t vsyncPeriod2 = mComposerClient->getDisplayAttribute_2_4(
                    display, config2, IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);

            if (vsyncPeriod1 == vsyncPeriod2) {
                return;  // continue
            }

            EXPECT_EQ(Error::NONE, mComposerClient->setActiveConfigWithConstraints(
                                           display, config2, constraints, &timeline));

            EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos >= constraints.desiredTimeNanos);
            // Refresh rate should change within a reasonable time
            constexpr std::chrono::nanoseconds kReasonableTimeForChange = 1s;  // 1 second
            EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos - constraints.desiredTimeNanos <=
                        kReasonableTimeForChange.count());

            if (timeline.refreshRequired) {
                if (refreshMiss) {
                    // Miss the refresh frame on purpose to make sure the implementation sends a
                    // callback
                    std::this_thread::sleep_until(toTimePoint(timeline.refreshTimeNanos) + 100ms);
                }
                sendRefreshFrame(timeline);
            }
            waitForVsyncPeriodChange(display, timeline, constraints.desiredTimeNanos, vsyncPeriod1,
                                     vsyncPeriod2);

            // At this point the refresh rate should have changed already, however in rare
            // cases the implementation might have missed the deadline. In this case a new
            // timeline should have been provided.
            auto newTimelime = mComposerCallback->takeLastVsyncPeriodChangeTimeline();
            if (timeline.refreshRequired && refreshMiss) {
                EXPECT_TRUE(newTimelime.has_value());
            }

            if (newTimelime.has_value()) {
                if (timeline.refreshRequired) {
                    sendRefreshFrame(newTimelime.value());
                }
                waitForVsyncPeriodChange(display, newTimelime.value(), constraints.desiredTimeNanos,
                                         vsyncPeriod1, vsyncPeriod2);
            }

            VsyncPeriodNanos vsyncPeriodNanos;
            EXPECT_EQ(Error::NONE,
                      mComposerClient->getDisplayVsyncPeriod(display, &vsyncPeriodNanos));
            EXPECT_EQ(vsyncPeriodNanos, vsyncPeriod2);
        });
    }
}

TEST_P(GraphicsComposerHidlCommandTest, setActiveConfigWithConstraints) {
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();
    Test_setActiveConfigWithConstraints(constraints, false);
}

TEST_P(GraphicsComposerHidlCommandTest, setActiveConfigWithConstraints_Delayed) {
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constexpr nsecs_t kDelayForChange = 300'000'000;  // 300ms
    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime() + kDelayForChange;
    Test_setActiveConfigWithConstraints(constraints, false);
}

TEST_P(GraphicsComposerHidlCommandTest, setActiveConfigWithConstraints_MissRefresh) {
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();
    Test_setActiveConfigWithConstraints(constraints, true);
}

TEST_P(GraphicsComposerHidlTest, setAutoLowLatencyModeBadDisplay) {
    EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, true));
    EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, false));
}

TEST_P(GraphicsComposerHidlTest, setAutoLowLatencyMode) {
    for (Display display : mComposerCallback->getDisplays()) {
        std::vector<DisplayCapability> capabilities;
        const auto error = mComposerClient->getDisplayCapabilities(display, &capabilities);
        EXPECT_EQ(Error::NONE, error);

        const bool allmSupport =
                std::find(capabilities.begin(), capabilities.end(),
                          DisplayCapability::AUTO_LOW_LATENCY_MODE) != capabilities.end();

        if (!allmSupport) {
            EXPECT_EQ(Error::UNSUPPORTED,
                      mComposerClient->setAutoLowLatencyMode(mPrimaryDisplay, true));
            EXPECT_EQ(Error::UNSUPPORTED,
                      mComposerClient->setAutoLowLatencyMode(mPrimaryDisplay, false));
            GTEST_SUCCEED() << "Auto Low Latency Mode is not supported on display "
                            << to_string(display) << ", skipping test";
            return;
        }

        EXPECT_EQ(Error::NONE, mComposerClient->setAutoLowLatencyMode(mPrimaryDisplay, true));
        EXPECT_EQ(Error::NONE, mComposerClient->setAutoLowLatencyMode(mPrimaryDisplay, false));
    }
}

TEST_P(GraphicsComposerHidlTest, getSupportedContentTypesBadDisplay) {
    std::vector<ContentType> supportedContentTypes;
    const auto error =
            mComposerClient->getSupportedContentTypes(mInvalidDisplayId, &supportedContentTypes);
    EXPECT_EQ(Error::BAD_DISPLAY, error);
}

TEST_P(GraphicsComposerHidlTest, getSupportedContentTypes) {
    std::vector<ContentType> supportedContentTypes;
    for (Display display : mComposerCallback->getDisplays()) {
        supportedContentTypes.clear();
        const auto error =
                mComposerClient->getSupportedContentTypes(display, &supportedContentTypes);
        const bool noneSupported =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          ContentType::NONE) != supportedContentTypes.end();
        EXPECT_EQ(Error::NONE, error);
        EXPECT_FALSE(noneSupported);
    }
}

TEST_P(GraphicsComposerHidlTest, setContentTypeNoneAlwaysAccepted) {
    for (Display display : mComposerCallback->getDisplays()) {
        const auto error = mComposerClient->setContentType(display, ContentType::NONE);
        EXPECT_NE(Error::UNSUPPORTED, error);
    }
}

TEST_P(GraphicsComposerHidlTest, setContentTypeBadDisplay) {
    const auto types = {ContentType::NONE, ContentType::GRAPHICS, ContentType::PHOTO,
                        ContentType::CINEMA, ContentType::GAME};
    for (auto type : types) {
        EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setContentType(mInvalidDisplayId, type));
    }
}

void GraphicsComposerHidlTest::Test_setContentTypeForDisplay(
        const Display& display, const std::vector<ContentType>& capabilities,
        const ContentType& contentType, const char* contentTypeStr) {
    const bool contentTypeSupport =
            std::find(capabilities.begin(), capabilities.end(), contentType) != capabilities.end();

    if (!contentTypeSupport) {
        EXPECT_EQ(Error::UNSUPPORTED, mComposerClient->setContentType(display, contentType));
        GTEST_SUCCEED() << contentTypeStr << " content type is not supported on display "
                        << to_string(display) << ", skipping test";
        return;
    }

    EXPECT_EQ(Error::NONE, mComposerClient->setContentType(display, contentType));
    EXPECT_EQ(Error::NONE, mComposerClient->setContentType(display, ContentType::NONE));
}

void GraphicsComposerHidlTest::Test_setContentType(const ContentType& contentType,
                                                   const char* contentTypeStr) {
    for (Display display : mComposerCallback->getDisplays()) {
        std::vector<ContentType> supportedContentTypes;
        const auto error =
                mComposerClient->getSupportedContentTypes(display, &supportedContentTypes);
        EXPECT_EQ(Error::NONE, error);

        Test_setContentTypeForDisplay(display, supportedContentTypes, contentType, contentTypeStr);
    }
}

TEST_P(GraphicsComposerHidlTest, setGraphicsContentType) {
    Test_setContentType(ContentType::GRAPHICS, "GRAPHICS");
}

TEST_P(GraphicsComposerHidlTest, setPhotoContentType) {
    Test_setContentType(ContentType::PHOTO, "PHOTO");
}

TEST_P(GraphicsComposerHidlTest, setCinemaContentType) {
    Test_setContentType(ContentType::CINEMA, "CINEMA");
}

TEST_P(GraphicsComposerHidlTest, setGameContentType) {
    Test_setContentType(ContentType::GAME, "GAME");
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
        android::hardware::PrintInstanceNameToString);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerHidlCommandTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace
}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
