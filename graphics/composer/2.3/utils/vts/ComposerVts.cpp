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

#include <composer-vts/2.3/ComposerVts.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace vts {

using V2_1::Error;

Composer::Composer() : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>()) {}

Composer::Composer(const std::string& name)
    : Composer(::testing::VtsHalHidlTargetTestBase::getService<IComposer>(name)) {}

Composer::Composer(const sp<IComposer>& composer)
    : V2_2::vts::Composer(composer), mComposer(composer) {}

std::unique_ptr<ComposerClient> Composer::createClient() {
    std::unique_ptr<ComposerClient> client;
    mComposer->createClient_2_3([&client](const auto& tmpError, const auto& tmpClient) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
        client = std::make_unique<ComposerClient>(tmpClient);
    });

    return client;
}

sp<IComposerClient> ComposerClient::getRaw() const {
    return mClient;
}

bool ComposerClient::getDisplayIdentificationData(Display display, uint8_t* outPort,
                                                  std::vector<uint8_t>* outData) {
    bool supported = true;
    mClient->getDisplayIdentificationData(
        display, [&](const auto& tmpError, const auto& tmpPort, const auto& tmpData) {
            if (tmpError == Error::UNSUPPORTED) {
                supported = false;
                return;
            }
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get display identification data";

            *outPort = tmpPort;
            *outData = tmpData;
            ASSERT_FALSE(outData->empty()) << "data is empty";
        });

    return supported;
}

std::vector<ColorMode> ComposerClient::getColorModes_2_3(Display display) {
    std::vector<ColorMode> modes;
    mClient->getColorModes_2_3(display, [&](const auto& tmpError, const auto& tmpModes) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get color modes";
        modes = tmpModes;
    });
    return modes;
}

void ComposerClient::setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent) {
    Error error = mClient->setColorMode_2_3(display, mode, intent);
    ASSERT_TRUE(error == Error::NONE || error == Error::UNSUPPORTED) << "failed to set color mode";
}

std::vector<RenderIntent> ComposerClient::getRenderIntents_2_3(Display display, ColorMode mode) {
    std::vector<RenderIntent> intents;
    mClient->getRenderIntents_2_3(display, mode, [&](const auto& tmpError, const auto& tmpIntents) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get render intents";
        intents = tmpIntents;
    });
    return intents;
}

void ComposerClient::getReadbackBufferAttributes_2_3(Display display, PixelFormat* outPixelFormat,
                                                     Dataspace* outDataspace) {
    mClient->getReadbackBufferAttributes_2_3(
        display,
        [&](const auto& tmpError, const auto& tmpOutPixelFormat, const auto& tmpOutDataspace) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get readback buffer attributes";
            *outPixelFormat = tmpOutPixelFormat;
            *outDataspace = tmpOutDataspace;
        });
}

bool ComposerClient::getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                                PixelFormat format, Dataspace dataspace) {
    Error error = mClient->getClientTargetSupport_2_3(display, width, height, format, dataspace);
    return error == Error::NONE;
}

std::vector<IComposerClient::PerFrameMetadataKey> ComposerClient::getPerFrameMetadataKeys_2_3(
    Display display) {
    std::vector<IComposerClient::PerFrameMetadataKey> keys;
    mClient->getPerFrameMetadataKeys_2_3(display, [&](const auto& tmpError, const auto& tmpKeys) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get perFrameMetadataKeys";
        keys = tmpKeys;
    });
    return keys;
}

std::vector<Hdr> ComposerClient::getHdrCapabilities_2_3(Display display, float* outMaxLuminance,
                                                        float* outMaxAverageLuminance,
                                                        float* outMinLuminance) {
    std::vector<Hdr> types;
    mClient->getHdrCapabilities_2_3(
        display, [&](const auto& tmpError, const auto& tmpTypes, const auto& tmpMaxLuminance,
                     const auto& tmpMaxAverageLuminance, const auto& tmpMinLuminance) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get HDR capabilities";
            types = tmpTypes;
            *outMaxLuminance = tmpMaxLuminance;
            *outMaxAverageLuminance = tmpMaxAverageLuminance;
            *outMinLuminance = tmpMinLuminance;
        });

    return types;
}

Error ComposerClient::getDisplayedContentSamplingAttributes(
    uint64_t display, PixelFormat& format, Dataspace& dataspace,
    hidl_bitfield<IComposerClient::FormatColorComponent>& componentMask) {
    auto error = Error::BAD_PARAMETER;
    mClient->getDisplayedContentSamplingAttributes(
        display, [&](const auto& tmpError, const auto& tmpFormat, const auto& tmpDataspace,
                     const auto& tmpComponentMask) {
            error = tmpError;
            format = tmpFormat;
            dataspace = tmpDataspace;
            componentMask = tmpComponentMask;
        });
    return error;
}

Error ComposerClient::setDisplayedContentSamplingEnabled(
    uint64_t display, IComposerClient::DisplayedContentSampling enable,
    hidl_bitfield<IComposerClient::FormatColorComponent> componentMask, uint64_t maxFrames) {
    return mClient->setDisplayedContentSamplingEnabled(display, enable, componentMask, maxFrames);
}

Error ComposerClient::getDisplayedContentSample(uint64_t display, uint64_t maxFrames,
                                                uint64_t timestamp, uint64_t& frameCount,
                                                hidl_vec<uint64_t>& sampleComponent0,
                                                hidl_vec<uint64_t>& sampleComponent1,
                                                hidl_vec<uint64_t>& sampleComponent2,
                                                hidl_vec<uint64_t>& sampleComponent3) {
    auto error = Error::BAD_PARAMETER;
    mClient->getDisplayedContentSample(
        display, maxFrames, timestamp,
        [&](const auto& tmpError, const auto& tmpFrameCount, const auto& tmpSamples0,
            const auto& tmpSamples1, const auto& tmpSamples2, const auto& tmpSamples3) {
            error = tmpError;
            frameCount = tmpFrameCount;
            sampleComponent0 = tmpSamples0;
            sampleComponent1 = tmpSamples1;
            sampleComponent2 = tmpSamples2;
            sampleComponent3 = tmpSamples3;
        });
    return error;
}

std::vector<IComposerClient::DisplayCapability> ComposerClient::getDisplayCapabilities(
    Display display) {
    std::vector<IComposerClient::DisplayCapability> capabilities;
    mClient->getDisplayCapabilities(
        display, [&](const auto&, const auto& tmpCapabilities) { capabilities = tmpCapabilities; });

    return capabilities;
}

}  // namespace vts
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
