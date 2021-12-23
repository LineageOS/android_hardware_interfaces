// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <algorithm>
#include <numeric>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "composer-vts/include/GraphicsComposerCallback.h"

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop  // ignored "-Wconversion

#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_TargetTest"

namespace aidl::android::hardware::graphics::composer3::vts {
namespace {

using namespace std::chrono_literals;

using ::android::GraphicBuffer;
using ::android::sp;

class VtsDisplay {
  public:
    VtsDisplay(int64_t displayId, int32_t displayWidth, int32_t displayHeight)
        : mDisplayId(displayId), mDisplayWidth(displayWidth), mDisplayHeight(displayHeight) {}

    int64_t get() const { return mDisplayId; }

    FRect getCrop() const {
        return {0, 0, static_cast<float>(mDisplayWidth), static_cast<float>(mDisplayHeight)};
    }

    Rect getFrameRect() const { return {0, 0, mDisplayWidth, mDisplayHeight}; }

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

        mComposerCallback = ::ndk::SharedRefBase::make<GraphicsComposerCallback>();
        EXPECT_TRUE(mComposerClient->registerCallback(mComposerCallback).isOk());

        // assume the first displays are built-in and are never removed
        mDisplays = waitForDisplays();
        mPrimaryDisplay = mDisplays[0].get();
        ASSERT_NO_FATAL_FAILURE(mInvalidDisplayId = GetInvalidDisplayId());

        int32_t activeConfig;
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &activeConfig).isOk());
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                  DisplayAttribute::WIDTH, &mDisplayWidth)
                            .isOk());
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                  DisplayAttribute::HEIGHT, &mDisplayHeight)
                            .isOk());

        // explicitly disable vsync
        for (const auto& display : mDisplays) {
            EXPECT_TRUE(mComposerClient->setVsyncEnabled(display.get(), false).isOk());
        }
        mComposerCallback->setVsyncAllowed(false);
    }

    void TearDown() override {
        destroyAllLayers();
        if (mComposerCallback != nullptr) {
            EXPECT_EQ(0, mComposerCallback->getInvalidHotplugCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidRefreshCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidVsyncPeriodChangeCount());
            EXPECT_EQ(0, mComposerCallback->getInvalidSeamlessPossibleCount());
        }
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

    int64_t createLayer(const VtsDisplay& display) {
        int64_t layer;
        EXPECT_TRUE(mComposerClient->createLayer(display.get(), kBufferSlotCount, &layer).isOk());

        auto resourceIt = mDisplayResources.find(display.get());
        if (resourceIt == mDisplayResources.end()) {
            resourceIt = mDisplayResources.insert({display.get(), DisplayResource(false)}).first;
        }

        EXPECT_TRUE(resourceIt->second.layers.insert(layer).second)
                << "duplicated layer id " << layer;

        return layer;
    }

    void destroyAllLayers() {
        for (const auto& it : mDisplayResources) {
            auto display = it.first;
            const DisplayResource& resource = it.second;

            for (auto layer : resource.layers) {
                const auto error = mComposerClient->destroyLayer(display, layer);
                EXPECT_TRUE(error.isOk());
            }

            if (resource.isVirtual) {
                const auto error = mComposerClient->destroyVirtualDisplay(display);
                EXPECT_TRUE(error.isOk());
            }
        }
        mDisplayResources.clear();
    }

    void destroyLayer(const VtsDisplay& display, int64_t layer) {
        auto const error = mComposerClient->destroyLayer(display.get(), layer);
        ASSERT_TRUE(error.isOk()) << "failed to destroy layer " << layer;

        auto resourceIt = mDisplayResources.find(display.get());
        ASSERT_NE(mDisplayResources.end(), resourceIt);
        resourceIt->second.layers.erase(layer);
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

        // Although 0 could be an invalid display, a return value of 0
        // from GetInvalidDisplayId means all other ids are in use, a condition which
        // we are assuming a device will never have
        EXPECT_NE(0, id);
        return id;
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

    struct TestParameters {
        nsecs_t delayForChange;
        bool refreshMiss;
    };

    // Keep track of all virtual displays and layers.  When a test fails with
    // ASSERT_*, the destructor will clean up the resources for the test.
    struct DisplayResource {
        DisplayResource(bool isVirtual_) : isVirtual(isVirtual_) {}

        bool isVirtual;
        std::unordered_set<int64_t> layers;
    };

    std::shared_ptr<IComposer> mComposer;
    std::shared_ptr<IComposerClient> mComposerClient;
    int64_t mInvalidDisplayId;
    int64_t mPrimaryDisplay;
    std::vector<VtsDisplay> mDisplays;
    std::shared_ptr<GraphicsComposerCallback> mComposerCallback;
    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;
    std::unordered_map<int64_t, DisplayResource> mDisplayResources;
    int32_t mDisplayWidth;
    int32_t mDisplayHeight;
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

TEST_P(GraphicsComposerAidlTest, DumpDebugInfo) {
    std::string debugInfo;
    EXPECT_TRUE(mComposer->dumpDebugInfo(&debugInfo).isOk());
}

TEST_P(GraphicsComposerAidlTest, CreateClientSingleton) {
    std::shared_ptr<IComposerClient> composerClient;
    const auto error = mComposer->createClient(&composerClient);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_NO_RESOURCES, error.getServiceSpecificError());
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

    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "getPerFrameMetadataKeys is not supported";
        return;
    }
    EXPECT_TRUE(error.isOk());
    ASSERT_TRUE(keys.size() >= 0);
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
    std::vector<ColorMode> colorModes;
    EXPECT_TRUE(mComposerClient->getColorModes(mPrimaryDisplay, &colorModes).isOk());
    for (auto mode : colorModes) {
        std::vector<RenderIntent> intents;
        EXPECT_TRUE(mComposerClient->getRenderIntents(mPrimaryDisplay, mode, &intents).isOk())
                << "failed to get render intents";
        for (auto intent : intents) {
            auto const error = mComposerClient->setColorMode(mInvalidDisplayId, mode, intent);

            EXPECT_FALSE(error.isOk());
            ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
        }
    }
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
    auto error = mComposerClient->getDisplayedContentSamplingAttributes(
            mPrimaryDisplay, &displayContentSamplingAttributes);
    if (error.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Sampling attributes aren't supported on this device, test skipped";
        return;
    }

    int64_t maxFrames = 10;
    int64_t timestamp = 0;
    int64_t frameCount = 0;
    DisplayContentSample displayContentSample;
    error = mComposerClient->getDisplayedContentSample(mPrimaryDisplay, maxFrames, timestamp,
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

    for (size_t i = 0; i < histogram.size(); i++) {
        const bool shouldHaveHistogram =
                static_cast<int>(displayContentSamplingAttributes.componentMask) & (1 << i);
        EXPECT_EQ(shouldHaveHistogram, !histogram[i].empty());
    }
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
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
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
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());

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

TEST_P(GraphicsComposerAidlTest, CreateVirtualDisplay) {
    int32_t maxVirtualDisplayCount;
    EXPECT_TRUE(mComposerClient->getMaxVirtualDisplayCount(&maxVirtualDisplayCount).isOk());
    if (maxVirtualDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    VirtualDisplay virtualDisplay;

    EXPECT_TRUE(mComposerClient
                        ->createVirtualDisplay(64, 64, common::PixelFormat::IMPLEMENTATION_DEFINED,
                                               kBufferSlotCount, &virtualDisplay)
                        .isOk());

    ASSERT_TRUE(mDisplayResources.insert({virtualDisplay.display, DisplayResource(true)}).second)
            << "duplicated virtual display id " << virtualDisplay.display;

    EXPECT_TRUE(mComposerClient->destroyVirtualDisplay(virtualDisplay.display).isOk());
}

TEST_P(GraphicsComposerAidlTest, DestroyVirtualDisplayBadDisplay) {
    int32_t maxDisplayCount = 0;
    EXPECT_TRUE(mComposerClient->getMaxVirtualDisplayCount(&maxDisplayCount).isOk());
    if (maxDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }
    const auto error = mComposerClient->destroyVirtualDisplay(mInvalidDisplayId);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, CreateLayer) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    EXPECT_TRUE(mComposerClient->destroyLayer(mPrimaryDisplay, layer).isOk());
}

TEST_P(GraphicsComposerAidlTest, CreateLayerBadDisplay) {
    int64_t layer;
    const auto error = mComposerClient->createLayer(mInvalidDisplayId, kBufferSlotCount, &layer);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, DestroyLayerBadDisplay) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    const auto error = mComposerClient->destroyLayer(mInvalidDisplayId, layer);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
    EXPECT_TRUE(mComposerClient->destroyLayer(mPrimaryDisplay, layer).isOk());
}

TEST_P(GraphicsComposerAidlTest, DestroyLayerBadLayerError) {
    // We haven't created any layers yet, so any id should be invalid
    const auto error = mComposerClient->destroyLayer(mPrimaryDisplay, 1);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_LAYER, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetActiveConfigBadDisplay) {
    int32_t config;
    const auto error = mComposerClient->getActiveConfig(mInvalidDisplayId, &config);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayConfig) {
    std::vector<int32_t> configs;
    EXPECT_TRUE(mComposerClient->getDisplayConfigs(mPrimaryDisplay, &configs).isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayConfigBadDisplay) {
    std::vector<int32_t> configs;
    const auto error = mComposerClient->getDisplayConfigs(mInvalidDisplayId, &configs);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayName) {
    std::string displayName;
    EXPECT_TRUE(mComposerClient->getDisplayName(mPrimaryDisplay, &displayName).isOk());
}

TEST_P(GraphicsComposerAidlTest, SetClientTargetSlotCount) {
    EXPECT_TRUE(
            mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kBufferSlotCount).isOk());
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfig) {
    std::vector<int32_t> configs;
    EXPECT_TRUE(mComposerClient->getDisplayConfigs(mPrimaryDisplay, &configs).isOk());
    for (auto config : configs) {
        EXPECT_TRUE(mComposerClient->setActiveConfig(mPrimaryDisplay, config).isOk());
        int32_t config1;
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &config1).isOk());
        EXPECT_EQ(config, config1);
    }
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfigPowerCycle) {
    EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::OFF).isOk());
    EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON).isOk());

    std::vector<int32_t> configs;
    EXPECT_TRUE(mComposerClient->getDisplayConfigs(mPrimaryDisplay, &configs).isOk());
    for (auto config : configs) {
        EXPECT_TRUE(mComposerClient->setActiveConfig(mPrimaryDisplay, config).isOk());
        int32_t config1;
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &config1).isOk());
        EXPECT_EQ(config, config1);

        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::OFF).isOk());
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON).isOk());
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &config1).isOk());
        EXPECT_EQ(config, config1);
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeUnsupported) {
    std::vector<DisplayCapability> capabilities;
    auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    const bool isDozeSupported = std::find(capabilities.begin(), capabilities.end(),
                                           DisplayCapability::DOZE) != capabilities.end();
    const bool isSuspendSupported = std::find(capabilities.begin(), capabilities.end(),
                                              DisplayCapability::SUSPEND) != capabilities.end();
    if (!isDozeSupported) {
        error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::DOZE);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());

        error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::DOZE_SUSPEND);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
    }

    if (!isSuspendSupported) {
        error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON_SUSPEND);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());

        error = mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::DOZE_SUSPEND);
        EXPECT_FALSE(error.isOk());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, error.getServiceSpecificError());
    }
}

TEST_P(GraphicsComposerAidlTest, SetVsyncEnabled) {
    mComposerCallback->setVsyncAllowed(true);

    EXPECT_TRUE(mComposerClient->setVsyncEnabled(mPrimaryDisplay, true).isOk());
    usleep(60 * 1000);
    EXPECT_TRUE(mComposerClient->setVsyncEnabled(mPrimaryDisplay, false).isOk());

    mComposerCallback->setVsyncAllowed(false);
}

TEST_P(GraphicsComposerAidlTest, SetPowerMode) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    const bool isDozeSupported = std::find(capabilities.begin(), capabilities.end(),
                                           DisplayCapability::DOZE) != capabilities.end();
    const bool isSuspendSupported = std::find(capabilities.begin(), capabilities.end(),
                                              DisplayCapability::SUSPEND) != capabilities.end();

    std::vector<PowerMode> modes;
    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::ON);

    if (isSuspendSupported) {
        modes.push_back(PowerMode::ON_SUSPEND);
    }

    if (isDozeSupported) {
        modes.push_back(PowerMode::DOZE);
    }

    if (isSuspendSupported && isDozeSupported) {
        modes.push_back(PowerMode::DOZE_SUSPEND);
    }

    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeVariations) {
    std::vector<DisplayCapability> capabilities;
    const auto error = mComposerClient->getDisplayCapabilities(mPrimaryDisplay, &capabilities);
    ASSERT_TRUE(error.isOk());
    const bool isDozeSupported = std::find(capabilities.begin(), capabilities.end(),
                                           DisplayCapability::DOZE) != capabilities.end();
    const bool isSuspendSupported = std::find(capabilities.begin(), capabilities.end(),
                                              DisplayCapability::SUSPEND) != capabilities.end();

    std::vector<PowerMode> modes;

    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::ON);
    modes.push_back(PowerMode::OFF);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }
    modes.clear();

    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::OFF);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }
    modes.clear();

    modes.push_back(PowerMode::ON);
    modes.push_back(PowerMode::ON);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
    }
    modes.clear();

    if (isSuspendSupported) {
        modes.push_back(PowerMode::ON_SUSPEND);
        modes.push_back(PowerMode::ON_SUSPEND);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
        }
        modes.clear();
    }

    if (isDozeSupported) {
        modes.push_back(PowerMode::DOZE);
        modes.push_back(PowerMode::DOZE);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
        }
        modes.clear();
    }

    if (isSuspendSupported && isDozeSupported) {
        modes.push_back(PowerMode::DOZE_SUSPEND);
        modes.push_back(PowerMode::DOZE_SUSPEND);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, mode).isOk());
        }
        modes.clear();
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeBadDisplay) {
    const auto error = mComposerClient->setPowerMode(mInvalidDisplayId, PowerMode::ON);

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_DISPLAY, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeBadParameter) {
    const auto error = mComposerClient->setPowerMode(mPrimaryDisplay, static_cast<PowerMode>(-1));

    EXPECT_FALSE(error.isOk());
    ASSERT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrix) {
    std::vector<float> matrix;
    EXPECT_TRUE(
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::SRGB_LINEAR, &matrix)
                    .isOk());

    // the last row is known
    ASSERT_EQ(0.0f, matrix[12]);
    ASSERT_EQ(0.0f, matrix[13]);
    ASSERT_EQ(0.0f, matrix[14]);
    ASSERT_EQ(1.0f, matrix[15]);
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrixBadParameter) {
    std::vector<float> matrix;
    const auto error =
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::UNKNOWN, &matrix);

    EXPECT_FALSE(error.isOk());
    EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, error.getServiceSpecificError());
}

// Tests for Command.
class GraphicsComposerAidlCommandTest : public GraphicsComposerAidlTest {
  protected:
    void TearDown() override {
        const auto errors = mReader.takeErrors();
        ASSERT_TRUE(mReader.takeErrors().empty());

        std::vector<int64_t> layers;
        std::vector<Composition> types;
        mReader.takeChangedCompositionTypes(mPrimaryDisplay, &layers, &types);

        ASSERT_TRUE(layers.empty());
        ASSERT_TRUE(types.empty());

        ASSERT_NO_FATAL_FAILURE(GraphicsComposerAidlTest::TearDown());
    }

    void execute() {
        const auto& commands = mWriter.getPendingCommands();
        if (commands.empty()) {
            mWriter.reset();
            return;
        }

        std::vector<CommandResultPayload> results;
        const auto status = mComposerClient->executeCommands(commands, &results);
        ASSERT_TRUE(status.isOk()) << "executeCommands failed " << status.getDescription();

        mReader.parse(results);
        mWriter.reset();
    }

    static inline auto toTimePoint(nsecs_t time) {
        return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(time));
    }

    void setActiveConfig(VtsDisplay& display, int32_t config) {
        EXPECT_TRUE(mComposerClient->setActiveConfig(display.get(), config).isOk());
        int32_t displayWidth;
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(display.get(), config, DisplayAttribute::WIDTH,
                                                  &displayWidth)
                            .isOk());
        int32_t displayHeight;
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(display.get(), config, DisplayAttribute::HEIGHT,
                                                  &displayHeight)
                            .isOk());
        display.setDimensions(displayWidth, displayHeight);
    }

    void forEachTwoConfigs(int64_t display, std::function<void(int32_t, int32_t)> func) {
        std::vector<int32_t> displayConfigs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display, &displayConfigs).isOk());
        for (const int32_t config1 : displayConfigs) {
            for (const int32_t config2 : displayConfigs) {
                if (config1 != config2) {
                    func(config1, config2);
                }
            }
        }
    }

    void waitForVsyncPeriodChange(int64_t display, const VsyncPeriodChangeTimeline& timeline,
                                  int64_t desiredTimeNanos, int64_t oldPeriodNanos,
                                  int64_t newPeriodNanos) {
        const auto kChangeDeadline = toTimePoint(timeline.newVsyncAppliedTimeNanos) + 100ms;
        while (std::chrono::steady_clock::now() <= kChangeDeadline) {
            int32_t vsyncPeriodNanos;
            EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display, &vsyncPeriodNanos).isOk());
            if (systemTime() <= desiredTimeNanos) {
                EXPECT_EQ(vsyncPeriodNanos, oldPeriodNanos);
            } else if (vsyncPeriodNanos == newPeriodNanos) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(oldPeriodNanos));
        }
    }

    sp<GraphicBuffer> allocate() {
        return sp<GraphicBuffer>::make(
                static_cast<uint32_t>(mDisplayWidth), static_cast<uint32_t>(mDisplayHeight),
                ::android::PIXEL_FORMAT_RGBA_8888,
                /*layerCount*/ 1,
                (static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                 static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                 static_cast<uint64_t>(common::BufferUsage::COMPOSER_OVERLAY)),
                "VtsHalGraphicsComposer3_TargetTest");
    }

    void sendRefreshFrame(const VtsDisplay& display, const VsyncPeriodChangeTimeline* timeline) {
        if (timeline != nullptr) {
            // Refresh time should be before newVsyncAppliedTimeNanos
            EXPECT_LT(timeline->refreshTimeNanos, timeline->newVsyncAppliedTimeNanos);

            std::this_thread::sleep_until(toTimePoint(timeline->refreshTimeNanos));
        }

        EXPECT_TRUE(mComposerClient->setPowerMode(display.get(), PowerMode::ON).isOk());
        EXPECT_TRUE(
                mComposerClient
                        ->setColorMode(display.get(), ColorMode::NATIVE, RenderIntent::COLORIMETRIC)
                        .isOk());

        int64_t layer = 0;
        ASSERT_NO_FATAL_FAILURE(layer = createLayer(display));
        {
            const auto buffer = allocate();
            ASSERT_NE(nullptr, buffer);
            ASSERT_EQ(::android::OK, buffer->initCheck());
            ASSERT_NE(nullptr, buffer->handle);

            mWriter.setLayerCompositionType(display.get(), layer, Composition::DEVICE);
            mWriter.setLayerDisplayFrame(display.get(), layer, display.getFrameRect());
            mWriter.setLayerPlaneAlpha(display.get(), layer, 1);
            mWriter.setLayerSourceCrop(display.get(), layer, display.getCrop());
            mWriter.setLayerTransform(display.get(), layer, static_cast<Transform>(0));
            mWriter.setLayerVisibleRegion(display.get(), layer,
                                          std::vector<Rect>(1, display.getFrameRect()));
            mWriter.setLayerZOrder(display.get(), layer, 10);
            mWriter.setLayerBlendMode(display.get(), layer, BlendMode::NONE);
            mWriter.setLayerSurfaceDamage(display.get(), layer,
                                          std::vector<Rect>(1, display.getFrameRect()));
            mWriter.setLayerBuffer(display.get(), layer, 0, buffer->handle, -1);
            mWriter.setLayerDataspace(display.get(), layer, common::Dataspace::UNKNOWN);

            mWriter.validateDisplay(display.get(), ComposerClientWriter::kNoTimestamp);
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());

            mWriter.presentDisplay(display.get());
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());
        }

        {
            const auto buffer = allocate();
            ASSERT_NE(nullptr, buffer->handle);

            mWriter.setLayerBuffer(display.get(), layer, 0, buffer->handle, -1);
            mWriter.setLayerSurfaceDamage(display.get(), layer,
                                          std::vector<Rect>(1, {0, 0, 10, 10}));
            mWriter.validateDisplay(display.get(), ComposerClientWriter::kNoTimestamp);
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());

            mWriter.presentDisplay(display.get());
            execute();
        }

        ASSERT_NO_FATAL_FAILURE(destroyLayer(display, layer));
    }

    sp<::android::Fence> presentAndGetFence(
            std::optional<ClockMonotonicTimestamp> expectedPresentTime) {
        mWriter.validateDisplay(mPrimaryDisplay, expectedPresentTime);
        execute();
        EXPECT_TRUE(mReader.takeErrors().empty());

        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        EXPECT_TRUE(mReader.takeErrors().empty());

        int presentFence;
        mReader.takePresentFence(mPrimaryDisplay, &presentFence);
        EXPECT_NE(-1, presentFence);
        return sp<::android::Fence>::make(presentFence);
    }

    int32_t getVsyncPeriod() {
        int32_t activeConfig;
        EXPECT_TRUE(mComposerClient->getActiveConfig(mPrimaryDisplay, &activeConfig).isOk());

        int32_t vsyncPeriod;
        EXPECT_TRUE(mComposerClient
                            ->getDisplayAttribute(mPrimaryDisplay, activeConfig,
                                                  DisplayAttribute::VSYNC_PERIOD, &vsyncPeriod)
                            .isOk());
        return vsyncPeriod;
    }

    int64_t createOnScreenLayer() {
        const int64_t layer = createLayer(mDisplays[0]);
        mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::DEVICE);
        mWriter.setLayerDisplayFrame(mPrimaryDisplay, layer, {0, 0, mDisplayWidth, mDisplayHeight});
        mWriter.setLayerPlaneAlpha(mPrimaryDisplay, layer, 1);
        mWriter.setLayerSourceCrop(
                mPrimaryDisplay, layer,
                {0, 0, static_cast<float>(mDisplayWidth), static_cast<float>(mDisplayHeight)});
        mWriter.setLayerTransform(mPrimaryDisplay, layer, static_cast<Transform>(0));
        mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer,
                                      std::vector<Rect>(1, {0, 0, mDisplayWidth, mDisplayHeight}));
        mWriter.setLayerZOrder(mPrimaryDisplay, layer, 10);
        mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::NONE);
        mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer,
                                      std::vector<Rect>(1, {0, 0, mDisplayWidth, mDisplayHeight}));
        mWriter.setLayerDataspace(mPrimaryDisplay, layer, common::Dataspace::UNKNOWN);
        return layer;
    }

    void Test_setActiveConfigWithConstraints(const TestParameters& params) {
        for (VtsDisplay& display : mDisplays) {
            forEachTwoConfigs(display.get(), [&](int32_t config1, int32_t config2) {
                setActiveConfig(display, config1);
                sendRefreshFrame(display, nullptr);

                int32_t vsyncPeriod1;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config1,
                                                          DisplayAttribute::VSYNC_PERIOD,
                                                          &vsyncPeriod1)
                                    .isOk());
                int32_t configGroup1;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config1,
                                                          DisplayAttribute::CONFIG_GROUP,
                                                          &configGroup1)
                                    .isOk());
                int32_t vsyncPeriod2;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config2,
                                                          DisplayAttribute::VSYNC_PERIOD,
                                                          &vsyncPeriod2)
                                    .isOk());
                int32_t configGroup2;
                EXPECT_TRUE(mComposerClient
                                    ->getDisplayAttribute(display.get(), config2,
                                                          DisplayAttribute::CONFIG_GROUP,
                                                          &configGroup2)
                                    .isOk());

                if (vsyncPeriod1 == vsyncPeriod2) {
                    return;  // continue
                }

                // We don't allow delayed change when changing config groups
                if (params.delayForChange > 0 && configGroup1 != configGroup2) {
                    return;  // continue
                }

                VsyncPeriodChangeTimeline timeline;
                VsyncPeriodChangeConstraints constraints = {
                        .desiredTimeNanos = systemTime() + params.delayForChange,
                        .seamlessRequired = false};
                EXPECT_TRUE(setActiveConfigWithConstraints(display, config2, constraints, &timeline)
                                    .isOk());

                EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos >= constraints.desiredTimeNanos);
                // Refresh rate should change within a reasonable time
                constexpr std::chrono::nanoseconds kReasonableTimeForChange = 1s;  // 1 second
                EXPECT_TRUE(timeline.newVsyncAppliedTimeNanos - constraints.desiredTimeNanos <=
                            kReasonableTimeForChange.count());

                if (timeline.refreshRequired) {
                    if (params.refreshMiss) {
                        // Miss the refresh frame on purpose to make sure the implementation sends a
                        // callback
                        std::this_thread::sleep_until(toTimePoint(timeline.refreshTimeNanos) +
                                                      100ms);
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
                                             constraints.desiredTimeNanos, vsyncPeriod1,
                                             vsyncPeriod2);
                }

                int32_t vsyncPeriodNanos;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                EXPECT_EQ(vsyncPeriodNanos, vsyncPeriod2);
            });
        }
    }

    void Test_expectedPresentTime(std::optional<int> framesDelay) {
        ASSERT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON).isOk());

        const auto vsyncPeriod = getVsyncPeriod();

        const auto buffer1 = allocate();
        const auto buffer2 = allocate();
        ASSERT_NE(nullptr, buffer1);
        ASSERT_NE(nullptr, buffer2);

        const auto layer = createOnScreenLayer();
        mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, buffer1->handle, -1);
        const sp<::android::Fence> presentFence1 =
                presentAndGetFence(ComposerClientWriter::kNoTimestamp);
        presentFence1->waitForever(LOG_TAG);

        auto expectedPresentTime = presentFence1->getSignalTime() + vsyncPeriod;
        if (framesDelay.has_value()) {
            expectedPresentTime += *framesDelay * vsyncPeriod;
        }

        mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, buffer2->handle, -1);
        const auto setExpectedPresentTime = [&]() -> std::optional<ClockMonotonicTimestamp> {
            if (!framesDelay.has_value()) {
                return ComposerClientWriter::kNoTimestamp;
            } else if (*framesDelay == 0) {
                return ClockMonotonicTimestamp{0};
            }
            return ClockMonotonicTimestamp{expectedPresentTime};
        }();

        const sp<::android::Fence> presentFence2 = presentAndGetFence(setExpectedPresentTime);
        presentFence2->waitForever(LOG_TAG);

        const auto actualPresentTime = presentFence2->getSignalTime();
        const auto presentError = std::abs(expectedPresentTime - actualPresentTime);
        EXPECT_LE(presentError, vsyncPeriod / 2);

        ASSERT_TRUE(mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::OFF).isOk());
    }

    // clang-format off
    const std::array<float, 16> kIdentity = {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    }};
    // clang-format on

    ComposerClientWriter mWriter;
    ComposerClientReader mReader;
};

TEST_P(GraphicsComposerAidlCommandTest, SET_COLOR_TRANSFORM) {
    mWriter.setColorTransform(mPrimaryDisplay, kIdentity.data());
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerColorTransform) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());
    mWriter.setLayerColorTransform(mPrimaryDisplay, layer, kIdentity.data());
    execute();

    const auto errors = mReader.takeErrors();
    if (errors.size() == 1 && errors[0].errorCode == EX_UNSUPPORTED_OPERATION) {
        GTEST_SUCCEED() << "setLayerColorTransform is not supported";
        return;
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SET_CLIENT_TARGET) {
    EXPECT_TRUE(
            mComposerClient->setClientTargetSlotCount(mPrimaryDisplay, kBufferSlotCount).isOk());

    mWriter.setClientTarget(mPrimaryDisplay, 0, nullptr, -1, Dataspace::UNKNOWN,
                            std::vector<Rect>());

    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_OUTPUT_BUFFER) {
    int32_t virtualDisplayCount;
    EXPECT_TRUE(mComposerClient->getMaxVirtualDisplayCount(&virtualDisplayCount).isOk());
    if (virtualDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    VirtualDisplay display;
    EXPECT_TRUE(mComposerClient
                        ->createVirtualDisplay(64, 64, common::PixelFormat::IMPLEMENTATION_DEFINED,
                                               kBufferSlotCount, &display)
                        .isOk());

    const auto buffer = allocate();
    const auto handle = buffer->handle;
    mWriter.setOutputBuffer(display.display, 0, handle, -1);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, VALIDATE_DISPLAY) {
    mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, ACCEPT_DISPLAY_CHANGES) {
    mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
    mWriter.acceptDisplayChanges(mPrimaryDisplay);
    execute();
}

// TODO(b/208441745) fix the test failure
TEST_P(GraphicsComposerAidlCommandTest, PRESENT_DISPLAY) {
    mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
    mWriter.presentDisplay(mPrimaryDisplay);
    execute();
}

/**
 * Test IComposerClient::Command::PRESENT_DISPLAY
 *
 * Test that IComposerClient::Command::PRESENT_DISPLAY works without
 * additional call to validateDisplay when only the layer buffer handle and
 * surface damage have been set
 */
// TODO(b/208441745) fix the test failure
TEST_P(GraphicsComposerAidlCommandTest, PRESENT_DISPLAY_NO_LAYER_STATE_CHANGES) {
    std::vector<Capability> capabilities;
    EXPECT_TRUE(mComposer->getCapabilities(&capabilities).isOk());
    if (none_of(capabilities.begin(), capabilities.end(),
                [&](auto item) { return item == Capability::SKIP_VALIDATE; })) {
        GTEST_SUCCEED() << "Device does not have skip validate capability, skipping";
        return;
    }
    mComposerClient->setPowerMode(mPrimaryDisplay, PowerMode::ON);

    std::vector<RenderIntent> renderIntents;
    mComposerClient->getRenderIntents(mPrimaryDisplay, ColorMode::NATIVE, &renderIntents);
    for (auto intent : renderIntents) {
        mComposerClient->setColorMode(mPrimaryDisplay, ColorMode::NATIVE, intent);

        const auto buffer = allocate();
        const auto handle = buffer->handle;
        ASSERT_NE(nullptr, handle);

        Rect displayFrame{0, 0, mDisplayWidth, mDisplayHeight};

        int64_t layer;
        EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());
        mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::DEVICE);
        mWriter.setLayerDisplayFrame(mPrimaryDisplay, layer, displayFrame);
        mWriter.setLayerPlaneAlpha(mPrimaryDisplay, layer, 1);
        mWriter.setLayerSourceCrop(mPrimaryDisplay, layer,
                                   {0, 0, (float)mDisplayWidth, (float)mDisplayHeight});
        mWriter.setLayerTransform(mPrimaryDisplay, layer, static_cast<Transform>(0));
        mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer, std::vector<Rect>(1, displayFrame));
        mWriter.setLayerZOrder(mPrimaryDisplay, layer, 10);
        mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::NONE);
        mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>(1, displayFrame));
        mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, handle, -1);
        mWriter.setLayerDataspace(mPrimaryDisplay, layer, Dataspace::UNKNOWN);

        mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
        execute();
        std::vector<int64_t> layers;
        std::vector<Composition> types;
        mReader.takeChangedCompositionTypes(mPrimaryDisplay, &layers, &types);
        if (!layers.empty()) {
            GTEST_SUCCEED() << "Composition change requested, skipping test";
            return;
        }

        ASSERT_TRUE(mReader.takeErrors().empty());
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        const auto buffer2 = allocate();
        const auto handle2 = buffer2->handle;
        ASSERT_NE(nullptr, handle2);
        mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, handle2, -1);
        mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>(1, {0, 0, 10, 10}));
        mWriter.presentDisplay(mPrimaryDisplay);
        execute();
    }
}

// TODO(b/208441745) fix the test failure
TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_CURSOR_POSITION) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    const auto buffer = allocate();
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);
    Rect displayFrame{0, 0, mDisplayWidth, mDisplayHeight};

    mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, handle, -1);
    mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::CURSOR);
    mWriter.setLayerDisplayFrame(mPrimaryDisplay, layer, displayFrame);
    mWriter.setLayerPlaneAlpha(mPrimaryDisplay, layer, 1);
    mWriter.setLayerSourceCrop(mPrimaryDisplay, layer,
                               {0, 0, (float)mDisplayWidth, (float)mDisplayHeight});
    mWriter.setLayerTransform(mPrimaryDisplay, layer, static_cast<Transform>(0));
    mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer, std::vector<Rect>(1, displayFrame));
    mWriter.setLayerZOrder(mPrimaryDisplay, layer, 10);
    mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::NONE);
    mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>(1, displayFrame));
    mWriter.setLayerDataspace(mPrimaryDisplay, layer, Dataspace::UNKNOWN);
    mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);

    execute();
    std::vector<int64_t> layers;
    std::vector<Composition> types;
    mReader.takeChangedCompositionTypes(mPrimaryDisplay, &layers, &types);
    if (!layers.empty()) {
        GTEST_SUCCEED() << "Composition change requested, skipping test";
        return;
    }
    mWriter.presentDisplay(mPrimaryDisplay);
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerCursorPosition(mPrimaryDisplay, layer, 1, 1);
    execute();

    mWriter.setLayerCursorPosition(mPrimaryDisplay, layer, 0, 0);
    mWriter.validateDisplay(mPrimaryDisplay, ComposerClientWriter::kNoTimestamp);
    mWriter.presentDisplay(mPrimaryDisplay);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_BUFFER) {
    const auto buffer = allocate();
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);

    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());
    mWriter.setLayerBuffer(mPrimaryDisplay, layer, 0, handle, -1);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_SURFACE_DAMAGE) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    Rect empty{0, 0, 0, 0};
    Rect unit{0, 0, 1, 1};

    mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>(1, empty));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>(1, unit));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerSurfaceDamage(mPrimaryDisplay, layer, std::vector<Rect>());
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_BLEND_MODE) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::NONE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::PREMULTIPLIED);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerBlendMode(mPrimaryDisplay, layer, BlendMode::COVERAGE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_COLOR) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerColor(mPrimaryDisplay, layer,
                          Color{static_cast<int8_t>(0xff), static_cast<int8_t>(0xff),
                                static_cast<int8_t>(0xff), static_cast<int8_t>(0xff)});
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerColor(mPrimaryDisplay, layer, Color{0, 0, 0, 0});
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_COMPOSITION_TYPE) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::CLIENT);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::DEVICE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::SOLID_COLOR);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerCompositionType(mPrimaryDisplay, layer, Composition::CURSOR);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_DATASPACE) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerDataspace(mPrimaryDisplay, layer, Dataspace::UNKNOWN);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_DISPLAY_FRAME) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerDisplayFrame(mPrimaryDisplay, layer, Rect{0, 0, 1, 1});
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_PLANE_ALPHA) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerPlaneAlpha(mPrimaryDisplay, layer, 0.0f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerPlaneAlpha(mPrimaryDisplay, layer, 1.0f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_SIDEBAND_STREAM) {
    std::vector<Capability> capabilities;
    EXPECT_TRUE(mComposer->getCapabilities(&capabilities).isOk());
    if (none_of(capabilities.begin(), capabilities.end(),
                [&](auto& item) { return item == Capability::SIDEBAND_STREAM; })) {
        GTEST_SUCCEED() << "no sideband stream support";
        return;
    }

    const auto buffer = allocate();
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);

    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerSidebandStream(mPrimaryDisplay, layer, handle);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_SOURCE_CROP) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerSourceCrop(mPrimaryDisplay, layer, FRect{0.0f, 0.0f, 1.0f, 1.0f});
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_TRANSFORM) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, static_cast<Transform>(0));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, Transform::FLIP_H);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, Transform::FLIP_V);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, Transform::ROT_90);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, Transform::ROT_180);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer, Transform::ROT_270);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer,
                              static_cast<Transform>(static_cast<int>(Transform::FLIP_H) |
                                                     static_cast<int>(Transform::ROT_90)));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerTransform(mPrimaryDisplay, layer,
                              static_cast<Transform>(static_cast<int>(Transform::FLIP_V) |
                                                     static_cast<int>(Transform::ROT_90)));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_VISIBLE_REGION) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    Rect empty{0, 0, 0, 0};
    Rect unit{0, 0, 1, 1};

    mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer, std::vector<Rect>(1, empty));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer, std::vector<Rect>(1, unit));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerVisibleRegion(mPrimaryDisplay, layer, std::vector<Rect>());
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_Z_ORDER) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

    mWriter.setLayerZOrder(mPrimaryDisplay, layer, 10);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    mWriter.setLayerZOrder(mPrimaryDisplay, layer, 0);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SET_LAYER_PER_FRAME_METADATA) {
    int64_t layer;
    EXPECT_TRUE(mComposerClient->createLayer(mPrimaryDisplay, kBufferSlotCount, &layer).isOk());

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

    std::vector<PerFrameMetadata> aidlMetadata;
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X, 0.680f});
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y, 0.320f});
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X, 0.265f});
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y, 0.690f});
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X, 0.150f});
    aidlMetadata.push_back({PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y, 0.060f});
    aidlMetadata.push_back({PerFrameMetadataKey::WHITE_POINT_X, 0.3127f});
    aidlMetadata.push_back({PerFrameMetadataKey::WHITE_POINT_Y, 0.3290f});
    aidlMetadata.push_back({PerFrameMetadataKey::MAX_LUMINANCE, 100.0f});
    aidlMetadata.push_back({PerFrameMetadataKey::MIN_LUMINANCE, 0.1f});
    aidlMetadata.push_back({PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL, 78.0});
    aidlMetadata.push_back({PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL, 62.0});
    mWriter.setLayerPerFrameMetadata(mPrimaryDisplay, layer, aidlMetadata);
    execute();

    const auto errors = mReader.takeErrors();
    if (errors.size() == 1 && errors[0].errorCode == EX_UNSUPPORTED_OPERATION) {
        GTEST_SUCCEED() << "SetLayerPerFrameMetadata is not supported";
        EXPECT_TRUE(mComposerClient->destroyLayer(mPrimaryDisplay, layer).isOk());
        return;
    }

    EXPECT_TRUE(mComposerClient->destroyLayer(mPrimaryDisplay, layer).isOk());
}

TEST_P(GraphicsComposerAidlCommandTest, setActiveConfigWithConstraints) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = false});
}

TEST_P(GraphicsComposerAidlCommandTest, setActiveConfigWithConstraints_Delayed) {
    Test_setActiveConfigWithConstraints({.delayForChange = 300'000'000,  // 300ms
                                         .refreshMiss = false});
}

TEST_P(GraphicsComposerAidlCommandTest, setActiveConfigWithConstraints_MissRefresh) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = true});
}

TEST_P(GraphicsComposerAidlCommandTest, getDisplayVsyncPeriod) {
    for (VtsDisplay& display : mDisplays) {
        std::vector<int32_t> configs;
        EXPECT_TRUE(mComposerClient->getDisplayConfigs(display.get(), &configs).isOk());
        for (int32_t config : configs) {
            int32_t expectedVsyncPeriodNanos = -1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config,
                                                      DisplayAttribute::VSYNC_PERIOD,
                                                      &expectedVsyncPeriodNanos)
                                .isOk());

            VsyncPeriodChangeTimeline timeline;
            VsyncPeriodChangeConstraints constraints;

            constraints.desiredTimeNanos = systemTime();
            constraints.seamlessRequired = false;
            EXPECT_TRUE(mComposerClient
                                ->setActiveConfigWithConstraints(display.get(), config, constraints,
                                                                 &timeline)
                                .isOk());

            if (timeline.refreshRequired) {
                sendRefreshFrame(display, &timeline);
            }
            waitForVsyncPeriodChange(display.get(), timeline, constraints.desiredTimeNanos, 0,
                                     expectedVsyncPeriodNanos);

            int32_t vsyncPeriodNanos;
            int retryCount = 100;
            do {
                std::this_thread::sleep_for(10ms);
                vsyncPeriodNanos = 0;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                --retryCount;
            } while (vsyncPeriodNanos != expectedVsyncPeriodNanos && retryCount > 0);

            EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);

            // Make sure that the vsync period stays the same if the active config is not
            // changed.
            auto timeout = 1ms;
            for (int i = 0; i < 10; i++) {
                std::this_thread::sleep_for(timeout);
                timeout *= 2;
                vsyncPeriodNanos = 0;
                EXPECT_TRUE(mComposerClient->getDisplayVsyncPeriod(display.get(), &vsyncPeriodNanos)
                                    .isOk());
                EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);
            }
        }
    }
}

TEST_P(GraphicsComposerAidlCommandTest, setActiveConfigWithConstraints_SeamlessNotAllowed) {
    VsyncPeriodChangeTimeline timeline;
    VsyncPeriodChangeConstraints constraints;

    constraints.seamlessRequired = true;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        forEachTwoConfigs(display.get(), [&](int32_t config1, int32_t config2) {
            int32_t configGroup1;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config1,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup1)
                                .isOk());
            int32_t configGroup2;
            EXPECT_TRUE(mComposerClient
                                ->getDisplayAttribute(display.get(), config2,
                                                      DisplayAttribute::CONFIG_GROUP, &configGroup2)
                                .isOk());
            if (configGroup1 != configGroup2) {
                setActiveConfig(display, config1);
                sendRefreshFrame(display, nullptr);
                EXPECT_EQ(IComposerClient::EX_SEAMLESS_NOT_ALLOWED,
                          setActiveConfigWithConstraints(display, config2, constraints, &timeline)
                                  .getServiceSpecificError());
            }
        });
    }
}

TEST_P(GraphicsComposerAidlCommandTest, expectedPresentTime_NoTimestamp) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(std::nullopt));
}

TEST_P(GraphicsComposerAidlCommandTest, expectedPresentTime_0) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(0));
}

TEST_P(GraphicsComposerAidlCommandTest, expectedPresentTime_5) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(5));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlCommandTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlCommandTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);

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

    android::ProcessState::self()->setThreadPoolMaxThreadCount(4);

    // The binder threadpool we start will inherit sched policy and priority
    // of (this) creating thread. We want the binder thread pool to have
    // SCHED_FIFO policy and priority 1 (lowest RT priority)
    // Once the pool is created we reset this thread's priority back to
    // original.
    // This thread policy is based on what we do in the SurfaceFlinger while starting
    // the thread pool and we need to replicate that for the VTS tests.
    int newPriority = 0;
    int origPolicy = sched_getscheduler(0);
    struct sched_param origSchedParam;

    int errorInPriorityModification = sched_getparam(0, &origSchedParam);
    if (errorInPriorityModification == 0) {
        int policy = SCHED_FIFO;
        newPriority = sched_get_priority_min(policy);

        struct sched_param param;
        param.sched_priority = newPriority;

        errorInPriorityModification = sched_setscheduler(0, policy, &param);
    }

    // start the thread pool
    android::ProcessState::self()->startThreadPool();

    // Reset current thread's policy and priority
    if (errorInPriorityModification == 0) {
        errorInPriorityModification = sched_setscheduler(0, origPolicy, &origSchedParam);
    } else {
        ALOGE("Failed to set VtsHalGraphicsComposer3_TargetTest binder threadpool priority to "
              "SCHED_FIFO");
    }

    return RUN_ALL_TESTS();
}
