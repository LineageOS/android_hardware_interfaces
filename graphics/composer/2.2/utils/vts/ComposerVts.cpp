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

#include <composer-vts/2.2/ComposerVts.h>

#include <VtsHalHidlTargetTestBase.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#include <hidl/HidlTransportUtils.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace vts {

using android::hardware::details::canCastInterface;
using android::hardware::details::getDescriptor;
using android::hardware::graphics::composer::V2_2::IComposerClient;

std::unique_ptr<ComposerClient_v2_2> Composer_v2_2::createClient_v2_2() {
    std::unique_ptr<ComposerClient_v2_2> client;
    mComposer->createClient([&](const auto& tmpError, const auto& tmpClient) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
        ALOGV("tmpClient is a %s", getDescriptor(&(*tmpClient)).c_str());
        ASSERT_TRUE(canCastInterface(
            &(*tmpClient), "android.hardware.graphics.composer@2.2::IComposerClient", false))
            << "Cannot create 2.2 IComposerClient";
        client = std::make_unique<ComposerClient_v2_2>(IComposerClient::castFrom(tmpClient, true));
    });

    return client;
}

sp<V2_2::IComposerClient> ComposerClient_v2_2::getRaw() const {
    return mClient_v2_2;
}

std::vector<IComposerClient::PerFrameMetadataKey> ComposerClient_v2_2::getPerFrameMetadataKeys(
    Display display) {
    std::vector<IComposerClient::PerFrameMetadataKey> keys;
    mClient_v2_2->getPerFrameMetadataKeys(display, [&](const auto& tmpError, const auto& tmpKeys) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get HDR metadata keys";
        keys = tmpKeys;
    });

    return keys;
}

void ComposerClient_v2_2::execute_v2_2(V2_1::vts::TestCommandReader* reader,
                                       V2_2::CommandWriterBase* writer) {
    bool queueChanged = false;
    uint32_t commandLength = 0;
    hidl_vec<hidl_handle> commandHandles;
    ASSERT_TRUE(writer->writeQueue(&queueChanged, &commandLength, &commandHandles));

    if (queueChanged) {
        auto ret = mClient_v2_2->setInputCommandQueue(*writer->getMQDescriptor());
        ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
        return;
    }

    mClient_v2_2->executeCommands(commandLength, commandHandles,
                                  [&](const auto& tmpError, const auto& tmpOutQueueChanged,
                                      const auto& tmpOutLength, const auto& tmpOutHandles) {
                                      ASSERT_EQ(Error::NONE, tmpError);

                                      if (tmpOutQueueChanged) {
                                          mClient_v2_2->getOutputCommandQueue(
                                              [&](const auto& tmpError, const auto& tmpDescriptor) {
                                                  ASSERT_EQ(Error::NONE, tmpError);
                                                  reader->setMQDescriptor(tmpDescriptor);
                                              });
                                      }

                                      ASSERT_TRUE(reader->readQueue(tmpOutLength, tmpOutHandles));
                                      reader->parse();
                                  });
}

Display ComposerClient_v2_2::createVirtualDisplay_2_2(uint32_t width, uint32_t height,
                                                      PixelFormat formatHint,
                                                      uint32_t outputBufferSlotCount,
                                                      PixelFormat* outFormat) {
    Display display = 0;
    mClient_v2_2->createVirtualDisplay_2_2(
        width, height, formatHint, outputBufferSlotCount,
        [&](const auto& tmpError, const auto& tmpDisplay, const auto& tmpFormat) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to create virtual display";
            display = tmpDisplay;
            *outFormat = tmpFormat;

            ASSERT_TRUE(mDisplayResources.insert({display, DisplayResource(true)}).second)
                << "duplicated virtual display id " << display;
        });

    return display;
}

bool ComposerClient_v2_2::getClientTargetSupport_2_2(Display display, uint32_t width,
                                                     uint32_t height, PixelFormat format,
                                                     Dataspace dataspace) {
    Error error =
        mClient_v2_2->getClientTargetSupport_2_2(display, width, height, format, dataspace);
    return error == Error::NONE;
}

void ComposerClient_v2_2::setPowerMode_2_2(Display display, V2_2::IComposerClient::PowerMode mode) {
    Error error = mClient_v2_2->setPowerMode_2_2(display, mode);
    ASSERT_TRUE(error == Error::NONE || error == Error::UNSUPPORTED) << "failed to set power mode";
}

void ComposerClient_v2_2::setReadbackBuffer(Display display, const native_handle_t* buffer,
                                            int32_t /* releaseFence */) {
    // Ignoring fence, HIDL doesn't care
    Error error = mClient_v2_2->setReadbackBuffer(display, buffer, nullptr);
    ASSERT_EQ(Error::NONE, error) << "failed to setReadbackBuffer";
}

void ComposerClient_v2_2::getReadbackBufferAttributes(Display display, PixelFormat* outPixelFormat,
                                                      Dataspace* outDataspace) {
    mClient_v2_2->getReadbackBufferAttributes(
        display,
        [&](const auto& tmpError, const auto& tmpOutPixelFormat, const auto& tmpOutDataspace) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get readback buffer attributes";
            *outPixelFormat = tmpOutPixelFormat;
            *outDataspace = tmpOutDataspace;
        });
}

void ComposerClient_v2_2::getReadbackBufferFence(Display display, int32_t* outFence) {
    hidl_handle handle;
    mClient_v2_2->getReadbackBufferFence(display, [&](const auto& tmpError, const auto& tmpHandle) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get readback fence";
        handle = tmpHandle;
    });
    *outFence = 0;
}

std::vector<ColorMode> ComposerClient_v2_2::getColorModes(Display display) {
    std::vector<ColorMode> modes;
    mClient_v2_2->getColorModes_2_2(display, [&](const auto& tmpError, const auto& tmpModes) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get color modes";
        modes = tmpModes;
    });
    return modes;
}

std::vector<RenderIntent> ComposerClient_v2_2::getRenderIntents(Display display, ColorMode mode) {
    std::vector<RenderIntent> intents;
    mClient_v2_2->getRenderIntents(
        display, mode, [&](const auto& tmpError, const auto& tmpIntents) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get render intents";
            intents = tmpIntents;
        });
    return intents;
}

void ComposerClient_v2_2::setColorMode(Display display, ColorMode mode, RenderIntent intent) {
    Error error = mClient_v2_2->setColorMode_2_2(display, mode, intent);
    ASSERT_TRUE(error == Error::NONE || error == Error::UNSUPPORTED) << "failed to set color mode";
}

std::array<float, 16> ComposerClient_v2_2::getDataspaceSaturationMatrix(Dataspace dataspace) {
    std::array<float, 16> matrix;
    mClient_v2_2->getDataspaceSaturationMatrix(
        dataspace, [&](const auto& tmpError, const auto& tmpMatrix) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get datasapce saturation matrix";
            std::copy_n(tmpMatrix.data(), matrix.size(), matrix.begin());
        });

    return matrix;
}

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
