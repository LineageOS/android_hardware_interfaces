/**
 * Copyright (c) 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/properties.h>
#include <android/binder_process.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <algorithm>
#include <iterator>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include "GraphicsComposerCallback.h"
#include "VtsComposerClient.h"

#undef LOG_TAG
#define LOG_TAG "VtsHalGraphicsComposer3_TargetTest"

namespace aidl::android::hardware::graphics::composer3::vts {

using namespace std::chrono_literals;

using ::android::GraphicBuffer;
using ::android::sp;

class GraphicsComposerAidlTest : public ::testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        mComposerClient = std::make_unique<VtsComposerClient>(GetParam());
        ASSERT_TRUE(mComposerClient->createClient().isOk());

        const auto& [status, displays] = mComposerClient->getDisplays();
        ASSERT_TRUE(status.isOk());
        mDisplays = displays;

        // explicitly disable vsync
        for (const auto& display : mDisplays) {
            EXPECT_TRUE(mComposerClient->setVsync(display.getDisplayId(), false).isOk());
        }
        mComposerClient->setVsyncAllowed(false);
    }

    void TearDown() override {
        ASSERT_TRUE(mComposerClient->tearDown());
        mComposerClient.reset();
    }

    void assertServiceSpecificError(const ScopedAStatus& status, int32_t serviceSpecificError) {
        ASSERT_EQ(status.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(status.getServiceSpecificError(), serviceSpecificError);
    }

    void Test_setContentTypeForDisplay(int64_t display,
                                       const std::vector<ContentType>& supportedContentTypes,
                                       ContentType contentType, const char* contentTypeStr) {
        const bool contentTypeSupport =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          contentType) != supportedContentTypes.end();

        if (!contentTypeSupport) {
            const auto& status = mComposerClient->setContentType(display, contentType);
            EXPECT_FALSE(status.isOk());
            EXPECT_NO_FATAL_FAILURE(
                    assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
            GTEST_SUCCEED() << contentTypeStr << " content type is not supported on display "
                            << std::to_string(display) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setContentType(display, contentType).isOk());
        EXPECT_TRUE(mComposerClient->setContentType(display, ContentType::NONE).isOk());
    }

    void Test_setContentType(ContentType contentType, const char* contentTypeStr) {
        for (const auto& display : mDisplays) {
            const auto& [status, supportedContentTypes] =
                    mComposerClient->getSupportedContentTypes(display.getDisplayId());
            EXPECT_TRUE(status.isOk());
            Test_setContentTypeForDisplay(display.getDisplayId(), supportedContentTypes,
                                          contentType, contentTypeStr);
        }
    }

    bool hasCapability(Capability capability) {
        const auto& [status, capabilities] = mComposerClient->getCapabilities();
        EXPECT_TRUE(status.isOk());
        return std::any_of(
                capabilities.begin(), capabilities.end(),
                [&](const Capability& activeCapability) { return activeCapability == capability; });
    }

    int getInterfaceVersion() {
        const auto& [versionStatus, version] = mComposerClient->getInterfaceVersion();
        EXPECT_TRUE(versionStatus.isOk());
        return version;
    }

    const VtsDisplay& getPrimaryDisplay() const { return mDisplays[0]; }

    int64_t getPrimaryDisplayId() const { return getPrimaryDisplay().getDisplayId(); }

    int64_t getInvalidDisplayId() const { return mComposerClient->getInvalidDisplayId(); }

    VtsDisplay& getEditablePrimaryDisplay() { return mDisplays[0]; }

    struct TestParameters {
        nsecs_t delayForChange;
        bool refreshMiss;
    };

    std::unique_ptr<VtsComposerClient> mComposerClient;
    std::vector<VtsDisplay> mDisplays;
    // use the slot count usually set by SF
    static constexpr uint32_t kBufferSlotCount = 64;
};

TEST_P(GraphicsComposerAidlTest, GetDisplayCapabilities_BadDisplay) {
    const auto& [status, _] = mComposerClient->getDisplayCapabilities(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayCapabilities) {
    for (const auto& display : mDisplays) {
        const auto& [status, capabilities] =
                mComposerClient->getDisplayCapabilities(display.getDisplayId());

        EXPECT_TRUE(status.isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, DumpDebugInfo) {
    ASSERT_TRUE(mComposerClient->dumpDebugInfo().isOk());
}

TEST_P(GraphicsComposerAidlTest, CreateClientSingleton) {
    std::shared_ptr<IComposerClient> composerClient;
    const auto& status = mComposerClient->createClient();

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_NO_RESOURCES));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayIdentificationData) {
    const auto& [status0, displayIdentification0] =
            mComposerClient->getDisplayIdentificationData(getPrimaryDisplayId());
    if (!status0.isOk() && status0.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status0.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "Display identification data not supported, skipping test";
        return;
    }
    ASSERT_TRUE(status0.isOk()) << "failed to get display identification data";
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

    const auto& [status1, displayIdentification1] =
            mComposerClient->getDisplayIdentificationData(getPrimaryDisplayId());
    ASSERT_TRUE(status1.isOk());

    ASSERT_EQ(displayIdentification0.port, displayIdentification1.port) << "ports are not stable";
    ASSERT_TRUE(displayIdentification0.data.size() == displayIdentification1.data.size() &&
                std::equal(displayIdentification0.data.begin(), displayIdentification0.data.end(),
                           displayIdentification1.data.begin()))
            << "data is not stable";
}

TEST_P(GraphicsComposerAidlTest, GetHdrCapabilities) {
    const auto& [status, hdrCapabilities] =
            mComposerClient->getHdrCapabilities(getPrimaryDisplayId());

    ASSERT_TRUE(status.isOk());
    EXPECT_TRUE(hdrCapabilities.maxLuminance >= hdrCapabilities.minLuminance);
}

TEST_P(GraphicsComposerAidlTest, GetPerFrameMetadataKeys) {
    const auto& [status, keys] = mComposerClient->getPerFrameMetadataKeys(getPrimaryDisplayId());
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "getPerFrameMetadataKeys is not supported";
        return;
    }

    ASSERT_TRUE(status.isOk());
    EXPECT_TRUE(keys.size() >= 0);
}

TEST_P(GraphicsComposerAidlTest, GetReadbackBufferAttributes) {
    const auto& [status, _] = mComposerClient->getReadbackBufferAttributes(getPrimaryDisplayId());
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "getReadbackBufferAttributes is not supported";
        return;
    }

    ASSERT_TRUE(status.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntents) {
    const auto& [status, modes] = mComposerClient->getColorModes(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());

    for (auto mode : modes) {
        const auto& [intentStatus, intents] =
                mComposerClient->getRenderIntents(getPrimaryDisplayId(), mode);
        EXPECT_TRUE(intentStatus.isOk());
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

        const auto iter = std::find(intents.cbegin(), intents.cend(), requiredIntent);
        EXPECT_NE(intents.cend(), iter);
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntents_BadDisplay) {
    const auto& [status, modes] = mComposerClient->getColorModes(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    for (auto mode : modes) {
        const auto& [intentStatus, _] =
                mComposerClient->getRenderIntents(getInvalidDisplayId(), mode);

        EXPECT_FALSE(intentStatus.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(intentStatus, IComposerClient::EX_BAD_DISPLAY));
    }
}

TEST_P(GraphicsComposerAidlTest, GetRenderIntents_BadParameter) {
    const auto& [status, _] =
            mComposerClient->getRenderIntents(getPrimaryDisplayId(), static_cast<ColorMode>(-1));

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

TEST_P(GraphicsComposerAidlTest, GetColorModes) {
    const auto& [status, colorModes] = mComposerClient->getColorModes(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    const auto native = std::find(colorModes.cbegin(), colorModes.cend(), ColorMode::NATIVE);
    EXPECT_NE(colorModes.cend(), native);
}

TEST_P(GraphicsComposerAidlTest, GetColorMode_BadDisplay) {
    const auto& [status, _] = mComposerClient->getColorModes(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetColorMode) {
    const auto& [status, colorModes] = mComposerClient->getColorModes(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());

    for (auto mode : colorModes) {
        const auto& [intentStatus, intents] =
                mComposerClient->getRenderIntents(getPrimaryDisplayId(), mode);
        EXPECT_TRUE(intentStatus.isOk()) << "failed to get render intents";

        for (auto intent : intents) {
            const auto modeStatus =
                    mComposerClient->setColorMode(getPrimaryDisplayId(), mode, intent);
            EXPECT_TRUE(modeStatus.isOk() ||
                        (modeStatus.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                         IComposerClient::EX_UNSUPPORTED == modeStatus.getServiceSpecificError()))
                    << "failed to set color mode";
        }
    }

    const auto modeStatus = mComposerClient->setColorMode(getPrimaryDisplayId(), ColorMode::NATIVE,
                                                          RenderIntent::COLORIMETRIC);
    EXPECT_TRUE(modeStatus.isOk() ||
                (modeStatus.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                 IComposerClient::EX_UNSUPPORTED == modeStatus.getServiceSpecificError()))
            << "failed to set color mode";
}

TEST_P(GraphicsComposerAidlTest, SetColorMode_BadDisplay) {
    const auto& [status, colorModes] = mComposerClient->getColorModes(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    for (auto mode : colorModes) {
        const auto& [intentStatus, intents] =
                mComposerClient->getRenderIntents(getPrimaryDisplayId(), mode);
        ASSERT_TRUE(intentStatus.isOk()) << "failed to get render intents";

        for (auto intent : intents) {
            auto const modeStatus =
                    mComposerClient->setColorMode(getInvalidDisplayId(), mode, intent);

            EXPECT_FALSE(modeStatus.isOk());
            EXPECT_NO_FATAL_FAILURE(
                    assertServiceSpecificError(modeStatus, IComposerClient::EX_BAD_DISPLAY));
        }
    }
}

TEST_P(GraphicsComposerAidlTest, SetColorMode_BadParameter) {
    auto status = mComposerClient->setColorMode(getPrimaryDisplayId(), static_cast<ColorMode>(-1),
                                                RenderIntent::COLORIMETRIC);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));

    status = mComposerClient->setColorMode(getPrimaryDisplayId(), ColorMode::NATIVE,
                                           static_cast<RenderIntent>(-1));

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSamplingAttributes) {
    int constexpr kInvalid = -1;
    const auto& [status, format] =
            mComposerClient->getDisplayedContentSamplingAttributes(getPrimaryDisplayId());

    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    ASSERT_TRUE(status.isOk());
    EXPECT_NE(kInvalid, static_cast<int>(format.format));
    EXPECT_NE(kInvalid, static_cast<int>(format.dataspace));
    EXPECT_NE(kInvalid, static_cast<int>(format.componentMask));
};

TEST_P(GraphicsComposerAidlTest, SetDisplayedContentSamplingEnabled) {
    int constexpr kMaxFrames = 10;
    FormatColorComponent enableAllComponents = FormatColorComponent::FORMAT_COMPONENT_0;
    auto status = mComposerClient->setDisplayedContentSamplingEnabled(
            getPrimaryDisplayId(), /*isEnabled*/ true, enableAllComponents, kMaxFrames);
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }
    EXPECT_TRUE(status.isOk());

    status = mComposerClient->setDisplayedContentSamplingEnabled(
            getPrimaryDisplayId(), /*isEnabled*/ false, enableAllComponents, kMaxFrames);
    EXPECT_TRUE(status.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayedContentSample) {
    const auto& [status, displayContentSamplingAttributes] =
            mComposerClient->getDisplayedContentSamplingAttributes(getPrimaryDisplayId());
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Sampling attributes aren't supported on this device, test skipped";
        return;
    }

    int64_t constexpr kMaxFrames = 10;
    int64_t constexpr kTimestamp = 0;
    const auto& [sampleStatus, displayContentSample] = mComposerClient->getDisplayedContentSample(
            getPrimaryDisplayId(), kMaxFrames, kTimestamp);
    if (!sampleStatus.isOk() && sampleStatus.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        sampleStatus.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        SUCCEED() << "Device does not support optional extension. Test skipped";
        return;
    }

    EXPECT_TRUE(sampleStatus.isOk());
    const std::vector<std::vector<int64_t>> histogram = {
            displayContentSample.sampleComponent0, displayContentSample.sampleComponent1,
            displayContentSample.sampleComponent2, displayContentSample.sampleComponent3};

    for (size_t i = 0; i < histogram.size(); i++) {
        const bool shouldHaveHistogram =
                static_cast<int>(displayContentSamplingAttributes.componentMask) & (1 << i);
        EXPECT_EQ(shouldHaveHistogram, !histogram[i].empty());
    }
}

TEST_P(GraphicsComposerAidlTest, GetDisplayConnectionType) {
    const auto& [status, type] = mComposerClient->getDisplayConnectionType(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));

    for (const auto& display : mDisplays) {
        const auto& [connectionTypeStatus, _] =
                mComposerClient->getDisplayConnectionType(display.getDisplayId());
        EXPECT_TRUE(connectionTypeStatus.isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, GetDisplayAttribute) {
    for (const auto& display : mDisplays) {
        const auto& [status, configs] = mComposerClient->getDisplayConfigs(display.getDisplayId());
        EXPECT_TRUE(status.isOk());

        for (const auto& config : configs) {
            const std::array<DisplayAttribute, 4> requiredAttributes = {{
                    DisplayAttribute::WIDTH,
                    DisplayAttribute::HEIGHT,
                    DisplayAttribute::VSYNC_PERIOD,
                    DisplayAttribute::CONFIG_GROUP,
            }};
            for (const auto& attribute : requiredAttributes) {
                const auto& [attribStatus, value] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), config, attribute);
                EXPECT_TRUE(attribStatus.isOk());
                EXPECT_NE(-1, value);
            }

            const std::array<DisplayAttribute, 2> optionalAttributes = {{
                    DisplayAttribute::DPI_X,
                    DisplayAttribute::DPI_Y,
            }};
            for (const auto& attribute : optionalAttributes) {
                const auto& [attribStatus, value] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), config, attribute);
                EXPECT_TRUE(attribStatus.isOk() ||
                            (attribStatus.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                             IComposerClient::EX_UNSUPPORTED ==
                                     attribStatus.getServiceSpecificError()));
            }
        }
    }
}

TEST_P(GraphicsComposerAidlTest, CheckConfigsAreValid) {
    for (const auto& display : mDisplays) {
        const auto& [status, configs] = mComposerClient->getDisplayConfigs(display.getDisplayId());
        EXPECT_TRUE(status.isOk());

        EXPECT_FALSE(std::any_of(configs.begin(), configs.end(), [](auto config) {
            return config == IComposerClient::INVALID_CONFIGURATION;
        }));
    }
}

TEST_P(GraphicsComposerAidlTest, GetDisplayVsyncPeriod_BadDisplay) {
    const auto& [status, vsyncPeriodNanos] =
            mComposerClient->getDisplayVsyncPeriod(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfigWithConstraints_BadDisplay) {
    VsyncPeriodChangeConstraints constraints;
    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();
    auto invalidDisplay = VtsDisplay(getInvalidDisplayId());

    const auto& [status, timeline] = mComposerClient->setActiveConfigWithConstraints(
            &invalidDisplay, /*config*/ 0, constraints);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfigWithConstraints_BadConfig) {
    VsyncPeriodChangeConstraints constraints;
    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        int32_t constexpr kInvalidConfigId = IComposerClient::INVALID_CONFIGURATION;
        const auto& [status, _] = mComposerClient->setActiveConfigWithConstraints(
                &display, kInvalidConfigId, constraints);

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_CONFIG));
    }
}

TEST_P(GraphicsComposerAidlTest, SetBootDisplayConfig_BadDisplay) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    const auto& status = mComposerClient->setBootDisplayConfig(getInvalidDisplayId(), /*config*/ 0);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetBootDisplayConfig_BadConfig) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    for (VtsDisplay& display : mDisplays) {
        int32_t constexpr kInvalidConfigId = IComposerClient::INVALID_CONFIGURATION;
        const auto& status =
                mComposerClient->setBootDisplayConfig(display.getDisplayId(), kInvalidConfigId);

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_CONFIG));
    }
}

TEST_P(GraphicsComposerAidlTest, SetBootDisplayConfig) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    const auto& [status, configs] = mComposerClient->getDisplayConfigs(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());
    for (const auto& config : configs) {
        EXPECT_TRUE(mComposerClient->setBootDisplayConfig(getPrimaryDisplayId(), config).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, ClearBootDisplayConfig_BadDisplay) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    const auto& status = mComposerClient->clearBootDisplayConfig(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, ClearBootDisplayConfig) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    EXPECT_TRUE(mComposerClient->clearBootDisplayConfig(getPrimaryDisplayId()).isOk());
}

TEST_P(GraphicsComposerAidlTest, GetPreferredBootDisplayConfig_BadDisplay) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    const auto& [status, _] = mComposerClient->getPreferredBootDisplayConfig(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetPreferredBootDisplayConfig) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        GTEST_SUCCEED() << "Boot Display Config not supported";
        return;
    }
    const auto& [status, preferredDisplayConfig] =
            mComposerClient->getPreferredBootDisplayConfig(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());

    const auto& [configStatus, configs] = mComposerClient->getDisplayConfigs(getPrimaryDisplayId());

    EXPECT_TRUE(configStatus.isOk());
    EXPECT_NE(configs.end(), std::find(configs.begin(), configs.end(), preferredDisplayConfig));
}

TEST_P(GraphicsComposerAidlTest, BootDisplayConfig_Unsupported) {
    if (!hasCapability(Capability::BOOT_DISPLAY_CONFIG)) {
        const auto& [configStatus, config] =
                mComposerClient->getActiveConfig(getPrimaryDisplayId());
        EXPECT_TRUE(configStatus.isOk());

        auto status = mComposerClient->setBootDisplayConfig(getPrimaryDisplayId(), config);
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));

        status = mComposerClient->getPreferredBootDisplayConfig(getPrimaryDisplayId()).first;
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));

        status = mComposerClient->clearBootDisplayConfig(getPrimaryDisplayId());
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
    }
}

TEST_P(GraphicsComposerAidlTest, GetHdrConversionCapabilities) {
    if (!hasCapability(Capability::HDR_OUTPUT_CONVERSION_CONFIG)) {
        GTEST_SUCCEED() << "HDR output conversion not supported";
        return;
    }
    const auto& [status, conversionCapabilities] = mComposerClient->getHdrConversionCapabilities();
    EXPECT_TRUE(status.isOk());
}

TEST_P(GraphicsComposerAidlTest, SetHdrConversionStrategy_Passthrough) {
    if (!hasCapability(Capability::HDR_OUTPUT_CONVERSION_CONFIG)) {
        GTEST_SUCCEED() << "HDR output conversion not supported";
        return;
    }
    common::HdrConversionStrategy hdrConversionStrategy;
    hdrConversionStrategy.set<common::HdrConversionStrategy::Tag::passthrough>(true);
    const auto& [status, preferredHdrOutputType] =
            mComposerClient->setHdrConversionStrategy(hdrConversionStrategy);
    EXPECT_TRUE(status.isOk());
    EXPECT_EQ(common::Hdr::INVALID, preferredHdrOutputType);
}

TEST_P(GraphicsComposerAidlTest, SetHdrConversionStrategy_Force) {
    if (!hasCapability(Capability::HDR_OUTPUT_CONVERSION_CONFIG)) {
        GTEST_SUCCEED() << "HDR output conversion not supported";
        return;
    }
    const auto& [status, conversionCapabilities] = mComposerClient->getHdrConversionCapabilities();
    const auto& [status2, hdrCapabilities] =
            mComposerClient->getHdrCapabilities(getPrimaryDisplayId());
    const auto& hdrTypes = hdrCapabilities.types;
    for (auto conversionCapability : conversionCapabilities) {
        if (conversionCapability.outputType != common::Hdr::INVALID) {
            if (std::find(hdrTypes.begin(), hdrTypes.end(), conversionCapability.outputType) ==
                hdrTypes.end()) {
                continue;
            }
            common::HdrConversionStrategy hdrConversionStrategy;
            hdrConversionStrategy.set<common::HdrConversionStrategy::Tag::forceHdrConversion>(
                    conversionCapability.outputType);
            const auto& [statusSet, preferredHdrOutputType] =
                    mComposerClient->setHdrConversionStrategy(hdrConversionStrategy);
            EXPECT_TRUE(statusSet.isOk());
            EXPECT_EQ(common::Hdr::INVALID, preferredHdrOutputType);
        }
    }
}

TEST_P(GraphicsComposerAidlTest, SetHdrConversionStrategy_Auto) {
    if (!hasCapability(Capability::HDR_OUTPUT_CONVERSION_CONFIG)) {
        GTEST_SUCCEED() << "HDR output conversion not supported";
        return;
    }
    const auto& [status, conversionCapabilities] = mComposerClient->getHdrConversionCapabilities();
    const auto& [status2, hdrCapabilities] =
            mComposerClient->getHdrCapabilities(getPrimaryDisplayId());
    if (hdrCapabilities.types.size() <= 0) {
        return;
    }
    std::vector<aidl::android::hardware::graphics::common::Hdr> autoHdrTypes;
    for (auto conversionCapability : conversionCapabilities) {
        if (conversionCapability.outputType != common::Hdr::INVALID) {
            autoHdrTypes.push_back(conversionCapability.outputType);
        }
    }
    common::HdrConversionStrategy hdrConversionStrategy;
    hdrConversionStrategy.set<common::HdrConversionStrategy::Tag::autoAllowedHdrTypes>(
            autoHdrTypes);
    const auto& [statusSet, preferredHdrOutputType] =
            mComposerClient->setHdrConversionStrategy(hdrConversionStrategy);
    EXPECT_TRUE(statusSet.isOk());
    EXPECT_NE(common::Hdr::INVALID, preferredHdrOutputType);
}

TEST_P(GraphicsComposerAidlTest, SetAutoLowLatencyMode_BadDisplay) {
    auto status = mComposerClient->setAutoLowLatencyMode(getInvalidDisplayId(), /*isEnabled*/ true);
    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));

    status = mComposerClient->setAutoLowLatencyMode(getInvalidDisplayId(), /*isEnabled*/ false);
    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetAutoLowLatencyMode) {
    for (const auto& display : mDisplays) {
        const auto& [status, capabilities] =
                mComposerClient->getDisplayCapabilities(display.getDisplayId());
        ASSERT_TRUE(status.isOk());

        const bool allmSupport =
                std::find(capabilities.begin(), capabilities.end(),
                          DisplayCapability::AUTO_LOW_LATENCY_MODE) != capabilities.end();

        if (!allmSupport) {
            const auto& statusIsOn = mComposerClient->setAutoLowLatencyMode(display.getDisplayId(),
                                                                            /*isEnabled*/ true);
            EXPECT_FALSE(statusIsOn.isOk());
            EXPECT_NO_FATAL_FAILURE(
                    assertServiceSpecificError(statusIsOn, IComposerClient::EX_UNSUPPORTED));
            const auto& statusIsOff = mComposerClient->setAutoLowLatencyMode(display.getDisplayId(),
                                                                             /*isEnabled*/ false);
            EXPECT_FALSE(statusIsOff.isOk());
            EXPECT_NO_FATAL_FAILURE(
                    assertServiceSpecificError(statusIsOff, IComposerClient::EX_UNSUPPORTED));
            GTEST_SUCCEED() << "Auto Low Latency Mode is not supported on display "
                            << std::to_string(display.getDisplayId()) << ", skipping test";
            return;
        }

        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.getDisplayId(), true).isOk());
        EXPECT_TRUE(mComposerClient->setAutoLowLatencyMode(display.getDisplayId(), false).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, GetSupportedContentTypes_BadDisplay) {
    const auto& [status, _] = mComposerClient->getSupportedContentTypes(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetSupportedContentTypes) {
    for (const auto& display : mDisplays) {
        const auto& [status, supportedContentTypes] =
                mComposerClient->getSupportedContentTypes(display.getDisplayId());
        ASSERT_TRUE(status.isOk());

        const bool noneSupported =
                std::find(supportedContentTypes.begin(), supportedContentTypes.end(),
                          ContentType::NONE) != supportedContentTypes.end();

        EXPECT_FALSE(noneSupported);
    }
}

TEST_P(GraphicsComposerAidlTest, SetContentTypeNoneAlwaysAccepted) {
    for (const auto& display : mDisplays) {
        EXPECT_TRUE(
                mComposerClient->setContentType(display.getDisplayId(), ContentType::NONE).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, SetContentType_BadDisplay) {
    constexpr ContentType types[] = {ContentType::NONE, ContentType::GRAPHICS, ContentType::PHOTO,
                                     ContentType::CINEMA, ContentType::GAME};
    for (const auto& type : types) {
        const auto& status = mComposerClient->setContentType(getInvalidDisplayId(), type);

        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
    }
}

TEST_P(GraphicsComposerAidlTest, SetGraphicsContentType) {
    Test_setContentType(ContentType::GRAPHICS, "GRAPHICS");
}

TEST_P(GraphicsComposerAidlTest, SetPhotoContentType) {
    Test_setContentType(ContentType::PHOTO, "PHOTO");
}

TEST_P(GraphicsComposerAidlTest, SetCinemaContentType) {
    Test_setContentType(ContentType::CINEMA, "CINEMA");
}

TEST_P(GraphicsComposerAidlTest, SetGameContentType) {
    Test_setContentType(ContentType::GAME, "GAME");
}

TEST_P(GraphicsComposerAidlTest, CreateVirtualDisplay) {
    const auto& [status, maxVirtualDisplayCount] = mComposerClient->getMaxVirtualDisplayCount();
    EXPECT_TRUE(status.isOk());

    if (maxVirtualDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    const auto& [virtualDisplayStatus, virtualDisplay] = mComposerClient->createVirtualDisplay(
            /*width*/ 64, /*height*/ 64, common::PixelFormat::IMPLEMENTATION_DEFINED,
            kBufferSlotCount);

    ASSERT_TRUE(virtualDisplayStatus.isOk());
    EXPECT_TRUE(mComposerClient->destroyVirtualDisplay(virtualDisplay.display).isOk());
}

TEST_P(GraphicsComposerAidlTest, DestroyVirtualDisplay_BadDisplay) {
    const auto& [status, maxDisplayCount] = mComposerClient->getMaxVirtualDisplayCount();
    EXPECT_TRUE(status.isOk());

    if (maxDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    const auto& destroyStatus = mComposerClient->destroyVirtualDisplay(getInvalidDisplayId());

    EXPECT_FALSE(destroyStatus.isOk());
    EXPECT_NO_FATAL_FAILURE(
            assertServiceSpecificError(destroyStatus, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, CreateLayer) {
    const auto& [status, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(mComposerClient->destroyLayer(getPrimaryDisplayId(), layer).isOk());
}

TEST_P(GraphicsComposerAidlTest, CreateLayer_BadDisplay) {
    const auto& [status, _] = mComposerClient->createLayer(getInvalidDisplayId(), kBufferSlotCount);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, DestroyLayer_BadDisplay) {
    const auto& [status, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(status.isOk());

    const auto& destroyStatus = mComposerClient->destroyLayer(getInvalidDisplayId(), layer);

    EXPECT_FALSE(destroyStatus.isOk());
    EXPECT_NO_FATAL_FAILURE(
            assertServiceSpecificError(destroyStatus, IComposerClient::EX_BAD_DISPLAY));
    ASSERT_TRUE(mComposerClient->destroyLayer(getPrimaryDisplayId(), layer).isOk());
}

TEST_P(GraphicsComposerAidlTest, DestroyLayer_BadLayerError) {
    // We haven't created any layers yet, so any id should be invalid
    const auto& status = mComposerClient->destroyLayer(getPrimaryDisplayId(), /*layer*/ 1);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_LAYER));
}

TEST_P(GraphicsComposerAidlTest, GetActiveConfig_BadDisplay) {
    const auto& [status, _] = mComposerClient->getActiveConfig(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayConfig) {
    const auto& [status, _] = mComposerClient->getDisplayConfigs(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayConfig_BadDisplay) {
    const auto& [status, _] = mComposerClient->getDisplayConfigs(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayName) {
    const auto& [status, _] = mComposerClient->getDisplayName(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());
}

TEST_P(GraphicsComposerAidlTest, GetDisplayPhysicalOrientation_BadDisplay) {
    const auto& [status, _] = mComposerClient->getDisplayPhysicalOrientation(getInvalidDisplayId());

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, GetDisplayPhysicalOrientation) {
    const auto allowedDisplayOrientations = std::array<Transform, 4>{
            Transform::NONE,
            Transform::ROT_90,
            Transform::ROT_180,
            Transform::ROT_270,
    };

    const auto& [status, displayOrientation] =
            mComposerClient->getDisplayPhysicalOrientation(getPrimaryDisplayId());

    EXPECT_TRUE(status.isOk());
    EXPECT_NE(std::find(allowedDisplayOrientations.begin(), allowedDisplayOrientations.end(),
                        displayOrientation),
              allowedDisplayOrientations.end());
}

TEST_P(GraphicsComposerAidlTest, SetClientTargetSlotCount) {
    EXPECT_TRUE(mComposerClient->setClientTargetSlotCount(getPrimaryDisplayId(), kBufferSlotCount)
                        .isOk());
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfig) {
    const auto& [status, configs] = mComposerClient->getDisplayConfigs(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());

    for (const auto& config : configs) {
        auto display = getEditablePrimaryDisplay();
        EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config).isOk());
        const auto& [configStatus, config1] =
                mComposerClient->getActiveConfig(getPrimaryDisplayId());
        EXPECT_TRUE(configStatus.isOk());
        EXPECT_EQ(config, config1);
    }
}

TEST_P(GraphicsComposerAidlTest, SetActiveConfigPowerCycle) {
    EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::OFF).isOk());
    EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());

    const auto& [status, configs] = mComposerClient->getDisplayConfigs(getPrimaryDisplayId());
    EXPECT_TRUE(status.isOk());

    for (const auto& config : configs) {
        auto display = getEditablePrimaryDisplay();
        EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config).isOk());
        const auto& [config1Status, config1] =
                mComposerClient->getActiveConfig(getPrimaryDisplayId());
        EXPECT_TRUE(config1Status.isOk());
        EXPECT_EQ(config, config1);

        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::OFF).isOk());
        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());
        const auto& [config2Status, config2] =
                mComposerClient->getActiveConfig(getPrimaryDisplayId());
        EXPECT_TRUE(config2Status.isOk());
        EXPECT_EQ(config, config2);
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeUnsupported) {
    const auto& [status, capabilities] =
            mComposerClient->getDisplayCapabilities(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    const bool isDozeSupported = std::find(capabilities.begin(), capabilities.end(),
                                           DisplayCapability::DOZE) != capabilities.end();
    const bool isSuspendSupported = std::find(capabilities.begin(), capabilities.end(),
                                              DisplayCapability::SUSPEND) != capabilities.end();

    if (!isDozeSupported) {
        const auto& powerModeDozeStatus =
                mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::DOZE);
        EXPECT_FALSE(powerModeDozeStatus.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(powerModeDozeStatus, IComposerClient::EX_UNSUPPORTED));

        const auto& powerModeDozeSuspendStatus =
                mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::DOZE_SUSPEND);
        EXPECT_FALSE(powerModeDozeSuspendStatus.isOk());
        EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(powerModeDozeSuspendStatus,
                                                           IComposerClient::EX_UNSUPPORTED));
    }

    if (!isSuspendSupported) {
        const auto& powerModeSuspendStatus =
                mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON_SUSPEND);
        EXPECT_FALSE(powerModeSuspendStatus.isOk());
        EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(powerModeSuspendStatus,
                                                           IComposerClient::EX_UNSUPPORTED));

        const auto& powerModeDozeSuspendStatus =
                mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::DOZE_SUSPEND);
        EXPECT_FALSE(powerModeDozeSuspendStatus.isOk());
        EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(powerModeDozeSuspendStatus,
                                                           IComposerClient::EX_UNSUPPORTED));
    }
}

TEST_P(GraphicsComposerAidlTest, SetVsyncEnabled) {
    mComposerClient->setVsyncAllowed(true);

    EXPECT_TRUE(mComposerClient->setVsync(getPrimaryDisplayId(), true).isOk());
    usleep(60 * 1000);
    EXPECT_TRUE(mComposerClient->setVsync(getPrimaryDisplayId(), false).isOk());

    mComposerClient->setVsyncAllowed(false);
}

TEST_P(GraphicsComposerAidlTest, SetPowerMode) {
    const auto& [status, capabilities] =
            mComposerClient->getDisplayCapabilities(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

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
        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerModeVariations) {
    const auto& [status, capabilities] =
            mComposerClient->getDisplayCapabilities(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());

    const bool isDozeSupported = std::find(capabilities.begin(), capabilities.end(),
                                           DisplayCapability::DOZE) != capabilities.end();
    const bool isSuspendSupported = std::find(capabilities.begin(), capabilities.end(),
                                              DisplayCapability::SUSPEND) != capabilities.end();

    std::vector<PowerMode> modes;

    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::ON);
    modes.push_back(PowerMode::OFF);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
    }
    modes.clear();

    modes.push_back(PowerMode::OFF);
    modes.push_back(PowerMode::OFF);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
    }
    modes.clear();

    modes.push_back(PowerMode::ON);
    modes.push_back(PowerMode::ON);
    for (auto mode : modes) {
        EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
    }
    modes.clear();

    if (isSuspendSupported) {
        modes.push_back(PowerMode::ON_SUSPEND);
        modes.push_back(PowerMode::ON_SUSPEND);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
        }
        modes.clear();
    }

    if (isDozeSupported) {
        modes.push_back(PowerMode::DOZE);
        modes.push_back(PowerMode::DOZE);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
        }
        modes.clear();
    }

    if (isSuspendSupported && isDozeSupported) {
        modes.push_back(PowerMode::DOZE_SUSPEND);
        modes.push_back(PowerMode::DOZE_SUSPEND);
        for (auto mode : modes) {
            EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), mode).isOk());
        }
        modes.clear();
    }
}

TEST_P(GraphicsComposerAidlTest, SetPowerMode_BadDisplay) {
    const auto& status = mComposerClient->setPowerMode(getInvalidDisplayId(), PowerMode::ON);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_DISPLAY));
}

TEST_P(GraphicsComposerAidlTest, SetPowerMode_BadParameter) {
    const auto& status =
            mComposerClient->setPowerMode(getPrimaryDisplayId(), static_cast<PowerMode>(-1));

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrix) {
    const auto& [status, matrix] =
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::SRGB_LINEAR);
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(16, matrix.size());  // matrix should not be empty if call succeeded.

    // the last row is known
    EXPECT_EQ(0.0f, matrix[12]);
    EXPECT_EQ(0.0f, matrix[13]);
    EXPECT_EQ(0.0f, matrix[14]);
    EXPECT_EQ(1.0f, matrix[15]);
}

TEST_P(GraphicsComposerAidlTest, GetDataspaceSaturationMatrix_BadParameter) {
    const auto& [status, matrix] =
            mComposerClient->getDataspaceSaturationMatrix(common::Dataspace::UNKNOWN);

    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

/*
 * Test that no two display configs are exactly the same.
 */
TEST_P(GraphicsComposerAidlTest, GetDisplayConfigNoRepetitions) {
    for (const auto& display : mDisplays) {
        const auto& [status, configs] = mComposerClient->getDisplayConfigs(display.getDisplayId());
        for (std::vector<int>::size_type i = 0; i < configs.size(); i++) {
            for (std::vector<int>::size_type j = i + 1; j < configs.size(); j++) {
                const auto& [widthStatus1, width1] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[i], DisplayAttribute::WIDTH);
                const auto& [heightStatus1, height1] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[i], DisplayAttribute::HEIGHT);
                const auto& [vsyncPeriodStatus1, vsyncPeriod1] =
                        mComposerClient->getDisplayAttribute(display.getDisplayId(), configs[i],
                                                             DisplayAttribute::VSYNC_PERIOD);
                const auto& [groupStatus1, group1] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[i], DisplayAttribute::CONFIG_GROUP);

                const auto& [widthStatus2, width2] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[j], DisplayAttribute::WIDTH);
                const auto& [heightStatus2, height2] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[j], DisplayAttribute::HEIGHT);
                const auto& [vsyncPeriodStatus2, vsyncPeriod2] =
                        mComposerClient->getDisplayAttribute(display.getDisplayId(), configs[j],
                                                             DisplayAttribute::VSYNC_PERIOD);
                const auto& [groupStatus2, group2] = mComposerClient->getDisplayAttribute(
                        display.getDisplayId(), configs[j], DisplayAttribute::CONFIG_GROUP);

                ASSERT_FALSE(width1 == width2 && height1 == height2 &&
                             vsyncPeriod1 == vsyncPeriod2 && group1 == group2);
            }
        }
    }
}

class GraphicsComposerAidlV2Test : public GraphicsComposerAidlTest {
  protected:
    void SetUp() override {
        GraphicsComposerAidlTest::SetUp();
        if (getInterfaceVersion() <= 1) {
            GTEST_SKIP() << "Device interface version is expected to be >= 2";
        }
    }
};

TEST_P(GraphicsComposerAidlV2Test, GetOverlaySupport) {
    const auto& [status, properties] = mComposerClient->getOverlaySupport();
    if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
        status.getServiceSpecificError() == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "getOverlaySupport is not supported";
        return;
    }

    ASSERT_TRUE(status.isOk());
    for (const auto& i : properties.combinations) {
        for (const auto standard : i.standards) {
            const auto val = static_cast<int32_t>(standard) &
                             static_cast<int32_t>(common::Dataspace::STANDARD_MASK);
            ASSERT_TRUE(val == static_cast<int32_t>(standard));
        }
        for (const auto transfer : i.transfers) {
            const auto val = static_cast<int32_t>(transfer) &
                             static_cast<int32_t>(common::Dataspace::TRANSFER_MASK);
            ASSERT_TRUE(val == static_cast<int32_t>(transfer));
        }
        for (const auto range : i.ranges) {
            const auto val = static_cast<int32_t>(range) &
                             static_cast<int32_t>(common::Dataspace::RANGE_MASK);
            ASSERT_TRUE(val == static_cast<int32_t>(range));
        }
    }
}

class GraphicsComposerAidlV3Test : public GraphicsComposerAidlTest {
  protected:
    void SetUp() override {
        GraphicsComposerAidlTest::SetUp();
        if (getInterfaceVersion() <= 2) {
            GTEST_SKIP() << "Device interface version is expected to be >= 3";
        }
    }
};

TEST_P(GraphicsComposerAidlV3Test, GetDisplayConfigurations) {
    for (const auto& display : mDisplays) {
        const auto& [status, displayConfigurations] =
                mComposerClient->getDisplayConfigurations(display.getDisplayId());
        EXPECT_TRUE(status.isOk());
        EXPECT_FALSE(displayConfigurations.empty());

        for (const auto& displayConfig : displayConfigurations) {
            EXPECT_NE(-1, displayConfig.width);
            EXPECT_NE(-1, displayConfig.height);
            EXPECT_NE(-1, displayConfig.vsyncPeriod);
            EXPECT_NE(-1, displayConfig.configGroup);
            if (displayConfig.dpi) {
                EXPECT_NE(-1.f, displayConfig.dpi->x);
                EXPECT_NE(-1.f, displayConfig.dpi->y);
            }
            if (displayConfig.vrrConfig) {
                const auto& vrrConfig = *displayConfig.vrrConfig;
                EXPECT_GE(vrrConfig.minFrameIntervalNs, displayConfig.vsyncPeriod);

                EXPECT_EQ(1, std::count_if(
                                     displayConfigurations.cbegin(), displayConfigurations.cend(),
                                     [displayConfig](const auto& config) {
                                         return config.configGroup == displayConfig.configGroup;
                                     }))
                        << "There should be only one VRR mode in one ConfigGroup";

                const auto verifyFrameIntervalIsDivisorOfVsync = [&](int32_t frameIntervalNs) {
                    constexpr auto kThreshold = 0.05f;  // 5%
                    const auto ratio =
                            static_cast<float>(frameIntervalNs) / displayConfig.vsyncPeriod;
                    return ratio - std::round(ratio) <= kThreshold;
                };

                EXPECT_TRUE(verifyFrameIntervalIsDivisorOfVsync(vrrConfig.minFrameIntervalNs));

                if (vrrConfig.frameIntervalPowerHints) {
                    const auto& frameIntervalPowerHints = *vrrConfig.frameIntervalPowerHints;
                    EXPECT_FALSE(frameIntervalPowerHints.empty());

                    const auto minFrameInterval = *min_element(frameIntervalPowerHints.cbegin(),
                                                               frameIntervalPowerHints.cend());
                    EXPECT_LE(minFrameInterval->frameIntervalNs,
                              VtsComposerClient::kMaxFrameIntervalNs);

                    EXPECT_TRUE(std::all_of(frameIntervalPowerHints.cbegin(),
                                            frameIntervalPowerHints.cend(),
                                            [&](const auto& frameIntervalPowerHint) {
                                                return verifyFrameIntervalIsDivisorOfVsync(
                                                        frameIntervalPowerHint->frameIntervalNs);
                                            }));
                }

                if (vrrConfig.notifyExpectedPresentConfig) {
                    const auto& notifyExpectedPresentConfig =
                            *vrrConfig.notifyExpectedPresentConfig;
                    EXPECT_GT(0, notifyExpectedPresentConfig.notifyExpectedPresentHeadsUpNs);
                    EXPECT_GE(0, notifyExpectedPresentConfig.notifyExpectedPresentTimeoutNs);
                }
            }
        }
    }
}

TEST_P(GraphicsComposerAidlV3Test, GetDisplayConfigsIsSubsetOfGetDisplayConfigurations) {
    for (const auto& display : mDisplays) {
        const auto& [status, displayConfigurations] =
                mComposerClient->getDisplayConfigurations(display.getDisplayId());
        EXPECT_TRUE(status.isOk());

        const auto& [legacyConfigStatus, legacyConfigs] =
                mComposerClient->getDisplayConfigs(display.getDisplayId());
        EXPECT_TRUE(legacyConfigStatus.isOk());
        EXPECT_FALSE(legacyConfigs.empty());
        EXPECT_TRUE(legacyConfigs.size() <= displayConfigurations.size());

        for (const auto legacyConfigId : legacyConfigs) {
            const auto& legacyWidth = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::WIDTH);
            const auto& legacyHeight = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::HEIGHT);
            const auto& legacyVsyncPeriod = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::VSYNC_PERIOD);
            const auto& legacyConfigGroup = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::CONFIG_GROUP);
            const auto& legacyDpiX = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::DPI_X);
            const auto& legacyDpiY = mComposerClient->getDisplayAttribute(
                    display.getDisplayId(), legacyConfigId, DisplayAttribute::DPI_Y);

            EXPECT_TRUE(legacyWidth.first.isOk() && legacyHeight.first.isOk() &&
                        legacyVsyncPeriod.first.isOk() && legacyConfigGroup.first.isOk());

            EXPECT_TRUE(std::any_of(
                    displayConfigurations.begin(), displayConfigurations.end(),
                    [&](const auto& displayConfiguration) {
                        const bool requiredAttributesPredicate =
                                displayConfiguration.configId == legacyConfigId &&
                                displayConfiguration.width == legacyWidth.second &&
                                displayConfiguration.height == legacyHeight.second &&
                                displayConfiguration.vsyncPeriod == legacyVsyncPeriod.second &&
                                displayConfiguration.configGroup == legacyConfigGroup.second;

                        if (!requiredAttributesPredicate) {
                            // Required attributes did not match
                            return false;
                        }

                        // Check optional attributes
                        const auto& [legacyDpiXStatus, legacyDpiXValue] = legacyDpiX;
                        const auto& [legacyDpiYStatus, legacyDpiYValue] = legacyDpiY;
                        if (displayConfiguration.dpi) {
                            if (!legacyDpiXStatus.isOk() || !legacyDpiYStatus.isOk()) {
                                // getDisplayAttribute failed for optional attributes
                                return false;
                            }

                            // DPI values in DisplayConfigurations are not scaled (* 1000.f)
                            // the way they are in the legacy DisplayConfigs.
                            constexpr float kEpsilon = 0.001f;
                            return std::abs(displayConfiguration.dpi->x -
                                            legacyDpiXValue / 1000.f) < kEpsilon &&
                                   std::abs(displayConfiguration.dpi->y -
                                            legacyDpiYValue / 1000.f) < kEpsilon;
                        } else {
                            return !legacyDpiXStatus.isOk() && !legacyDpiYStatus.isOk() &&
                                   EX_SERVICE_SPECIFIC == legacyDpiXStatus.getExceptionCode() &&
                                   EX_SERVICE_SPECIFIC == legacyDpiYStatus.getExceptionCode() &&
                                   IComposerClient::EX_UNSUPPORTED ==
                                           legacyDpiXStatus.getServiceSpecificError() &&
                                   IComposerClient::EX_UNSUPPORTED ==
                                           legacyDpiYStatus.getServiceSpecificError();
                        }
                    }));
        }
    }
}

// TODO(b/291792736) Add detailed VTS test cases for NotifyExpectedPresent
TEST_P(GraphicsComposerAidlV3Test, NotifyExpectedPresent) {
    for (const auto& display : mDisplays) {
        EXPECT_TRUE(mComposerClient
                            ->notifyExpectedPresent(display.getDisplayId(),
                                                    ClockMonotonicTimestamp{0},
                                                    std::chrono::nanoseconds{8ms}.count())
                            .isOk());
    }
}

// Tests for Command.
class GraphicsComposerAidlCommandTest : public GraphicsComposerAidlTest {
  protected:
    void TearDown() override {
        const auto errors = mReader.takeErrors();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_TRUE(mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty());

        ASSERT_NO_FATAL_FAILURE(GraphicsComposerAidlTest::TearDown());
    }

    void execute() {
        std::vector<CommandResultPayload> payloads;
        for (auto& [_, writer] : mWriters) {
            executeInternal(writer, payloads);
        }
        mReader.parse(std::move(payloads));
    }

    void execute(ComposerClientWriter& writer, ComposerClientReader& reader) {
        std::vector<CommandResultPayload> payloads;
        executeInternal(writer, payloads);
        reader.parse(std::move(payloads));
    }

    static inline auto toTimePoint(nsecs_t time) {
        return std::chrono::time_point<std::chrono::steady_clock>(std::chrono::nanoseconds(time));
    }

    void forEachTwoConfigs(int64_t display, std::function<void(int32_t, int32_t)> func) {
        const auto& [status, displayConfigs] = mComposerClient->getDisplayConfigs(display);
        ASSERT_TRUE(status.isOk());
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
            const auto& [status, vsyncPeriodNanos] =
                    mComposerClient->getDisplayVsyncPeriod(display);
            EXPECT_TRUE(status.isOk());
            if (systemTime() <= desiredTimeNanos) {
                EXPECT_EQ(vsyncPeriodNanos, oldPeriodNanos);
            } else if (vsyncPeriodNanos == newPeriodNanos) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(oldPeriodNanos));
        }
    }

    bool checkIfCallbackRefreshRateChangedDebugEnabledReceived(
            std::function<bool(RefreshRateChangedDebugData)> filter) {
        const auto list = mComposerClient->takeListOfRefreshRateChangedDebugData();
        return std::any_of(list.begin(), list.end(), [&](auto refreshRateChangedDebugData) {
            return filter(refreshRateChangedDebugData);
        });
    }

    sp<GraphicBuffer> allocate(uint32_t width, uint32_t height,
                               ::android::PixelFormat pixelFormat) {
        return sp<GraphicBuffer>::make(
                width, height, pixelFormat, /*layerCount*/ 1U,
                static_cast<uint64_t>(common::BufferUsage::CPU_WRITE_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::CPU_READ_OFTEN) |
                        static_cast<uint64_t>(common::BufferUsage::COMPOSER_OVERLAY),
                "VtsHalGraphicsComposer3_TargetTest");
    }

    sp<GraphicBuffer> allocate(::android::PixelFormat pixelFormat) {
        return allocate(static_cast<uint32_t>(getPrimaryDisplay().getDisplayWidth()),
                        static_cast<uint32_t>(getPrimaryDisplay().getDisplayHeight()), pixelFormat);
    }

    void sendRefreshFrame(const VtsDisplay& display, const VsyncPeriodChangeTimeline* timeline) {
        if (timeline != nullptr) {
            // Refresh time should be before newVsyncAppliedTimeNanos
            EXPECT_LT(timeline->refreshTimeNanos, timeline->newVsyncAppliedTimeNanos);

            std::this_thread::sleep_until(toTimePoint(timeline->refreshTimeNanos));
        }

        EXPECT_TRUE(mComposerClient->setPowerMode(display.getDisplayId(), PowerMode::ON).isOk());
        EXPECT_TRUE(mComposerClient
                            ->setColorMode(display.getDisplayId(), ColorMode::NATIVE,
                                           RenderIntent::COLORIMETRIC)
                            .isOk());

        const auto& [status, layer] =
                mComposerClient->createLayer(display.getDisplayId(), kBufferSlotCount);
        EXPECT_TRUE(status.isOk());
        auto& writer = getWriter(display.getDisplayId());
        {
            const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
            ASSERT_NE(nullptr, buffer);
            ASSERT_EQ(::android::OK, buffer->initCheck());
            ASSERT_NE(nullptr, buffer->handle);

            configureLayer(display, layer, Composition::DEVICE, display.getFrameRect(),
                           display.getCrop());
            writer.setLayerBuffer(display.getDisplayId(), layer, /*slot*/ 0, buffer->handle,
                                  /*acquireFence*/ -1);
            writer.setLayerDataspace(display.getDisplayId(), layer, common::Dataspace::UNKNOWN);

            writer.validateDisplay(display.getDisplayId(), ComposerClientWriter::kNoTimestamp,
                                   VtsComposerClient::kNoFrameIntervalNs);
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());

            writer.presentDisplay(display.getDisplayId());
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());
        }

        {
            const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
            ASSERT_NE(nullptr, buffer->handle);

            writer.setLayerBuffer(display.getDisplayId(), layer, /*slot*/ 0, buffer->handle,
                                  /*acquireFence*/ -1);
            writer.setLayerSurfaceDamage(display.getDisplayId(), layer,
                                         std::vector<Rect>(1, {0, 0, 10, 10}));
            writer.validateDisplay(display.getDisplayId(), ComposerClientWriter::kNoTimestamp,
                                   VtsComposerClient::kNoFrameIntervalNs);
            execute();
            ASSERT_TRUE(mReader.takeErrors().empty());

            writer.presentDisplay(display.getDisplayId());
            execute();
        }

        EXPECT_TRUE(mComposerClient->destroyLayer(display.getDisplayId(), layer).isOk());
    }

    sp<::android::Fence> presentAndGetFence(
            std::optional<ClockMonotonicTimestamp> expectedPresentTime) {
        auto& writer = getWriter(getPrimaryDisplayId());
        writer.validateDisplay(getPrimaryDisplayId(), expectedPresentTime,
                               VtsComposerClient::kNoFrameIntervalNs);
        execute();
        EXPECT_TRUE(mReader.takeErrors().empty());

        writer.presentDisplay(getPrimaryDisplayId());
        execute();
        EXPECT_TRUE(mReader.takeErrors().empty());

        auto presentFence = mReader.takePresentFence(getPrimaryDisplayId());
        // take ownership
        const int fenceOwner = presentFence.get();
        *presentFence.getR() = -1;
        EXPECT_NE(-1, fenceOwner);
        return sp<::android::Fence>::make(fenceOwner);
    }

    int32_t getVsyncPeriod() {
        const auto& [status, activeConfig] =
                mComposerClient->getActiveConfig(getPrimaryDisplayId());
        EXPECT_TRUE(status.isOk());

        const auto& [vsyncPeriodStatus, vsyncPeriod] = mComposerClient->getDisplayAttribute(
                getPrimaryDisplayId(), activeConfig, DisplayAttribute::VSYNC_PERIOD);
        EXPECT_TRUE(vsyncPeriodStatus.isOk());
        return vsyncPeriod;
    }

    int64_t createOnScreenLayer(Composition composition = Composition::DEVICE) {
        const auto& [status, layer] =
                mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
        EXPECT_TRUE(status.isOk());
        Rect displayFrame{0, 0, getPrimaryDisplay().getDisplayWidth(),
                          getPrimaryDisplay().getDisplayHeight()};
        FRect cropRect{0, 0, (float)getPrimaryDisplay().getDisplayWidth(),
                       (float)getPrimaryDisplay().getDisplayHeight()};
        configureLayer(getPrimaryDisplay(), layer, composition, displayFrame, cropRect);
        auto& writer = getWriter(getPrimaryDisplayId());
        writer.setLayerDataspace(getPrimaryDisplayId(), layer, common::Dataspace::UNKNOWN);
        return layer;
    }

    void sendBufferUpdate(int64_t layer) {
        const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        ASSERT_NE(nullptr, buffer->handle);

        auto& writer = getWriter(getPrimaryDisplayId());
        writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, buffer->handle,
                              /*acquireFence*/ -1);

        const sp<::android::Fence> presentFence =
                presentAndGetFence(ComposerClientWriter::kNoTimestamp);
        presentFence->waitForever(LOG_TAG);
    }

    bool hasDisplayCapability(int64_t display, DisplayCapability cap) {
        const auto& [status, capabilities] = mComposerClient->getDisplayCapabilities(display);
        EXPECT_TRUE(status.isOk());

        return std::find(capabilities.begin(), capabilities.end(), cap) != capabilities.end();
    }

    void Test_setActiveConfigWithConstraints(const TestParameters& params) {
        for (VtsDisplay& display : mDisplays) {
            forEachTwoConfigs(display.getDisplayId(), [&](int32_t config1, int32_t config2) {
                EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config1).isOk());
                sendRefreshFrame(display, nullptr);

                const auto displayConfigGroup1 = display.getDisplayConfig(config1);
                int32_t vsyncPeriod1 = displayConfigGroup1.vsyncPeriod;
                int32_t configGroup1 = displayConfigGroup1.configGroup;

                const auto displayConfigGroup2 = display.getDisplayConfig(config2);
                int32_t vsyncPeriod2 = displayConfigGroup2.vsyncPeriod;
                int32_t configGroup2 = displayConfigGroup2.configGroup;

                if (vsyncPeriod1 == vsyncPeriod2) {
                    return;  // continue
                }

                // We don't allow delayed change when changing config groups
                if (params.delayForChange > 0 && configGroup1 != configGroup2) {
                    return;  // continue
                }

                VsyncPeriodChangeConstraints constraints = {
                        .desiredTimeNanos = systemTime() + params.delayForChange,
                        .seamlessRequired = false};
                const auto& [status, timeline] = mComposerClient->setActiveConfigWithConstraints(
                        &display, config2, constraints);
                EXPECT_TRUE(status.isOk());

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
                waitForVsyncPeriodChange(display.getDisplayId(), timeline,
                                         constraints.desiredTimeNanos, vsyncPeriod1, vsyncPeriod2);

                // At this point the refresh rate should have changed already, however in rare
                // cases the implementation might have missed the deadline. In this case a new
                // timeline should have been provided.
                auto newTimeline = mComposerClient->takeLastVsyncPeriodChangeTimeline();
                if (timeline.refreshRequired && params.refreshMiss) {
                    EXPECT_TRUE(newTimeline.has_value());
                }

                if (newTimeline.has_value()) {
                    if (newTimeline->refreshRequired) {
                        sendRefreshFrame(display, &newTimeline.value());
                    }
                    waitForVsyncPeriodChange(display.getDisplayId(), newTimeline.value(),
                                             constraints.desiredTimeNanos, vsyncPeriod1,
                                             vsyncPeriod2);
                }

                const auto& [vsyncPeriodNanosStatus, vsyncPeriodNanos] =
                        mComposerClient->getDisplayVsyncPeriod(display.getDisplayId());
                EXPECT_TRUE(vsyncPeriodNanosStatus.isOk());
                EXPECT_EQ(vsyncPeriodNanos, vsyncPeriod2);
            });
        }
    }

    void Test_expectedPresentTime(std::optional<int> framesDelay) {
        if (hasCapability(Capability::PRESENT_FENCE_IS_NOT_RELIABLE)) {
            GTEST_SUCCEED() << "Device has unreliable present fences capability, skipping";
            return;
        }

        ASSERT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());

        const auto vsyncPeriod = getVsyncPeriod();

        const auto buffer1 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        const auto buffer2 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        ASSERT_NE(nullptr, buffer1);
        ASSERT_NE(nullptr, buffer2);

        const auto layer = createOnScreenLayer();
        auto& writer = getWriter(getPrimaryDisplayId());
        writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, buffer1->handle,
                              /*acquireFence*/ -1);
        const sp<::android::Fence> presentFence1 =
                presentAndGetFence(ComposerClientWriter::kNoTimestamp);
        presentFence1->waitForever(LOG_TAG);

        auto expectedPresentTime = presentFence1->getSignalTime() + vsyncPeriod;
        if (framesDelay.has_value()) {
            expectedPresentTime += *framesDelay * vsyncPeriod;
        }

        writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, buffer2->handle,
                              /*acquireFence*/ -1);
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
        EXPECT_GE(actualPresentTime, expectedPresentTime - vsyncPeriod / 2);

        ASSERT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::OFF).isOk());
    }

    void configureLayer(const VtsDisplay& display, int64_t layer, Composition composition,
                        const Rect& displayFrame, const FRect& cropRect) {
        auto& writer = getWriter(display.getDisplayId());
        writer.setLayerCompositionType(display.getDisplayId(), layer, composition);
        writer.setLayerDisplayFrame(display.getDisplayId(), layer, displayFrame);
        writer.setLayerPlaneAlpha(display.getDisplayId(), layer, /*alpha*/ 1);
        writer.setLayerSourceCrop(display.getDisplayId(), layer, cropRect);
        writer.setLayerTransform(display.getDisplayId(), layer, static_cast<Transform>(0));
        writer.setLayerVisibleRegion(display.getDisplayId(), layer,
                                     std::vector<Rect>(1, displayFrame));
        writer.setLayerZOrder(display.getDisplayId(), layer, /*z*/ 10);
        writer.setLayerBlendMode(display.getDisplayId(), layer, BlendMode::NONE);
        writer.setLayerSurfaceDamage(display.getDisplayId(), layer,
                                     std::vector<Rect>(1, displayFrame));
    }
    // clang-format off
    const std::array<float, 16> kIdentity = {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
    }};
    // clang-format on

    ComposerClientWriter& getWriter(int64_t display) {
        std::lock_guard guard{mWritersMutex};
        auto [it, _] = mWriters.try_emplace(display, display);
        return it->second;
    }

    ComposerClientReader mReader;

  private:
    void executeInternal(ComposerClientWriter& writer,
                         std::vector<CommandResultPayload>& payloads) {
        auto commands = writer.takePendingCommands();
        if (commands.empty()) {
            return;
        }

        auto [status, results] = mComposerClient->executeCommands(commands);
        ASSERT_TRUE(status.isOk()) << "executeCommands failed " << status.getDescription();

        payloads.reserve(payloads.size() + results.size());
        payloads.insert(payloads.end(), std::make_move_iterator(results.begin()),
                        std::make_move_iterator(results.end()));
    }

    // Guards access to the map itself. Callers must ensure not to attempt to
    // - modify the same writer from multiple threads
    // - insert a new writer into the map during concurrent access, which would invalidate
    //   references from other threads
    std::mutex mWritersMutex;
    std::unordered_map<int64_t, ComposerClientWriter> mWriters GUARDED_BY(mWritersMutex);
};

TEST_P(GraphicsComposerAidlCommandTest, SetColorTransform) {
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setColorTransform(getPrimaryDisplayId(), kIdentity.data());
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerColorTransform) {
    const auto& [status, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(status.isOk());
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerColorTransform(getPrimaryDisplayId(), layer, kIdentity.data());
    execute();

    const auto errors = mReader.takeErrors();
    if (errors.size() == 1 && errors[0].errorCode == IComposerClient::EX_UNSUPPORTED) {
        GTEST_SUCCEED() << "setLayerColorTransform is not supported";
        return;
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetDisplayBrightness) {
    const auto& [status, capabilities] =
            mComposerClient->getDisplayCapabilities(getPrimaryDisplayId());
    ASSERT_TRUE(status.isOk());
    bool brightnessSupport = std::find(capabilities.begin(), capabilities.end(),
                                       DisplayCapability::BRIGHTNESS) != capabilities.end();
    auto& writer = getWriter(getPrimaryDisplayId());
    if (!brightnessSupport) {
        writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 0.5f, -1.f);
        execute();
        const auto errors = mReader.takeErrors();
        EXPECT_EQ(1, errors.size());
        EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errors[0].errorCode);
        GTEST_SUCCEED() << "SetDisplayBrightness is not supported";
        return;
    }

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 0.0f, -1.f);
    execute();
    EXPECT_TRUE(mReader.takeErrors().empty());

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 0.5f, -1.f);
    execute();
    EXPECT_TRUE(mReader.takeErrors().empty());

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 1.0f, -1.f);
    execute();
    EXPECT_TRUE(mReader.takeErrors().empty());

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ -1.0f, -1.f);
    execute();
    EXPECT_TRUE(mReader.takeErrors().empty());

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ 2.0f, -1.f);
    execute();
    {
        const auto errors = mReader.takeErrors();
        ASSERT_EQ(1, errors.size());
        EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, errors[0].errorCode);
    }

    writer.setDisplayBrightness(getPrimaryDisplayId(), /*brightness*/ -2.0f, -1.f);
    execute();
    {
        const auto errors = mReader.takeErrors();
        ASSERT_EQ(1, errors.size());
        EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, errors[0].errorCode);
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetClientTarget) {
    EXPECT_TRUE(mComposerClient->setClientTargetSlotCount(getPrimaryDisplayId(), kBufferSlotCount)
                        .isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setClientTarget(getPrimaryDisplayId(), /*slot*/ 0, nullptr, /*acquireFence*/ -1,
                           Dataspace::UNKNOWN, std::vector<Rect>(), 1.0f);

    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetOutputBuffer) {
    const auto& [status, virtualDisplayCount] = mComposerClient->getMaxVirtualDisplayCount();
    EXPECT_TRUE(status.isOk());
    if (virtualDisplayCount == 0) {
        GTEST_SUCCEED() << "no virtual display support";
        return;
    }

    const auto& [displayStatus, display] = mComposerClient->createVirtualDisplay(
            /*width*/ 64, /*height*/ 64, common::PixelFormat::IMPLEMENTATION_DEFINED,
            kBufferSlotCount);
    EXPECT_TRUE(displayStatus.isOk());

    const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    const auto handle = buffer->handle;
    auto& writer = getWriter(display.display);
    writer.setOutputBuffer(display.display, /*slot*/ 0, handle, /*releaseFence*/ -1);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, ValidDisplay) {
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, AcceptDisplayChanges) {
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    writer.acceptDisplayChanges(getPrimaryDisplayId());
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, PresentDisplay) {
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    writer.presentDisplay(getPrimaryDisplayId());
    execute();
}

/**
 * Test IComposerClient::Command::PRESENT_DISPLAY
 *
 * Test that IComposerClient::Command::PRESENT_DISPLAY works without
 * additional call to validateDisplay when only the layer buffer handle and
 * surface damage have been set
 */
TEST_P(GraphicsComposerAidlCommandTest, PresentDisplayNoLayerStateChanges) {
    EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());

    const auto& [renderIntentsStatus, renderIntents] =
            mComposerClient->getRenderIntents(getPrimaryDisplayId(), ColorMode::NATIVE);
    EXPECT_TRUE(renderIntentsStatus.isOk());
    auto& writer = getWriter(getPrimaryDisplayId());
    for (auto intent : renderIntents) {
        EXPECT_TRUE(mComposerClient->setColorMode(getPrimaryDisplayId(), ColorMode::NATIVE, intent)
                            .isOk());

        const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        const auto handle = buffer->handle;
        ASSERT_NE(nullptr, handle);

        const auto& [layerStatus, layer] =
                mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
        EXPECT_TRUE(layerStatus.isOk());

        Rect displayFrame{0, 0, getPrimaryDisplay().getDisplayWidth(),
                          getPrimaryDisplay().getDisplayHeight()};
        FRect cropRect{0, 0, (float)getPrimaryDisplay().getDisplayWidth(),
                       (float)getPrimaryDisplay().getDisplayHeight()};
        configureLayer(getPrimaryDisplay(), layer, Composition::CURSOR, displayFrame, cropRect);
        writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle,
                              /*acquireFence*/ -1);
        writer.setLayerDataspace(getPrimaryDisplayId(), layer, Dataspace::UNKNOWN);
        writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                               VtsComposerClient::kNoFrameIntervalNs);
        execute();
        if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
            GTEST_SUCCEED() << "Composition change requested, skipping test";
            return;
        }

        ASSERT_TRUE(mReader.takeErrors().empty());
        writer.presentDisplay(getPrimaryDisplayId());
        execute();
        ASSERT_TRUE(mReader.takeErrors().empty());

        const auto buffer2 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        const auto handle2 = buffer2->handle;
        ASSERT_NE(nullptr, handle2);
        writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle2,
                              /*acquireFence*/ -1);
        writer.setLayerSurfaceDamage(getPrimaryDisplayId(), layer,
                                     std::vector<Rect>(1, {0, 0, 10, 10}));
        writer.presentDisplay(getPrimaryDisplayId());
        execute();
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerCursorPosition) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle, /*acquireFence*/ -1);

    Rect displayFrame{0, 0, getPrimaryDisplay().getDisplayWidth(),
                      getPrimaryDisplay().getDisplayHeight()};
    FRect cropRect{0, 0, (float)getPrimaryDisplay().getDisplayWidth(),
                   (float)getPrimaryDisplay().getDisplayHeight()};
    configureLayer(getPrimaryDisplay(), layer, Composition::CURSOR, displayFrame, cropRect);
    writer.setLayerDataspace(getPrimaryDisplayId(), layer, Dataspace::UNKNOWN);
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);

    execute();

    if (!mReader.takeChangedCompositionTypes(getPrimaryDisplayId()).empty()) {
        GTEST_SUCCEED() << "Composition change requested, skipping test";
        return;
    }
    writer.presentDisplay(getPrimaryDisplayId());
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerCursorPosition(getPrimaryDisplayId(), layer, /*x*/ 1, /*y*/ 1);
    execute();

    writer.setLayerCursorPosition(getPrimaryDisplayId(), layer, /*x*/ 0, /*y*/ 0);
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    writer.presentDisplay(getPrimaryDisplayId());
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerBuffer) {
    const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);

    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle, /*acquireFence*/ -1);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerBufferMultipleTimes) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());
    auto& writer = getWriter(getPrimaryDisplayId());

    // Setup 3 buffers in the buffer cache, with the last buffer being active. Then, emulate the
    // Android platform code that clears all 3 buffer slots by setting all but the active buffer
    // slot to a placeholder buffer, and then restoring the active buffer.

    // This is used on HALs that don't support setLayerBufferSlotsToClear (version <= 3.1).

    const auto buffer1 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer1);
    const auto handle1 = buffer1->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle1, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    const auto buffer2 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer2);
    const auto handle2 = buffer2->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 1, handle2, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    const auto buffer3 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer3);
    const auto handle3 = buffer3->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 2, handle3, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    // Older versions of the HAL clear all but the active buffer slot with a placeholder buffer,
    // and then restoring the current active buffer at the end
    auto clearSlotBuffer = allocate(1u, 1u, ::android::PIXEL_FORMAT_RGB_888);
    ASSERT_NE(nullptr, clearSlotBuffer);
    auto clearSlotBufferHandle = clearSlotBuffer->handle;

    // clear buffer slots 0 and 1 with new layer commands... and then...
    writer.setLayerBufferWithNewCommand(getPrimaryDisplayId(), layer, /* slot */ 0,
                                        clearSlotBufferHandle, /*acquireFence*/ -1);
    writer.setLayerBufferWithNewCommand(getPrimaryDisplayId(), layer, /* slot */ 1,
                                        clearSlotBufferHandle, /*acquireFence*/ -1);
    // ...reset the layer buffer to the current active buffer slot with a final new command
    writer.setLayerBufferWithNewCommand(getPrimaryDisplayId(), layer, /*slot*/ 2, nullptr,
                                        /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerSurfaceDamage) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    Rect empty{0, 0, 0, 0};
    Rect unit{0, 0, 1, 1};

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerSurfaceDamage(getPrimaryDisplayId(), layer, std::vector<Rect>(1, empty));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerSurfaceDamage(getPrimaryDisplayId(), layer, std::vector<Rect>(1, unit));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerSurfaceDamage(getPrimaryDisplayId(), layer, std::vector<Rect>());
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerBlockingRegion) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    Rect empty{0, 0, 0, 0};
    Rect unit{0, 0, 1, 1};

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBlockingRegion(getPrimaryDisplayId(), layer, std::vector<Rect>(1, empty));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBlockingRegion(getPrimaryDisplayId(), layer, std::vector<Rect>(1, unit));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBlockingRegion(getPrimaryDisplayId(), layer, std::vector<Rect>());
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerBlendMode) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBlendMode(getPrimaryDisplayId(), layer, BlendMode::NONE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBlendMode(getPrimaryDisplayId(), layer, BlendMode::PREMULTIPLIED);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBlendMode(getPrimaryDisplayId(), layer, BlendMode::COVERAGE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerColor) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerColor(getPrimaryDisplayId(), layer, Color{1.0f, 1.0f, 1.0f, 1.0f});
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerColor(getPrimaryDisplayId(), layer, Color{0.0f, 0.0f, 0.0f, 0.0f});
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerCompositionType) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerCompositionType(getPrimaryDisplayId(), layer, Composition::CLIENT);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerCompositionType(getPrimaryDisplayId(), layer, Composition::DEVICE);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerCompositionType(getPrimaryDisplayId(), layer, Composition::SOLID_COLOR);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerCompositionType(getPrimaryDisplayId(), layer, Composition::CURSOR);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, DisplayDecoration) {
    for (VtsDisplay& display : mDisplays) {
        const auto [layerStatus, layer] =
                mComposerClient->createLayer(display.getDisplayId(), kBufferSlotCount);
        EXPECT_TRUE(layerStatus.isOk());

        const auto [error, support] =
                mComposerClient->getDisplayDecorationSupport(display.getDisplayId());

        const auto format = (error.isOk() && support) ? support->format
                        : aidl::android::hardware::graphics::common::PixelFormat::RGBA_8888;
        const auto decorBuffer = allocate(static_cast<::android::PixelFormat>(format));
        ASSERT_NE(nullptr, decorBuffer);
        if (::android::OK != decorBuffer->initCheck()) {
            if (support) {
                FAIL() << "Device advertised display decoration support with format  "
                       << aidl::android::hardware::graphics::common::toString(format)
                       << " but failed to allocate it!";
            } else {
                FAIL() << "Device advertised NO display decoration support, but it should "
                       << "still be able to allocate "
                       << aidl::android::hardware::graphics::common::toString(format);
            }
        }

        configureLayer(display, layer, Composition::DISPLAY_DECORATION, display.getFrameRect(),
                          display.getCrop());
        auto& writer = getWriter(display.getDisplayId());
        writer.setLayerBuffer(display.getDisplayId(), layer, /*slot*/ 0, decorBuffer->handle,
                              /*acquireFence*/ -1);
        writer.validateDisplay(display.getDisplayId(), ComposerClientWriter::kNoTimestamp,
                               VtsComposerClient::kNoFrameIntervalNs);
        execute();
        if (support) {
            ASSERT_TRUE(mReader.takeErrors().empty());
        } else {
            const auto errors = mReader.takeErrors();
            ASSERT_EQ(1, errors.size());
            EXPECT_EQ(IComposerClient::EX_UNSUPPORTED, errors[0].errorCode);
        }
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerDataspace) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerDataspace(getPrimaryDisplayId(), layer, Dataspace::UNKNOWN);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerDisplayFrame) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerDisplayFrame(getPrimaryDisplayId(), layer, Rect{0, 0, 1, 1});
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerPlaneAlpha) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerPlaneAlpha(getPrimaryDisplayId(), layer, /*alpha*/ 0.0f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerPlaneAlpha(getPrimaryDisplayId(), layer, /*alpha*/ 1.0f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerSidebandStream) {
    if (!hasCapability(Capability::SIDEBAND_STREAM)) {
        GTEST_SUCCEED() << "no sideband stream support";
        return;
    }

    const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    const auto handle = buffer->handle;
    ASSERT_NE(nullptr, handle);

    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerSidebandStream(getPrimaryDisplayId(), layer, handle);
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerSourceCrop) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerSourceCrop(getPrimaryDisplayId(), layer, FRect{0.0f, 0.0f, 1.0f, 1.0f});
    execute();
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerTransform) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerTransform(getPrimaryDisplayId(), layer, static_cast<Transform>(0));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer, Transform::FLIP_H);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer, Transform::FLIP_V);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer, Transform::ROT_90);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer, Transform::ROT_180);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer, Transform::ROT_270);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer,
                             static_cast<Transform>(static_cast<int>(Transform::FLIP_H) |
                                                    static_cast<int>(Transform::ROT_90)));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerTransform(getPrimaryDisplayId(), layer,
                             static_cast<Transform>(static_cast<int>(Transform::FLIP_V) |
                                                    static_cast<int>(Transform::ROT_90)));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerVisibleRegion) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    Rect empty{0, 0, 0, 0};
    Rect unit{0, 0, 1, 1};

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerVisibleRegion(getPrimaryDisplayId(), layer, std::vector<Rect>(1, empty));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerVisibleRegion(getPrimaryDisplayId(), layer, std::vector<Rect>(1, unit));
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerVisibleRegion(getPrimaryDisplayId(), layer, std::vector<Rect>());
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerZOrder) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerZOrder(getPrimaryDisplayId(), layer, /*z*/ 10);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerZOrder(getPrimaryDisplayId(), layer, /*z*/ 0);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandTest, SetLayerPerFrameMetadata) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());

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

    auto& writer = getWriter(getPrimaryDisplayId());
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
    writer.setLayerPerFrameMetadata(getPrimaryDisplayId(), layer, aidlMetadata);
    execute();

    const auto errors = mReader.takeErrors();
    if (errors.size() == 1 && errors[0].errorCode == EX_UNSUPPORTED_OPERATION) {
        GTEST_SUCCEED() << "SetLayerPerFrameMetadata is not supported";
        EXPECT_TRUE(mComposerClient->destroyLayer(getPrimaryDisplayId(), layer).isOk());
        return;
    }

    EXPECT_TRUE(mComposerClient->destroyLayer(getPrimaryDisplayId(), layer).isOk());
}

TEST_P(GraphicsComposerAidlCommandTest, setLayerBrightness) {
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);

    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBrightness(getPrimaryDisplayId(), layer, 0.2f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBrightness(getPrimaryDisplayId(), layer, 1.f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBrightness(getPrimaryDisplayId(), layer, 0.f);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    writer.setLayerBrightness(getPrimaryDisplayId(), layer, -1.f);
    execute();
    {
        const auto errors = mReader.takeErrors();
        ASSERT_EQ(1, errors.size());
        EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, errors[0].errorCode);
    }

    writer.setLayerBrightness(getPrimaryDisplayId(), layer, std::nanf(""));
    execute();
    {
        const auto errors = mReader.takeErrors();
        ASSERT_EQ(1, errors.size());
        EXPECT_EQ(IComposerClient::EX_BAD_PARAMETER, errors[0].errorCode);
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetActiveConfigWithConstraints) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = false});
}

TEST_P(GraphicsComposerAidlCommandTest, SetActiveConfigWithConstraints_Delayed) {
    Test_setActiveConfigWithConstraints({.delayForChange = 300'000'000,  // 300ms
                                         .refreshMiss = false});
}

TEST_P(GraphicsComposerAidlCommandTest, SetActiveConfigWithConstraints_MissRefresh) {
    Test_setActiveConfigWithConstraints({.delayForChange = 0, .refreshMiss = true});
}

TEST_P(GraphicsComposerAidlCommandTest, GetDisplayVsyncPeriod) {
    for (VtsDisplay& display : mDisplays) {
        const auto& [status, configs] = mComposerClient->getDisplayConfigs(display.getDisplayId());
        EXPECT_TRUE(status.isOk());

        for (int32_t config : configs) {
            int32_t expectedVsyncPeriodNanos = display.getDisplayConfig(config).vsyncPeriod;

            VsyncPeriodChangeConstraints constraints;

            constraints.desiredTimeNanos = systemTime();
            constraints.seamlessRequired = false;

            const auto& [timelineStatus, timeline] =
                    mComposerClient->setActiveConfigWithConstraints(&display, config, constraints);
            EXPECT_TRUE(timelineStatus.isOk());

            if (timeline.refreshRequired) {
                sendRefreshFrame(display, &timeline);
            }
            waitForVsyncPeriodChange(display.getDisplayId(), timeline, constraints.desiredTimeNanos,
                                     /*odPeriodNanos*/ 0, expectedVsyncPeriodNanos);

            int32_t vsyncPeriodNanos;
            int retryCount = 100;
            do {
                std::this_thread::sleep_for(10ms);
                const auto& [vsyncPeriodNanosStatus, vsyncPeriodNanosValue] =
                        mComposerClient->getDisplayVsyncPeriod(display.getDisplayId());

                EXPECT_TRUE(vsyncPeriodNanosStatus.isOk());
                vsyncPeriodNanos = vsyncPeriodNanosValue;
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
                const auto& [vsyncPeriodNanosStatus, vsyncPeriodNanosValue] =
                        mComposerClient->getDisplayVsyncPeriod(display.getDisplayId());

                EXPECT_TRUE(vsyncPeriodNanosStatus.isOk());
                vsyncPeriodNanos = vsyncPeriodNanosValue;
                EXPECT_EQ(vsyncPeriodNanos, expectedVsyncPeriodNanos);
            }
        }
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetActiveConfigWithConstraints_SeamlessNotAllowed) {
    VsyncPeriodChangeConstraints constraints;
    constraints.seamlessRequired = true;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        forEachTwoConfigs(display.getDisplayId(), [&](int32_t config1, int32_t config2) {
            int32_t configGroup1 = display.getDisplayConfig(config1).configGroup;
            int32_t configGroup2 = display.getDisplayConfig(config2).configGroup;
            if (configGroup1 != configGroup2) {
                EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config1).isOk());
                sendRefreshFrame(display, nullptr);
                const auto& [status, _] = mComposerClient->setActiveConfigWithConstraints(
                        &display, config2, constraints);
                EXPECT_FALSE(status.isOk());
                EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(
                        status, IComposerClient::EX_SEAMLESS_NOT_ALLOWED));
            }
        });
    }
}

TEST_P(GraphicsComposerAidlCommandTest, ExpectedPresentTime_NoTimestamp) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(/*framesDelay*/ std::nullopt));
}

TEST_P(GraphicsComposerAidlCommandTest, ExpectedPresentTime_0) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(/*framesDelay*/ 0));
}

TEST_P(GraphicsComposerAidlCommandTest, ExpectedPresentTime_5) {
    ASSERT_NO_FATAL_FAILURE(Test_expectedPresentTime(/*framesDelay*/ 5));
}

TEST_P(GraphicsComposerAidlCommandTest, SetIdleTimerEnabled_Unsupported) {
    const bool hasDisplayIdleTimerSupport =
            hasDisplayCapability(getPrimaryDisplayId(), DisplayCapability::DISPLAY_IDLE_TIMER);
    if (!hasDisplayIdleTimerSupport) {
        const auto& status =
                mComposerClient->setIdleTimerEnabled(getPrimaryDisplayId(), /*timeout*/ 0);
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
    }
}

TEST_P(GraphicsComposerAidlCommandTest, SetIdleTimerEnabled_BadParameter) {
    const bool hasDisplayIdleTimerSupport =
            hasDisplayCapability(getPrimaryDisplayId(), DisplayCapability::DISPLAY_IDLE_TIMER);
    if (!hasDisplayIdleTimerSupport) {
        GTEST_SUCCEED() << "DisplayCapability::DISPLAY_IDLE_TIMER is not supported";
        return;
    }

    const auto& status =
            mComposerClient->setIdleTimerEnabled(getPrimaryDisplayId(), /*timeout*/ -1);
    EXPECT_FALSE(status.isOk());
    EXPECT_NO_FATAL_FAILURE(assertServiceSpecificError(status, IComposerClient::EX_BAD_PARAMETER));
}

TEST_P(GraphicsComposerAidlCommandTest, SetIdleTimerEnabled_Disable) {
    const bool hasDisplayIdleTimerSupport =
            hasDisplayCapability(getPrimaryDisplayId(), DisplayCapability::DISPLAY_IDLE_TIMER);
    if (!hasDisplayIdleTimerSupport) {
        GTEST_SUCCEED() << "DisplayCapability::DISPLAY_IDLE_TIMER is not supported";
        return;
    }

    EXPECT_TRUE(mComposerClient->setIdleTimerEnabled(getPrimaryDisplayId(), /*timeout*/ 0).isOk());
    std::this_thread::sleep_for(1s);
    EXPECT_EQ(0, mComposerClient->getVsyncIdleCount());
}

TEST_P(GraphicsComposerAidlCommandTest, SetIdleTimerEnabled_Timeout_2) {
    const bool hasDisplayIdleTimerSupport =
            hasDisplayCapability(getPrimaryDisplayId(), DisplayCapability::DISPLAY_IDLE_TIMER);
    if (!hasDisplayIdleTimerSupport) {
        GTEST_SUCCEED() << "DisplayCapability::DISPLAY_IDLE_TIMER is not supported";
        return;
    }

    EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::ON).isOk());
    EXPECT_TRUE(mComposerClient->setIdleTimerEnabled(getPrimaryDisplayId(), /*timeout*/ 0).isOk());

    const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer->handle);

    const auto layer = createOnScreenLayer();
    auto& writer = getWriter(getPrimaryDisplayId());
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, buffer->handle,
                          /*acquireFence*/ -1);
    int32_t vsyncIdleCount = mComposerClient->getVsyncIdleCount();
    auto earlyVsyncIdleTime = systemTime() + std::chrono::nanoseconds(2s).count();
    EXPECT_TRUE(
            mComposerClient->setIdleTimerEnabled(getPrimaryDisplayId(), /*timeout*/ 2000).isOk());

    const sp<::android::Fence> presentFence =
            presentAndGetFence(ComposerClientWriter::kNoTimestamp);
    presentFence->waitForever(LOG_TAG);

    std::this_thread::sleep_for(3s);
    if (vsyncIdleCount < mComposerClient->getVsyncIdleCount()) {
        EXPECT_GE(mComposerClient->getVsyncIdleTime(), earlyVsyncIdleTime);
    }

    EXPECT_TRUE(mComposerClient->setPowerMode(getPrimaryDisplayId(), PowerMode::OFF).isOk());
}

class GraphicsComposerAidlCommandV2Test : public GraphicsComposerAidlCommandTest {
  protected:
    void SetUp() override {
        GraphicsComposerAidlTest::SetUp();
        if (getInterfaceVersion() <= 1) {
            GTEST_SKIP() << "Device interface version is expected to be >= 2";
        }
    }
};
/**
 * Test Capability::SKIP_VALIDATE
 *
 * Capability::SKIP_VALIDATE has been deprecated and should not be enabled.
 */
TEST_P(GraphicsComposerAidlCommandV2Test, SkipValidateDeprecatedTest) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ASSERT_FALSE(hasCapability(Capability::SKIP_VALIDATE))
            << "Found Capability::SKIP_VALIDATE capability.";
#pragma clang diagnostic pop
}

TEST_P(GraphicsComposerAidlCommandV2Test, SetLayerBufferSlotsToClear) {
    // Older HAL versions use a backwards compatible way of clearing buffer slots
    // HAL at version 1 or lower does not have LayerCommand::bufferSlotsToClear
    const auto& [layerStatus, layer] =
            mComposerClient->createLayer(getPrimaryDisplayId(), kBufferSlotCount);
    EXPECT_TRUE(layerStatus.isOk());
    auto& writer = getWriter(getPrimaryDisplayId());

    // setup 3 buffers in the buffer cache, with the last buffer being active
    // then emulate the Android platform code that clears all 3 buffer slots

    const auto buffer1 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer1);
    const auto handle1 = buffer1->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 0, handle1, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    const auto buffer2 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer2);
    const auto handle2 = buffer2->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 1, handle2, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    const auto buffer3 = allocate(::android::PIXEL_FORMAT_RGBA_8888);
    ASSERT_NE(nullptr, buffer3);
    const auto handle3 = buffer3->handle;
    writer.setLayerBuffer(getPrimaryDisplayId(), layer, /*slot*/ 2, handle3, /*acquireFence*/ -1);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());

    // Ensure we can clear all 3 buffer slots, even the active buffer - it is assumed the
    // current active buffer's slot will be cleared, but still remain the active buffer and no
    // errors will occur.
    writer.setLayerBufferSlotsToClear(getPrimaryDisplayId(), layer, {0, 1, 2});
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlCommandV2Test, SetRefreshRateChangedCallbackDebug_Unsupported) {
    if (!hasCapability(Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG)) {
        auto status = mComposerClient->setRefreshRateChangedCallbackDebugEnabled(
                getPrimaryDisplayId(), /*enabled*/ true);
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));

        status = mComposerClient->setRefreshRateChangedCallbackDebugEnabled(getPrimaryDisplayId(),
                                                                            /*enabled*/ false);
        EXPECT_FALSE(status.isOk());
        EXPECT_NO_FATAL_FAILURE(
                assertServiceSpecificError(status, IComposerClient::EX_UNSUPPORTED));
    }
}

TEST_P(GraphicsComposerAidlCommandV2Test, SetRefreshRateChangedCallbackDebug_Enabled) {
    if (!hasCapability(Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG)) {
        GTEST_SUCCEED() << "Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG is not supported";
        return;
    }

    for (VtsDisplay& display : mDisplays) {
        const auto displayId = display.getDisplayId();
        EXPECT_TRUE(mComposerClient->setPowerMode(displayId, PowerMode::ON).isOk());
        // Enable the callback
        ASSERT_TRUE(mComposerClient
                            ->setRefreshRateChangedCallbackDebugEnabled(displayId,
                                                                        /*enabled*/ true)
                            .isOk());
        std::this_thread::sleep_for(100ms);

        const auto [status, configId] = mComposerClient->getActiveConfig(display.getDisplayId());
        EXPECT_TRUE(status.isOk());

        const auto displayFilter = [&](auto refreshRateChangedDebugData) {
            bool nonVrrRateMatching = true;
            if (std::optional<VrrConfig> vrrConfigOpt =
                        display.getDisplayConfig(configId).vrrConfig;
                getInterfaceVersion() >= 3 && !vrrConfigOpt) {
                nonVrrRateMatching = refreshRateChangedDebugData.refreshPeriodNanos ==
                                     refreshRateChangedDebugData.vsyncPeriodNanos;
            }
            const bool isDisplaySame =
                    display.getDisplayId() == refreshRateChangedDebugData.display;
            return nonVrrRateMatching && isDisplaySame;
        };

        // Check that we immediately got a callback
        EXPECT_TRUE(checkIfCallbackRefreshRateChangedDebugEnabledReceived(displayFilter));

        ASSERT_TRUE(mComposerClient
                            ->setRefreshRateChangedCallbackDebugEnabled(displayId,
                                                                        /*enabled*/ false)
                            .isOk());
    }
}

TEST_P(GraphicsComposerAidlCommandV2Test,
       SetRefreshRateChangedCallbackDebugEnabled_noCallbackWhenIdle) {
    if (!hasCapability(Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG)) {
        GTEST_SUCCEED() << "Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG is not supported";
        return;
    }

    auto display = getEditablePrimaryDisplay();
    const auto displayId = display.getDisplayId();

    if (!hasDisplayCapability(displayId, DisplayCapability::DISPLAY_IDLE_TIMER)) {
        GTEST_SUCCEED() << "DisplayCapability::DISPLAY_IDLE_TIMER is not supported";
        return;
    }

    EXPECT_TRUE(mComposerClient->setPowerMode(displayId, PowerMode::ON).isOk());
    EXPECT_TRUE(mComposerClient->setPeakRefreshRateConfig(&display).isOk());

    ASSERT_TRUE(mComposerClient->setIdleTimerEnabled(displayId, /*timeoutMs*/ 500).isOk());
    // Enable the callback
    ASSERT_TRUE(mComposerClient
                        ->setRefreshRateChangedCallbackDebugEnabled(displayId,
                                                                    /*enabled*/ true)
                        .isOk());

    const auto displayFilter = [displayId](auto refreshRateChangedDebugData) {
        return displayId == refreshRateChangedDebugData.display;
    };

    int retryCount = 3;
    do {
        // Wait for 1s so that we enter the idle state
        std::this_thread::sleep_for(1s);
        if (!checkIfCallbackRefreshRateChangedDebugEnabledReceived(displayFilter)) {
            // DID NOT receive a callback, we are in the idle state.
            break;
        }
    } while (--retryCount > 0);

    if (retryCount == 0) {
        GTEST_SUCCEED() << "Unable to enter the idle mode";
        return;
    }

    // Send the REFRESH_RATE_INDICATOR update
    ASSERT_NO_FATAL_FAILURE(
            sendBufferUpdate(createOnScreenLayer(Composition::REFRESH_RATE_INDICATOR)));
    std::this_thread::sleep_for(1s);
    EXPECT_FALSE(checkIfCallbackRefreshRateChangedDebugEnabledReceived(displayFilter))
            << "A callback should not be received for REFRESH_RATE_INDICATOR";

    EXPECT_TRUE(mComposerClient
                        ->setRefreshRateChangedCallbackDebugEnabled(displayId,
                                                                    /*enabled*/ false)
                        .isOk());
}

TEST_P(GraphicsComposerAidlCommandV2Test,
       SetRefreshRateChangedCallbackDebugEnabled_SetActiveConfigWithConstraints) {
    if (!hasCapability(Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG)) {
        GTEST_SUCCEED() << "Capability::REFRESH_RATE_CHANGED_CALLBACK_DEBUG is not supported";
        return;
    }

    VsyncPeriodChangeConstraints constraints;
    constraints.seamlessRequired = false;
    constraints.desiredTimeNanos = systemTime();

    for (VtsDisplay& display : mDisplays) {
        const auto displayId = display.getDisplayId();
        EXPECT_TRUE(mComposerClient->setPowerMode(displayId, PowerMode::ON).isOk());

        // Enable the callback
        ASSERT_TRUE(mComposerClient
                            ->setRefreshRateChangedCallbackDebugEnabled(displayId, /*enabled*/ true)
                            .isOk());

        forEachTwoConfigs(displayId, [&](int32_t config1, int32_t config2) {
            const int32_t vsyncPeriod1 = display.getDisplayConfig(config1).vsyncPeriod;
            const int32_t vsyncPeriod2 = display.getDisplayConfig(config2).vsyncPeriod;

            if (vsyncPeriod1 == vsyncPeriod2) {
                return;  // continue
            }

            EXPECT_TRUE(mComposerClient->setActiveConfig(&display, config1).isOk());
            sendRefreshFrame(display, nullptr);

            const auto& [status, timeline] =
                    mComposerClient->setActiveConfigWithConstraints(&display, config2, constraints);
            EXPECT_TRUE(status.isOk());

            if (timeline.refreshRequired) {
                sendRefreshFrame(display, &timeline);
            }

            const auto callbackFilter = [displayId,
                                         vsyncPeriod2](auto refreshRateChangedDebugData) {
                constexpr int kVsyncThreshold = 1000;
                return displayId == refreshRateChangedDebugData.display &&
                       std::abs(vsyncPeriod2 - refreshRateChangedDebugData.vsyncPeriodNanos) <=
                               kVsyncThreshold;
            };

            int retryCount = 3;
            do {
                std::this_thread::sleep_for(100ms);
                if (checkIfCallbackRefreshRateChangedDebugEnabledReceived(callbackFilter)) {
                    GTEST_SUCCEED() << "Received a callback successfully";
                    break;
                }
            } while (--retryCount > 0);

            if (retryCount == 0) {
                GTEST_FAIL() << "failed to get a callback for the display " << displayId
                             << " with config " << config2;
            }
        });

        EXPECT_TRUE(
                mComposerClient
                        ->setRefreshRateChangedCallbackDebugEnabled(displayId, /*enabled*/ false)
                        .isOk());
    }
}

TEST_P(GraphicsComposerAidlCommandTest, MultiThreadedPresent) {
    std::vector<VtsDisplay*> displays;
    for (auto& display : mDisplays) {
        if (hasDisplayCapability(display.getDisplayId(),
                                 DisplayCapability::MULTI_THREADED_PRESENT)) {
            displays.push_back(&display);
        }
    }

    const size_t numDisplays = displays.size();
    if (numDisplays <= 1u) {
        GTEST_SKIP();
    }

    // When multi-threaded, use a reader per display. As with mWriters, this mutex
    // guards access to the map.
    std::mutex readersMutex;
    std::unordered_map<int64_t, ComposerClientReader> readers;
    std::vector<std::thread> threads;
    threads.reserve(numDisplays);

    // Each display will have a layer to present. This maps from the display to
    // the layer, so we can properly destroy each layer at the end.
    std::unordered_map<int64_t, int64_t> layers;

    for (auto* const display : displays) {
        const int64_t displayId = display->getDisplayId();

        // Ensure that all writers and readers have been added to their respective
        // maps initially, so that the following loop never modifies the maps. The
        // maps are accessed from different threads, and if the maps were modified,
        // this would invalidate their iterators, and therefore references to the
        // writers and readers.
        auto& writer = getWriter(displayId);
        {
            std::lock_guard guard{readersMutex};
            readers.try_emplace(displayId, displayId);
        }

        EXPECT_TRUE(mComposerClient->setPowerMode(displayId, PowerMode::ON).isOk());

        const auto& [status, layer] = mComposerClient->createLayer(displayId, kBufferSlotCount);
        const auto buffer = allocate(::android::PIXEL_FORMAT_RGBA_8888);
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(::android::OK, buffer->initCheck());
        ASSERT_NE(nullptr, buffer->handle);

        configureLayer(*display, layer, Composition::DEVICE, display->getFrameRect(),
                       display->getCrop());
        writer.setLayerBuffer(displayId, layer, /*slot*/ 0, buffer->handle,
                              /*acquireFence*/ -1);
        writer.setLayerDataspace(displayId, layer, common::Dataspace::UNKNOWN);
        layers.try_emplace(displayId, layer);
    }

    for (auto* const display : displays) {
        const int64_t displayId = display->getDisplayId();
        auto& writer = getWriter(displayId);
        std::unique_lock lock{readersMutex};
        auto& reader = readers.at(displayId);
        lock.unlock();

        writer.validateDisplay(displayId, ComposerClientWriter::kNoTimestamp,
                               VtsComposerClient::kNoFrameIntervalNs);
        execute(writer, reader);

        threads.emplace_back([this, displayId, &readers, &readersMutex]() {
            auto& writer = getWriter(displayId);
            std::unique_lock lock{readersMutex};
            ComposerClientReader& reader = readers.at(displayId);
            lock.unlock();

            writer.presentDisplay(displayId);
            execute(writer, reader);
            ASSERT_TRUE(reader.takeErrors().empty());

            auto presentFence = reader.takePresentFence(displayId);
            // take ownership
            const int fenceOwner = presentFence.get();
            *presentFence.getR() = -1;
            EXPECT_NE(-1, fenceOwner);
            const auto presentFence2 = sp<::android::Fence>::make(fenceOwner);
            presentFence2->waitForever(LOG_TAG);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (auto& [displayId, layer] : layers) {
        EXPECT_TRUE(mComposerClient->destroyLayer(displayId, layer).isOk());
    }

    std::lock_guard guard{readersMutex};
    for (auto& [displayId, reader] : readers) {
        ASSERT_TRUE(reader.takeErrors().empty());
        ASSERT_TRUE(reader.takeChangedCompositionTypes(displayId).empty());
    }
}

class GraphicsComposerAidlBatchedCommandTest : public GraphicsComposerAidlCommandTest {
  protected:
    void SetUp() override {
        GraphicsComposerAidlCommandTest::SetUp();
        if (getInterfaceVersion() <= 2) {
            GTEST_SKIP() << "Device interface version is expected to be >= 3";
        }
    }
    void TearDown() override {
        const auto errors = mReader.takeErrors();
        ASSERT_TRUE(mReader.takeErrors().empty());
        ASSERT_NO_FATAL_FAILURE(GraphicsComposerAidlTest::TearDown());
    }
};

TEST_P(GraphicsComposerAidlBatchedCommandTest, CreateBatchedCommand) {
    auto& writer = getWriter(getPrimaryDisplayId());
    int64_t layer = 5;
    writer.setLayerLifecycleBatchCommandType(getPrimaryDisplayId(), layer,
                                             LayerLifecycleBatchCommandType::CREATE);
    writer.setNewBufferSlotCount(getPrimaryDisplayId(), layer, 1);
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlBatchedCommandTest, DestroyBatchedCommand) {
    auto& writer = getWriter(getPrimaryDisplayId());
    int64_t layer = 5;
    writer.setLayerLifecycleBatchCommandType(getPrimaryDisplayId(), layer,
                                             LayerLifecycleBatchCommandType::CREATE);
    writer.setNewBufferSlotCount(getPrimaryDisplayId(), layer, 1);
    writer.validateDisplay(getPrimaryDisplayId(), ComposerClientWriter::kNoTimestamp,
                           VtsComposerClient::kNoFrameIntervalNs);
    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
    writer.setLayerLifecycleBatchCommandType(getPrimaryDisplayId(), layer,
                                             LayerLifecycleBatchCommandType::DESTROY);
    layer++;
    writer.setLayerLifecycleBatchCommandType(getPrimaryDisplayId(), layer,
                                             LayerLifecycleBatchCommandType::CREATE);
    writer.setNewBufferSlotCount(getPrimaryDisplayId(), layer, 1);

    execute();
    ASSERT_TRUE(mReader.takeErrors().empty());
}

TEST_P(GraphicsComposerAidlBatchedCommandTest, NoCreateDestroyBatchedCommandIncorrectLayer) {
    auto& writer = getWriter(getPrimaryDisplayId());
    int64_t layer = 5;
    writer.setLayerLifecycleBatchCommandType(getPrimaryDisplayId(), layer,
                                             LayerLifecycleBatchCommandType::DESTROY);
    execute();
    const auto errors = mReader.takeErrors();
    ASSERT_TRUE(errors.size() == 1 && errors[0].errorCode == IComposerClient::EX_BAD_LAYER);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlBatchedCommandTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlBatchedCommandTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
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
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlV2Test);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlV2Test,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlV3Test);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlV3Test,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(GraphicsComposerAidlCommandV2Test);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, GraphicsComposerAidlCommandV2Test,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IComposer::descriptor)),
        ::android::PrintInstanceNameToString);
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
