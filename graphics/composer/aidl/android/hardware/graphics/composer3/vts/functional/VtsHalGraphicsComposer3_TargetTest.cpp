
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/ProcessState.h>
#include <composer-vts/include/GraphicsComposerCallback.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#pragma push_macro("LOG_TAG")
#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_TargetTest"

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using namespace std::chrono_literals;

class VtsDisplay {
  public:
    VtsDisplay(int64_t displayId, int32_t displayWidth, int32_t displayHeight)
        : mDisplayId(displayId), mDisplayWidth(displayWidth), mDisplayHeight(displayHeight) {}

    int64_t get() const { return mDisplayId; }

    void setDimensions(int32_t displayWidth, int32_t displayHeight) {
        mDisplayWidth = displayWidth;
        mDisplayHeight = displayHeight;
    }

  private:
    const int64_t mDisplayId;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
};

class GraphicsComposerAidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        std::string name = GetParam();
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer = IComposer::fromBinder(binder));
        ASSERT_NE(mComposer, nullptr);
        ASSERT_NO_FATAL_FAILURE(mComposer->createClient(&mComposerClient));
        mInvalidDisplayId = GetInvalidDisplayId();

        mComposerCallback = ::ndk::SharedRefBase::make<GraphicsComposerCallback>();
        EXPECT_TRUE(mComposerClient->registerCallback(mComposerCallback).isOk());

        // assume the first displays are built-in and are never removed
        mDisplays = waitForDisplays();
    }

    // returns an invalid display id (one that has not been registered to a
    // display.  Currently assuming that a device will never have close to
    // std::numeric_limit<uint64_t>::max() displays registered while running tests
    int64_t GetInvalidDisplayId() {
        int64_t id = std::numeric_limits<int64_t>::max();
        while (id > 0) {
            if (std::none_of(mDisplays.begin(), mDisplays.end(),
                             [&](const VtsDisplay& display) { return id == display.get(); })) {
                return id;
            }
            id--;
        }

        return 0;
    }

    std::vector<VtsDisplay> waitForDisplays() {
        while (true) {
            // Sleep for a small period of time to allow all built-in displays
            // to post hotplug events
            std::this_thread::sleep_for(5ms);
            std::vector<int64_t> displays = mComposerCallback->getDisplays();
            if (displays.empty()) {
                continue;
            }

            std::vector<VtsDisplay> vtsDisplays;
            vtsDisplays.reserve(displays.size());
            for (int64_t display : displays) {
                int32_t activeConfig;
                EXPECT_TRUE(mComposerClient->getActiveConfig(display, &activeConfig).isOk());
                int32_t displayWidth;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display, activeConfig,
                                                          DisplayAttribute::WIDTH, &displayWidth)
                                    .isOk());
                int32_t displayHeight;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display, activeConfig,
                                                          DisplayAttribute::HEIGHT, &displayHeight)
                                    .isOk());
                vtsDisplays.emplace_back(VtsDisplay{display, displayWidth, displayHeight});
            }

            return vtsDisplays;
        }
    }

    // returns an invalid config id which is std::numeric_limit<int32_t>::max()
    int32_t GetInvalidConfigId() { return IComposerClient::INVALID_CONFIGURATION; }

    ndk::ScopedAStatus setActiveConfigWithConstraints(
            VtsDisplay& display, int32_t config, const VsyncPeriodChangeConstraints& constraints,
            VsyncPeriodChangeTimeline* timeline) {
        auto error = mComposerClient->setActiveConfigWithConstraints(display.get(), config,
                                                                     constraints, timeline);
        if (error.isOk()) {
            int32_t displayWidth;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::WIDTH, &displayWidth)
                                .isOk());
            int32_t displayHeight;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::HEIGHT, &displayHeight)
                                .isOk());
            display.setDimensions(displayWidth, displayHeight);
        }
        return error;
    }

    void Test_setContentTypeForDisplay(const int64_t& display,
                                       const std::vector<ContentType>& capabilities,
                                       const ContentType& contentType, const char* contentTypeStr) {
        const bool contentTypeSupport = std::find(capabilities.begin(), capabilities.end(),
                                                  contentType) != capabilities.end();

        if (!contentTypeSupport) {
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED,
                      mComposerClient->setContentType(display, contentType)
                              .getServiceSpecificError());
            GTEST_SUCCEED() << contentTypeStr << " content type is not supported on display "
                            << std::to_string(display) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setContentType(display, contentType).isOk());
        EXPECT_TRUE(mComposerClient->setContentType(display, ContentType::NONE).isOk());
    }

    void Test_setContentType(const ContentType& contentType, const char* contentTypeStr) {
        for (const auto& display : mDisplays) {
            std::vector<ContentType> supportedContentTypes;
            const auto error = mComposerClient->getSupportedContentTypes(display.get(),
                                                                         &supportedContentTypes);
            EXPECT_TRUE(error.isOk());

            Test_setContentTypeForDisplay(display.get(), supportedContentTypes, contentType,
                                          contentTypeStr);
        }
    }

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;
    int64_t mInvalidDisplayId;
    int64_t mPrimaryDisplay;
    std::vector<VtsDisplay> mDisplays;
    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
};

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilitiesBadDisplay) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mInvalidDisplayId, &capabilities);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilities) {
    for (const auto& display : mDisplays) {
        std::vector<DisplayCapability> capabilities;

        EXPECT_TRUE(mComposerClient->getDisplayCapabilities(display.get(), &capabilities).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, GetDisplayIdentificationData) {
    DisplayIdentification displayIdentification0;

    const auto error =
            mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &displayIdentification0);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        return;
    }
    ASSERT_TRUE(error.isOk()) << "failed to get display identification data";
    ASSERT_FALSE(displayIdentification0.data.empty());

    constexpr size_t kEdidBlockSize = 128;
    ASSERT_TRUE(displayIdentification0.data.size() % kEdidBlockSize == 0)
            << "EDID blob length is not a multiple of " << kEdidBlockSize;

    const uint8_t kEdidHeader[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    ASSERT_TRUE(std::equal(std::begin(kEdidHeader), std::end(kEdidHeader),
                           displayIdentification0.data.begin()))
            << "EDID blob doesn't start with the fixed EDID header";
    ASSERT_EQ(0, std::accumulate(displayIdentification0.data.begin(),
                                 displayIdentification0.data.begin() + kEdidBlockSize,
                                 static_cast<uint8_t>(0)))
            << "EDID base block doesn't checksum";

    DisplayIdentification displayIdentification1;
    ASSERT_TRUE(
            mComposerClient->getDisplayIdentificationData(mPrimaryDisplay, &displayIdentification1)
                    .isOk());

    ASSERT_EQ(displayIdentification0.port, displayIdentification1.port) << "ports are not stable";
    ASSERT_TRUE(displayIdentification0.data.size() == displayIdentification1.data.size() &&
                std::equal(displayIdentification0.data.begin(), displayIdentification0.data.end(),
                           displayIdentification1.data.begin()))
            << "data is not stable";
}

TEST_P(GraphicsComposerAidlTest, GetHdrCapabilities) {
    HdrCapabilities hdrCapabilities;
    const auto error = mComposerClient->getHdrCapabilities(mPrimaryDisplay, &hdrCapabilities);

    ASSERT_TRUE(error.isOk());
    ASSERT_TRUE(hdrCapabilities.maxLuminance >= hdrCapabilities.minLuminance);
}

TEST_P(GraphicsComposerAidlTest, GetPerFrameMetadataKeys) {
    std::vector<PerFrameMetadataKey> keys;
    const auto error = mComposerClient->getPerFrameMetadataKeys(mPrimaryDisplay, &keys);

    if (error.isOk()) {
        EXPECT_EQ(EX_NONE, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetReadbackBufferAttributes) {
    ReadbackBufferAttributes readBackBufferAttributes;
    const auto error = mComposerClient->getReadbackBufferAttributes(mPrimaryDisplay,
                                                                    &readBackBufferAttributes);

    if (error.isOk()) {
        EXPECT_EQ(EX_NONE, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntents) {
    std::vector<ColorMode> modes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &modes).isOk());
    for (auto mode : modes) {
        std::vector<RenderIntent> intents;
        EXPECT_TRUE(mComposerClient->getRenderIntents(mPrimaryDisplay, mode, &intents).isOk());

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

TEST_P(GraphicsComposerAidlTest, GetRenderIntentsBadDisplay) {
    std::vector<ColorMode> modes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &modes).isOk());
    for (auto mode : modes) {
        std::vector<RenderIntent> renderIntents;
        const auto error =
                mComposerClient->getRenderIntents(mInvalidDisplayId, mode, &renderIntents);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntentsBadParameter) {
    std::vector<RenderIntent> renderIntents;
    const auto error = mComposerClient->getRenderIntents(
            mPrimaryDisplay, static_cast<ColorMode>(-1), &renderIntents);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetColorModes) {
    std::vector<ColorMode> colorModes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &colorModes).isOk());

    auto native = std::find(colorModes.cbegin(), colorModes.cend(), ColorMode::NATIVE);
    ASSERT_NE(colorModes.cend(), native);
}

TEST_P(GraphicsComposerAidlTest, GetColorModeBadDisplay) {
    std::vector<ColorMode> colorModes;
    const auto error = mComposerClient->getColorModes(mInvalidDisplayId, &colorModes);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetColorMode) {
    std::vector<ColorMode> colorModes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &colorModes).isOk());
    for (auto mode : colorModes) {
        std::vector<RenderIntent> intents;
        EXPECT_TRUE(mComposerClient->getRenderIntents(mPrimaryDisplay, mode, &intents).isOk())
                << "failed to get render intents";
        for (auto intent : intents) {
            const auto error = mComposerClient->setColorMode(mPrimaryDisplay, mode, intent);
            EXPECT_TRUE(error.isOk() ||
                        IComposerClient::EX_UNSUPPORTED == error.getServiceSpecificError())
                    << "failed to set color mode";
        }
    }

    const auto error = mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE,
                                                     RenderIntent::COLORIMETRIC);
    EXPECT_TRUE(error.isOk() || IComposerClient::EX_UNSUPPORTED == error.getServiceSpecificError())
            << "failed to set color mode";
}

TEST_P(GraphicsComposerAidlTest, SetColorModeBadDisplay) {
    auto const error = mComposerClient->setColorMode(mInvalidDisplayId, ColorMode::NATIVE,
                                                     RenderIntent::COLORIMETRIC);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetColorModeBadParameter) {
    const auto colorModeError = mComposerClient->setColorMode(
            mPrimaryDisplay, static_cast<ColorMode>(-1), RenderIntent::COLORIMETRIC);

    EXPECT_FALSE(colorModeError.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, colorModeError.getServiceSpecificError());

    const auto renderIntentError = mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE,
                                                                 static_cast<RenderIntent>(-1));

    EXPECT_FALSE(renderIntentError.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, renderIntentError.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSamplingAttributes) {
    int constexpr invalid = -1;

    DisplayContentSamplingAttributes format;
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(mPrimaryDisplay, &format);

    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_TRUE(error.isOk());
    EXPECT_NE(format.format, static_cast<common::PixelFormat>(invalid));
    EXPECT_NE(format.dataspace, static_cast<common::Dataspace>(invalid));
    EXPECT_NE(format.componentMask, static_cast<FormatColorComponent>(invalid));
};

TEST_P(GraphicsComposerAidlTest, SetDisplayedContentSamplingEnabled) {
    auto const maxFrames = 10;
    FormatColorComponent enableAllComponents = FormatColorComponent::FORMAT_COMPONENT_0;
    auto error = mComposerClient->setDisplayedContentSamplingEnabled(
            mPrimaryDisplay, true, enableAllComponents, maxFrames);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }
    EXPECT_TRUE(error.isOk());

    error = mComposerClient->setDisplayedContentSamplingEnabled(mPrimaryDisplay, false,
                                                                enableAllComponents, maxFrames);
    EXPECT_TRUE(error.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSample) {
    DisplayContentSamplingAttributes displayContentSamplingAttributes;
    int constexpr invalid = -1;
    displayContentSamplingAttributes.format = static_cast<common::PixelFormat>(invalid);
    displayContentSamplingAttributes.dataspace = static_cast<common::Dataspace>(invalid);
    displayContentSamplingAttributes.componentMask = static_cast<FormatColorComponent>(invalid);
    EXPECT_TRUE(mComposerClient
                        ->getDisplayedContentSamplingAttributes(mPrimaryDisplay,
                                                                &displayContentSamplingAttributes)
                        .isOk());

    uint64_t maxFrames = 10;
    uint64_t timestamp = 0;
    uint64_t frameCount = 0;
    DisplayContentSample displayContentSample;
    auto error = mComposerClient->getDisplayedContentSample(mPrimaryDisplay, maxFrames, timestamp,
                                                            &displayContentSample);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_TRUE(error.isOk());
    EXPECT_LE(frameCount, maxFrames);
    std::vector<std::vector<int64_t>> histogram = {
            displayContentSample.sampleComponent0, displayContentSample.sampleComponent1,
            displayContentSample.sampleComponent2, displayContentSample.sampleComponent3};

    for (auto i = 0; i < histogram.size(); i++) {
        if (static_cast<int>(displayContentSamplingAttributes.componentMask) & (1 << i)) {
            EXPECT_NE(histogram[i].size(), 0);
        } else {
            EXPECT_EQ(histogram[i].size(), 0);
        }
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayCapabilitiesBasic) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    const bool hasDozeSupport = std::find(capabilities.begin(), capabilities.end(),
                                          DisplayCapability::DOZE) != capabilities.end();
    bool isDozeSupported = false;
    EXPECT_TRUE(mComposerClient->getDozeSupport(mPrimaryDisplay, &isDozeSupported).isOk());
    EXPECT_EQ(hasDozeSupport, isDozeSupported);

    bool hasBrightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                          DisplayCapability::BRIGHTNESS) != capabilities.end();
    bool isBrightnessSupported = false;
    EXPECT_TRUE(
            mComposerClient->getDisplayBrightnessSupport(mPrimaryDisplay, &isBrightnessSupported)
                    .isOk());
    EXPECT_EQ(isBrightnessSupported, hasBrightnessSupport);
}

/*
 * Test that if brightness operations are supported, setDisplayBrightness works as expected.
 */
TEST_P(GraphicsComposerAidlTest, setDisplayBrightness) {
    std::vector<DisplayCapability> capabilities;
    auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    bool brightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                       DisplayCapability::BRIGHTNESS) != capabilities.end();
    if (!brightnessSupport) {
        EXPECT_EQ(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.5f)
                          .getServiceSpecificError(),
                  IComposerClient::EX_UNSUPPORTED);
        GTEST_SUCCEED() << "Brightness operations are not supported";
        return;
    }

    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.0f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 0.5f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, 1.0f).isOk());
    EXPECT_TRUE(mComposerClient->setDisplayBrightness(mPrimaryDisplay, -1.0f).isOk());

    error = mComposerClient->setDisplayBrightness(mPrimaryDisplay, +2.0f);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(error.getServiceSpecificError(), IComposerClient::EX_BAD_PARAMETER);

    error = mComposerClient->setDisplayBrightness(mPrimaryDisplay, -2.0f);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(error.getServiceSpecificError(), IComposerClient::EX_BAD_PARAMETER);
}

TEST_P(GraphicsComposerAidlTest, getDisplayConnectionType) {
    DisplayConnectionType type;
    EXPECT_FALSE(mComposerClient->getDisplayConnectionType(mInvalidDisplayId, &type).isOk());
    for (const auto& display : mDisplays) {
        EXPECT_TRUE(mComposerClient->getDisplayConnectionType(display.get(), &type).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayAttribute) {
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        mComposerClient->getDisplayConfigs(display.get(), &configs);
        for (const auto& config : configs) {
            const std::array<DisplayAttribute, 4> requiredAttributes = {{
                    DisplayAttribute::WIDTH,
                    DisplayAttribute::HEIGHT,
                    DisplayAttribute::VSYNC_PERIOD,
                    DisplayAttribute::CONFIG_GROUP,
            }};
            int32_t value;
            for (const auto& attribute : requiredAttributes) {
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config, attribute, &value)
                                    .isOk());
                EXPECT_NE(-1, value);
            }

            const std::array<DisplayAttribute, 2> optionalAttributes = {{
                    DisplayAttribute::DPI_X,
                    DisplayAttribute::DPI_Y,
            }};
            for (const auto& attribute : optionalAttributes) {
                const auto error = mComposerClient->getDisplayAttribute(display.get(), config,
                                                                        attribute, &value);
                if (error.isOk()) {
                    EXPECT_EQ(EX_NONE, error.getServiceSpecificError());
                } else {
                    EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
                }
            }
        }
    }
}

TEST_P(GraphicsComposerAidlTest, checkConfigsAreValid) {
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        mComposerClient->getDisplayConfigs(display.get(), &configs);

        EXPECT_FALSE(std::any_of(configs.begin(), configs.end(), [](auto config) {
            return config == IComposerClient::INVALID_CONFIGURATION;
        }));
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayAttributeConfigsInAGroupDifferOnlyByVsyncPeriod) {
    struct Resolution {
        int32_t width;
        int32_t height;
    };
    struct Dpi {
        int32_t x;
        int32_t y;
    };
    for (const auto& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
        std::unordered_map<int32_t, Resolution> configGroupToResolutionMap;
        std::unordered_map<int32_t, Dpi> configGroupToDpiMap;
        for (const auto& config : configs) {
            int32_t configGroup = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup)
                                .isOk());
            int32_t width = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::WIDTH, &width)
                                .isOk());
            int32_t height = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::HEIGHT, &height)
                                .isOk());
            if (configGroupToResolutionMap.find(configGroup) == configGroupToResolutionMap.end()) {
                configGroupToResolutionMap[configGroup] = {width, height};
            }
            EXPECT_EQ(configGroupToResolutionMap[configGroup].width, width);
            EXPECT_EQ(configGroupToResolutionMap[configGroup].height, height);

            int32_t dpiX = -1;
            mComposerClient->getDisplayAttribute(display.get(), config, DisplayAttribute::DPI_X,
                                                 &dpiX);

            int32_t dpiY = -1;
            mComposerClient->getDisplayAttribute(display.get(), config, DisplayAttribute::DPI_Y,
                                                 &dpiY);
            if (dpiX == -1 && dpiY == -1) {
                continue;
            }

            if (configGroupToDpiMap.find(configGroup) == configGroupToDpiMap.end()) {
                configGroupToDpiMap[configGroup] = {dpiX, dpiY};
            }
            EXPECT_EQ(configGroupToDpiMap[configGroup].x, dpiX);
            EXPECT_EQ(configGroupToDpiMap[configGroup].y, dpiY);
        }
    }
}

TEST_P(GraphicsComposerAidlTest, getDisplayVsyncPeriod_BadDisplay) {
    int32_t vsyncPeriodNanos;
    const auto error = mComposerClient->getDisplayVsyncPeriod(mInvalidDisplayId, &vsyncPeriodNanos);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints_BadDisplay) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();
    int32_t config = 0;
    auto const error = mComposerClient->setActiveConfigWithConstraints(mInvalidDisplayId, config,
                                                                       constraints, &timeline);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setActiveConfigWithConstraints_BadConfig) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        int32_t invalidConfigId = GetInvalidConfigId();
        const auto error =
                setActiveConfigWithConstraints(display, invalidConfigId, constraints, &timeline);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_CONFIG, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, setAutoLowLatencyModeBadDisplay) {
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY,
              mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, true)
                      .getServiceSpecificError());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY,
              mComposerClient->setAutoLowLatencyMode(mInvalidDisplayId, false)
                      .getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, setAutoLowLatencyMode) {
    for (const auto& display : mDisplays) {
        std::vector<DisplayCapability> capabilities;
        const auto error = mComposerClient->getDisplayCapabilities(display.get(), &capabilities);
        EXPECT_TRUE(error.isOk());

        const bool allmSupport =
                std::find(capabilities.begin(), capabilities.end(),
                          DisplayCapability::AUTO_LOW_LATENCY_MODE) != capabilities.end();

        if (!allmSupport) {
            const auto errorIsOn = mComposerClient->setAutoLowLatencyMode(display.get(), true);
            EXPECT_FALSE(errorIsOn.isOk());
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errorIsOn.getServiceSpecificError());
            const auto errorIsOff = mComposerClient->setAutoLowLatencyMode(display.get(), false);
            EXPECT_FALSE(errorIsOff.isOk());
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errorIsOff.getServiceSpecificError());
            GTEST_SUCCEED() << "Auto Low Latency Mode is not supported on display "
                            << std::to_string(display.get()) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.get(), true).isOk());
        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.get(), false).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, getSupportedContentTypesBadDisplay) {
    std::vector<ContentType> supportedContentTypes;
    const auto error =
            mComposerClient->getSupportedContentTypes(mInvalidDisplayId, &supportedContentTypes);
    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, getSupportedContentTypes) {
    std::vector<ContentType> supportedContentTypes;
    for (const auto& display : mDisplays) {
        supportedContentTypes.clear();
        const auto error =
                mComposerClient->getSupportedContentTypes(display.get(), &supportedContentTypes);

        ASSERT_TRUE(error.isOk());

        const bool noneSupported =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          ContentType::NONE) != supportedContentTypes.end();
        EXPECT_FALSE(noneSupported);
    }
}

TEST_P(GraphicsComposerAidlTest, setContentTypeNoneAlwaysAccepted) {
    for (const auto& display : mDisplays) {
        const auto error = mComposerClient->setContentType(display.get(), ContentType::NONE);
        EXPECT_TRUE(error.isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, setContentTypeBadDisplay) {
    constexpr ContentType types[] = {ContentType::NONE, ContentType::GRAPHICS, ContentType::PHOTO,
                                     ContentType::CINEMA, ContentType::GAME};
    for (const auto& type : types) {
        auto const error = mComposerClient->setContentType(mInvalidDisplayId, type);

        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, setGraphicsContentType) {
    Test_setContentType(ContentType::GRAPHICS, "GRAPHICS");
}

TEST_P(GraphicsComposerAidlTest, setPhotoContentType) {
    Test_setContentType(ContentType::PHOTO, "PHOTO");
}

TEST_P(GraphicsComposerAidlTest, setCinemaContentType) {
    Test_setContentType(ContentType::CINEMA, "CINEMA");
}

TEST_P(GraphicsComposerAidlTest, setGameContentType) {
    Test_setContentType(ContentType::GAME, "GAME");
}

TEST_P(GraphicsComposerAidlTest, getLayerGenericMetadataKeys) {
    std::vector<LayerGenericMetadataKey> keys;
    EXPECT_TRUE(mComposerClient->getLayerGenericMetadataKeys(&keys).isOk());

    std::regex reverseDomainName("^[a-zA-Z-]{2,}(\\.[a-zA-Z0-9-]+)+$");
    std::unordered_set<std::string> uniqueNames;
    for (const auto& key : keys) {
        std::string name(key.name.c_str());

        // Keys must not start with 'android' or 'com.android'
        EXPECT_FALSE(name.find("android") == 0);
        EXPECT_FALSE(name.find("com.android") == 0);

        // Keys must be in reverse domain name format
        EXPECT_TRUE(std::regex_match(name, reverseDomainName));

        // Keys must be unique within this list
        const auto& [iter, inserted] = uniqueNames.insert(name);
        EXPECT_TRUE(inserted);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
}  // namespace
}  // namespace aidl::android::hardware::graphics::composer3::vts

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    using namespace std::chrono_literals;
    if (!android::base::WaitForProperty("init.svc.surfaceflinger", "stopped", 10s)) {
        ALOGE("Failed to stop init.svc.surfaceflinger");
        return -1;
    }
    return RUN_ALL_TESTS();
}
