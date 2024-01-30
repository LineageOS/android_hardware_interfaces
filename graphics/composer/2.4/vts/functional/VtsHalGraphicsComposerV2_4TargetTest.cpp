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
#include <regex>
#include <thread>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <composer-vts/2.4/ComposerVts.h>
#include <composer-vts/2.4/GraphicsComposerCallback.h>
#include <composer-vts/2.4/TestCommandReader.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <ui/GraphicBuffer.h>
#include <utils/Timers.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {
namespace vts {
namespace {

using namespace std::chrono_literals;

using common::V1_0::BufferUsage;
using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::PixelFormat;
using V2_1::Layer;
using V2_2::Transform;

using ContentType = IComposerClient::ContentType;
using DisplayCapability = IComposerClient::DisplayCapability;

class VtsDisplay {
  public:
    VtsDisplay(Display display, int32_t displayWidth, int32_t displayHeight)
        : mDisplay(display), mDisplayWidth(displayWidth), mDisplayHeight(displayHeight) {}

    Display get() const { return mDisplay; }

    IComposerClient::FRect getCrop() const {
        return {0, 0, static_cast<float>(mDisplayWidth), static_cast<float>(mDisplayHeight)};
    }

    IComposerClient::Rect getFrameRect() const { return {0, 0, mDisplayWidth, mDisplayHeight}; }

    void setDimensions(int32_t displayWidth, int32_t displayHeight) {
        mDisplayWidth = displayWidth;
        mDisplayHeight = displayHeight;
    }

  private:
    const Display mDisplay;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
};

class GraphicsComposerHidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(
                mComposer = std::make_unique<Composer>(IComposer::getService(GetParam())));
        ASSERT_NO_FATAL_FAILURE(mComposerClient = mComposer->createClient());

        mComposerCallback = new GraphicsComposerCallback;
        mComposerClient->registerCallback_2_4(mComposerCallback);

        // assume the first displays are built-in and are never removed
        mDisplays = waitForDisplays();

        mInvalidDisplayId = GetInvalidDisplayId();

        // explicitly disable vsync
        for (const auto& display : mDisplays) {
            mComposerClient->setVsyncEnabled(display.get(), false);
        }
        mComposerCallback->setVsyncAllowed(false);

        mWriter = std::make_unique<CommandWriterBase>(1024);
        mReader = std::make_unique<TestCommandReader>();
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
        uint64_t id = std::numeric_limits<uint64_t>::max();
        while (id > 0) {
            if (std::none_of(mDisplays.begin(), mDisplays.end(),
                             [&](const VtsDisplay& display) { return id == display.get(); })) {
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

    sp<GraphicBuffer> allocate(int32_t width, int32_t height) {
        auto result = sp<GraphicBuffer>::make(
                width, height, static_cast<int32_t>(PixelFormat::RGBA_8888), /*layerCount*/ 1,
                static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN | BufferUsage::CPU_READ_OFTEN |
                                      BufferUsage::COMPOSER_OVERLAY));
        if (result->initCheck() != STATUS_OK) {
            return nullptr;
        }
        return result;
    }

    struct TestParameters {
        nsecs_t delayForChange;
        bool refreshMiss;
    };

    void Test_setActiveConfigWithConstraints(const TestParameters& params);

    void sendRefreshFrame(const VtsDisplay& display, const VsyncPeriodChangeTimeline*);

    void waitForVsyncPeriodChange(Display display, const VsyncPeriodChangeTimeline& timeline,
                                  int64_t desiredTimeNanos, int64_t oldPeriodNanos,
                                  int64_t newPeriodNanos);

    std::unique_ptr<ComposerClient> mComposerClient;
    std::vector<VtsDisplay> mDisplays;
    Display mInvalidDisplayId;

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

    void Test_setContentType(const ContentType& contentType, const char* contentTypeStr);
    void Test_setContentTypeForDisplay(const Display& display,
                                       const std::vector<ContentType>& capabilities,
                                       const ContentType& contentType, const char* contentTypeStr);

    Error setActiveConfigWithConstraints(
            VtsDisplay& display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& constraints,
            VsyncPeriodChangeTimeline* timeline) {
        const auto error = mComposerClient->setActiveConfigWithConstraints(display.get(), config,
                                                                           constraints, timeline);
        if (error == Error::NONE) {
            const int32_t displayWidth = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config, IComposerClient::Attribute::WIDTH);
            const int32_t displayHeight = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config, IComposerClient::Attribute::HEIGHT);
            display.setDimensions(displayWidth, displayHeight);
        }
        return error;
    }

    void setActiveConfig(VtsDisplay& display, Config config) {
        mComposerClient->setActiveConfig(display.get(), config);
        const int32_t displayWidth = mComposerClient->getDisplayAttribute_2_4(
                display.get(), config, IComposerClient::Attribute::WIDTH);
        const int32_t displayHeight = mComposerClient->getDisplayAttribute_2_4(
                display.get(), config, IComposerClient::Attribute::HEIGHT);
        display.setDimensions(displayWidth, displayHeight);
    }

  private:
    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;

    std::vector<VtsDisplay> waitForDisplays() {
        while (true) {
            // Sleep for a small period of time to allow all built-in displays
            // to post hotplug events
            std::this_thread::sleep_for(5ms);
            std::vector<Display> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                continue;
            }

            std::vector<VtsDisplay> vtsDisplays;
            vtsDisplays.reserve(displays.size());
            for (Display display : displays) {
                const Config activeConfig = mComposerClient->getActiveConfig(display);
                const int32_t displayWidth = mComposerClient->getDisplayAttribute_2_4(
                        display, activeConfig, IComposerClient::Attribute::WIDTH);
                const int32_t displayHeight = mComposerClient->getDisplayAttribute_2_4(
                        display, activeConfig, IComposerClient::Attribute::HEIGHT);
                vtsDisplays.emplace_back(VtsDisplay{display, displayWidth, displayHeight});
            }

            return vtsDisplays;
        }
    }

    std::unique_ptr<Composer> mComposer;
    std::unique_ptr<CommandWriterBase> mWriter;
    std::unique_ptr<TestCommandReader> mReader;
    sp<GraphicsComposerCallback> mComposerCallback;
};

TEST_P(GraphicsComposerHidlTest, getDisplayCapabilitiesBadDisplay) {
    std::vector<IComposerClient::DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mInvalidDisplayId, &capabilities);
    EXPECT_EQ(Error::BAD_DISPLAY, error);
}

TEST_P(GraphicsComposerHidlTest, getDisplayCapabilities) {
    for (const auto& display : mDisplays) {
        std::vector<IComposerClient::DisplayCapability> capabilities;
        EXPECT_EQ(Error::NONE,
                  mComposerClient->getDisplayCapabilities(display.get(), &capabilities));
    }
}

TEST_P(GraphicsComposerHidlTest, getDisplayConnectionType) {
    IComposerClient::DisplayConnectionType type;
    EXPECT_EQ(Error::BAD_DISPLAY,
              mComposerClient->getDisplayConnectionType(mInvalidDisplayId, &type));

    for (const auto& display : mDisplays) {
        EXPECT_EQ(Error::NONE, mComposerClient->getDisplayConnectionType(display.get(), &type));
    }
}

TEST_P(GraphicsComposerHidlTest, GetDisplayAttribute_2_4) {
    for (const auto& display : mDisplays) {
        std::vector<Config> configs = mComposerClient->getDisplayConfigs(display.get());
        for (auto config : configs) {
            const std::array<IComposerClient::Attribute, 4> requiredAttributes = {{
                    IComposerClient::Attribute::WIDTH,
                    IComposerClient::Attribute::HEIGHT,
                    IComposerClient::Attribute::VSYNC_PERIOD,
                    IComposerClient::Attribute::CONFIG_GROUP,
            }};
            for (auto attribute : requiredAttributes) {
                mComposerClient->getRaw()->getDisplayAttribute_2_4(
                        display.get(), config, attribute,
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
                        display.get(), config, attribute, [&](const auto& tmpError, const auto&) {
                            EXPECT_TRUE(tmpError == Error::NONE || tmpError == Error::UNSUPPORTED);
                        });
            }
        }
    }
}

TEST_P(GraphicsComposerHidlTest, getDisplayVsyncPeriod_BadDisplay) {
    VsyncPeriodNanos vsyncPeriodNanos;
    EXPECT_EQ(Error::BAD_DISPLAY,
              mComposerClient->getDisplayVsyncPeriod(mInvalidDisplayId, &vsyncPeriodNanos));
}

TEST_P(GraphicsComposerHidlTest, getDisplayVsyncPeriod) {
    for (VtsDisplay& display : mDisplays) {
        for (Config config : mComposerClient->getDisplayConfigs(display.get())) {
            VsyncPeriodNanos expectedVsyncPeriodNanos = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config,
                    IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);

            VsyncPeriodChangeTimeline timeline;
            IComposerClient::VsyncPeriodChangeConstraints constraints;

            constraints.desiredTimeNanos = systemTime();
            constraints.seamlessRequired = false;
            EXPECT_EQ(Error::NONE,
                      setActiveConfigWithConstraints(display, config, constraints, &timeline));

            if (timeline.refreshRequired) {
                sendRefreshFrame(display, &timeline);
            }
            waitForVsyncPeriodChange(display.get(), timeline, constraints.desiredTimeNanos, 0,
                                     expectedVsyncPeriodNanos);

            VsyncPeriodNanos vsyncPeriodNanos;
            int retryCount = 100;
            do {
                std::this_thread::sleep_for(10ms);
                vsyncPeriodNanos = 0;
                EXPECT_EQ(Error::NONE,
                          mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos));
                --retryCount;
            } while (vsyncPeriodNanos != expectedVsyncPeriodNanos && retryCount > 0);

            EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);

            // Make sure that the vsync period stays the same if the active config is not changed.
            auto timeout = 1ms;
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(timeout);
                timeout *= 2;
                vsyncPeriodNanos = 0;
                EXPECT_EQ(Error::NONE,
                          mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos));
                EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);
            }
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

    for (VtsDisplay& display : mDisplays) {
        Config invalidConfigId = GetInvalidConfigId(display.get());
        EXPECT_EQ(Error::BAD_CONFIG,
                  setActiveConfigWithConstraints(display, invalidConfigId, constraints, &timeline));
    }
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_SeamlessNotAllowed) {
    VsyncPeriodChangeTimeline timeline;
    IComposerClient::VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = true;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        forEachTwoConfigs(display.get(), [&](Config config1, Config config2) {
            const auto configGroup1 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config1,
                    IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);
            const auto configGroup2 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config2,
                    IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);
            if (configGroup1 != configGroup2) {
                setActiveConfig(display, config1);
                sendRefreshFrame(display, nullptr);
                EXPECT_EQ(Error::SEAMLESS_NOT_ALLOWED,
                          setActiveConfigWithConstraints(display, config2, constraints, &timeline));
            }
        });
    }
}

static inline auto toTimePoint(nsecs_t time) {
    return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(time));
}

void GraphicsComposerHidlTest::sendRefreshFrame(const VtsDisplay& display,
                                                const VsyncPeriodChangeTimeline* timeline) {
    if (timeline != nullptr) {
        // Refresh time should be before newVsyncAppliedTimeNanos
        EXPECT_LT(timeline->refreshTimeNanos, timeline->newVsyncAppliedTimeNanos);

        std::this_thread::sleep_until(toTimePoint(timeline->refreshTimeNanos));
    }

    mWriter->selectDisplay(display.get());
    mComposerClient->setPowerMode(display.get(), V2_1::IComposerClient::PowerMode::ON);
    mComposerClient->setColorMode_2_3(display.get(), ColorMode::NATIVE, RenderIntent::COLORIMETRIC);

    IComposerClient::FRect displayCrop = display.getCrop();
    int32_t displayWidth = static_cast<int32_t>(std::ceilf(displayCrop.right - displayCrop.left));
    int32_t displayHeight = static_cast<int32_t>(std::ceilf(displayCrop.bottom - displayCrop.top));
    Layer layer;
    ASSERT_NO_FATAL_FAILURE(layer = mComposerClient->createLayer(display.get(), kBufferSlotCount));

    {
        auto handle = allocate(displayWidth, displayHeight);
        ASSERT_NE(nullptr, handle.get());

        mWriter->selectLayer(layer);
        mWriter->setLayerCompositionType(IComposerClient::Composition::DEVICE);
        mWriter->setLayerDisplayFrame(display.getFrameRect());
        mWriter->setLayerPlaneAlpha(1);
        mWriter->setLayerSourceCrop(display.getCrop());
        mWriter->setLayerTransform(static_cast<Transform>(0));
        mWriter->setLayerVisibleRegion(
                std::vector<IComposerClient::Rect>(1, display.getFrameRect()));
        mWriter->setLayerZOrder(10);
        mWriter->setLayerBlendMode(IComposerClient::BlendMode::NONE);
        mWriter->setLayerSurfaceDamage(
                std::vector<IComposerClient::Rect>(1, display.getFrameRect()));
        mWriter->setLayerBuffer(0, handle->handle, -1);
        mWriter->setLayerDataspace(Dataspace::UNKNOWN);

        mWriter->validateDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        mReader->mCompositionChanges.clear();

        mWriter->presentDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
    }

    {
        auto handle = allocate(displayWidth, displayHeight);
        ASSERT_NE(nullptr, handle.get());

        mWriter->selectLayer(layer);
        mWriter->setLayerBuffer(0, handle->handle, -1);
        mWriter->setLayerSurfaceDamage(std::vector<IComposerClient::Rect>(1, {0, 0, 10, 10}));
        mWriter->validateDisplay();
        execute();
        ASSERT_EQ(0, mReader->mErrors.size());
        mReader->mCompositionChanges.clear();

        mWriter->presentDisplay();
        execute();
    }

    ASSERT_NO_FATAL_FAILURE(mComposerClient->destroyLayer(display.get(), layer));
}

void GraphicsComposerHidlTest::waitForVsyncPeriodChange(Display display,
                                                        const VsyncPeriodChangeTimeline& timeline,
                                                        int64_t desiredTimeNanos,
                                                        int64_t oldPeriodNanos,
                                                        int64_t newPeriodNanos) {
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

void GraphicsComposerHidlTest::Test_setActiveConfigWithConstraints(const TestParameters& params) {
    for (VtsDisplay& display : mDisplays) {
        forEachTwoConfigs(display.get(), [&](Config config1, Config config2) {
            setActiveConfig(display, config1);
            sendRefreshFrame(display, nullptr);

            const auto vsyncPeriod1 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config1,
                    IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);
            const auto configGroup1 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config1,
                    IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);
            const auto vsyncPeriod2 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config2,
                    IComposerClient::IComposerClient::Attribute::VSYNC_PERIOD);
            const auto configGroup2 = mComposerClient->getDisplayAttribute_2_4(
                    display.get(), config2,
                    IComposerClient::IComposerClient::Attribute::CONFIG_GROUP);

            if (vsyncPeriod1 == vsyncPeriod2) {
                return;  // continue
            }

            // We don't allow delayed change when changing config groups
            if (params.delayForChange > 0 && configGroup1 != configGroup2) {
                return;  // continue
            }

            VsyncPeriodChangeTimeline timeline;
            IComposerClient::VsyncPeriodChangeConstraints constraints = {
                    .desiredTimeNanos = systemTime() + params.delayForChange,
                    .seamlessRequired = false};
            EXPECT_EQ(Error::NONE,
                      setActiveConfigWithConstraints(display, config2, constraints, &timeline));

            EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos >= constraints.desiredTimeNanos);
            if (configGroup1 == configGroup2) {
                // Refresh rate should change within a reasonable time
                constexpr std::chrono::nanoseconds kReasonableTimeForChange = 1s;
                EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos - constraints.desiredTimeNanos <=
                            kReasonableTimeForChange.count());
            }

            if (timeline.refreshRequired) {
                if (params.refreshMiss) {
                    // Miss the refresh frame on purpose to make sure the implementation sends a
                    // callback
                    std::this_thread::sleep_until(toTimePoint(timeline.refreshTimeNanos) + 100ms);
                }
                sendRefreshFrame(display, &timeline);
            }
            waitForVsyncPeriodChange(display.get(), timeline, constraints.desiredTimeNanos,
                                     vsyncPeriod1, vsyncPeriod2);

            // At this point the refresh rate should have changed already, however in rare
            // cases the implementation might have missed the deadline. In this case a new
            // timeline should have been provided.
            auto newTimeline = mComposerCallback->takeLastVsyncPeriodChangeTimeline();
            if (timeline.refreshRequired && params.refreshMiss) {
                EXPECT_TRUE(newTimeline.has_value());
            }

            if (newTimeline.has_value()) {
                if (newTimeline->refreshRequired) {
                    sendRefreshFrame(display, &newTimeline.value());
                }
                waitForVsyncPeriodChange(display.get(), newTimeline.value(),
                                         constraints.desiredTimeNanos, vsyncPeriod1, vsyncPeriod2);
            }

            VsyncPeriodNanos vsyncPeriodNanos;
            EXPECT_EQ(Error::NONE,
                      mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos));
            EXPECT_EQ(vsyncPeriodNanos, vsyncPeriod2);
        });
    }
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = false});
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_Delayed) {
    Test_setActiveConfigWithConstraints({.delayForChange = 300'000'000,  // 300ms
                                         .refreshMiss = false});
}

TEST_P(GraphicsComposerHidlTest, setActiveConfigWithConstraints_MissRefresh) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = true});
}

TEST_P(GraphicsComposerHidlTest, setAutoLowLatencyModeBadDisplay) {
    EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, true));
    EXPECT_EQ(Error::BAD_DISPLAY, mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, false));
}

TEST_P(GraphicsComposerHidlTest, setAutoLowLatencyMode) {
    for (const auto& display : mDisplays) {
        std::vector<DisplayCapability> capabilities;
        const auto error = mComposerClient->getDisplayCapabilities(display.get(), &capabilities);
        EXPECT_EQ(Error::NONE, error);

        const bool allmSupport =
                std::find(capabilities.begin(), capabilities.end(),
                          DisplayCapability::AUTO_LOW_LATENCY_MODE) != capabilities.end();

        if (!allmSupport) {
            EXPECT_EQ(Error::UNSUPPORTED,
                      mComposerClient->setAutoLowLatencyMode(display.get(), true));
            EXPECT_EQ(Error::UNSUPPORTED,
                      mComposerClient->setAutoLowLatencyMode(display.get(), false));
            GTEST_SUCCEED() << "Auto Low Latency Mode is not supported on display "
                            << std::to_string(display.get()) << ", skipping test";
            return;
        }

        EXPECT_EQ(Error::NONE, mComposerClient->setAutoLowLatencyMode(display.get(), true));
        EXPECT_EQ(Error::NONE, mComposerClient->setAutoLowLatencyMode(display.get(), false));
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
    for (const auto& display : mDisplays) {
        supportedContentTypes.clear();
        const auto error =
                mComposerClient->getSupportedContentTypes(display.get(), &supportedContentTypes);
        const bool noneSupported =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          ContentType::NONE) != supportedContentTypes.end();
        EXPECT_EQ(Error::NONE, error);
        EXPECT_FALSE(noneSupported);
    }
}

TEST_P(GraphicsComposerHidlTest, setContentTypeNoneAlwaysAccepted) {
    for (const auto& display : mDisplays) {
        const auto error = mComposerClient->setContentType(display.get(), ContentType::NONE);
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
                        << std::to_string(display) << ", skipping test";
        return;
    }

    EXPECT_EQ(Error::NONE, mComposerClient->setContentType(display, contentType));
    EXPECT_EQ(Error::NONE, mComposerClient->setContentType(display, ContentType::NONE));
}

void GraphicsComposerHidlTest::Test_setContentType(const ContentType& contentType,
                                                   const char* contentTypeStr) {
    for (const auto& display : mDisplays) {
        std::vector<ContentType> supportedContentTypes;
        const auto error =
                mComposerClient->getSupportedContentTypes(display.get(), &supportedContentTypes);
        EXPECT_EQ(Error::NONE, error);

        Test_setContentTypeForDisplay(display.get(), supportedContentTypes, contentType,
                                      contentTypeStr);
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

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IComposer::descriptor)),
        android::hardware::PrintInstanceNameToString);

TEST_P(GraphicsComposerHidlTest, getLayerGenericMetadataKeys) {
    std::vector<IComposerClient::LayerGenericMetadataKey> keys;
    mComposerClient->getLayerGenericMetadataKeys(&keys);

    std::regex reverseDomainName("^[a-zA-Z-]{2,}(\\.[a-zA-Z0-9-]+)+$");
    std::unordered_set<std::string> uniqueNames;
    for (const auto& key : keys) {
        std::string name(key.name.c_str());

        // Keys must not start with 'android' or 'com.android'
        ASSERT_FALSE(name.find("android") == 0);
        ASSERT_FALSE(name.find("com.android") == 0);

        // Keys must be in reverse domain name format
        ASSERT_TRUE(std::regex_match(name, reverseDomainName));

        // Keys must be unique within this list
        const auto& [iter, inserted] = uniqueNames.insert(name);
        ASSERT_TRUE(inserted);
    }
}

/*
 * Test that no two display configs are exactly the same.
 */
TEST_P(GraphicsComposerHidlTest, GetDisplayConfigNoRepetitions) {
    for (const auto& display : mDisplays) {
        std::vector<Config> configs = mComposerClient->getDisplayConfigs(display.get());
        for (int i = 0; i < configs.size(); i++) {
            for (int j = i + 1; j < configs.size(); j++) {
                const int32_t width1 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[i], IComposerClient::Attribute::WIDTH);
                const int32_t height1 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[i], IComposerClient::Attribute::HEIGHT);
                const int32_t vsyncPeriod1 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[i], IComposerClient::Attribute::VSYNC_PERIOD);
                const int32_t group1 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[i], IComposerClient::Attribute::CONFIG_GROUP);

                const int32_t width2 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[j], IComposerClient::Attribute::WIDTH);
                const int32_t height2 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[j], IComposerClient::Attribute::HEIGHT);
                const int32_t vsyncPeriod2 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[j], IComposerClient::Attribute::VSYNC_PERIOD);
                const int32_t group2 = mComposerClient->getDisplayAttribute_2_4(
                        display.get(), configs[j], IComposerClient::Attribute::CONFIG_GROUP);

                ASSERT_FALSE(width1 == width2 && height1 == height2 &&
                             vsyncPeriod1 == vsyncPeriod2 && group1 == group2);
            }
        }
    }
}

}  // namespace
}  // namespace vts
}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    using namespace std::chrono_literals;
    if (!android::base::WaitForProperty("init.svc.surfaceflinger", "stopped", 10s)) {
        ALOGE("Failed to stop init.svc.surfaceflinger");
        return -1;
    }

    return RUN_ALL_TESTS();
}
