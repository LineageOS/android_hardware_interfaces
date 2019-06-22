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

using details::canCastInterface;
using details::getDescriptor;

std::unique_ptr<ComposerClient> Composer::createClient() {
    std::unique_ptr<ComposerClient> client;
    getRaw()->createClient([&](const auto& tmpError, const auto& tmpClient) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to create client";
        ALOGV("tmpClient is a %s", getDescriptor(&(*tmpClient)).c_str());
        ASSERT_TRUE(canCastInterface(
            &(*tmpClient), "android.hardware.graphics.composer@2.2::IComposerClient", false))
            << "Cannot create 2.2 IComposerClient";
        client = std::make_unique<ComposerClient>(IComposerClient::castFrom(tmpClient, true));
    });

    return client;
}

sp<IComposerClient> ComposerClient::getRaw() const {
    return mClient;
}

std::vector<IComposerClient::PerFrameMetadataKey> ComposerClient::getPerFrameMetadataKeys(
    Display display) {
    std::vector<IComposerClient::PerFrameMetadataKey> keys;
    mClient->getPerFrameMetadataKeys(display, [&](const auto& tmpError, const auto& tmpKeys) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get HDR metadata keys";
        keys = tmpKeys;
    });

    return keys;
}

void ComposerClient::execute(V2_1::vts::TestCommandReader* reader, CommandWriterBase* writer) {
    bool queueChanged = false;
    uint32_t commandLength = 0;
    hidl_vec<hidl_handle> commandHandles;
    ASSERT_TRUE(writer->writeQueue(&queueChanged, &commandLength, &commandHandles));

    if (queueChanged) {
        auto ret = mClient->setInputCommandQueue(*writer->getMQDescriptor());
        ASSERT_EQ(Error::NONE, static_cast<Error>(ret));
    }

    mClient->executeCommands(commandLength, commandHandles,
                             [&](const auto& tmpError, const auto& tmpOutQueueChanged,
                                 const auto& tmpOutLength, const auto& tmpOutHandles) {
                                 ASSERT_EQ(Error::NONE, tmpError);

                                 if (tmpOutQueueChanged) {
                                     mClient->getOutputCommandQueue(
                                         [&](const auto& tmpError, const auto& tmpDescriptor) {
                                             ASSERT_EQ(Error::NONE, tmpError);
                                             reader->setMQDescriptor(tmpDescriptor);
                                         });
                                 }

                                 ASSERT_TRUE(reader->readQueue(tmpOutLength, tmpOutHandles));
                                 reader->parse();
                             });
    reader->reset();
    writer->reset();
}

Display ComposerClient::createVirtualDisplay_2_2(uint32_t width, uint32_t height,
                                                 PixelFormat formatHint,
                                                 uint32_t outputBufferSlotCount,
                                                 PixelFormat* outFormat) {
    Display display = 0;
    mClient->createVirtualDisplay_2_2(
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

bool ComposerClient::getClientTargetSupport_2_2(Display display, uint32_t width, uint32_t height,
                                                PixelFormat format, Dataspace dataspace) {
    Error error = mClient->getClientTargetSupport_2_2(display, width, height, format, dataspace);
    return error == Error::NONE;
}

void ComposerClient::setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) {
    Error error = mClient->setPowerMode_2_2(display, mode);
    ASSERT_TRUE(error == Error::NONE || error == Error::UNSUPPORTED) << "failed to set power mode";
}

void ComposerClient::setReadbackBuffer(Display display, const native_handle_t* buffer,
                                       int32_t /* releaseFence */) {
    // Ignoring fence, HIDL doesn't care
    Error error = mClient->setReadbackBuffer(display, buffer, nullptr);
    ASSERT_EQ(Error::NONE, error) << "failed to setReadbackBuffer";
}

void ComposerClient::getReadbackBufferAttributes(Display display, PixelFormat* outPixelFormat,
                                                 Dataspace* outDataspace) {
    mClient->getReadbackBufferAttributes(
        display,
        [&](const auto& tmpError, const auto& tmpOutPixelFormat, const auto& tmpOutDataspace) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get readback buffer attributes";
            *outPixelFormat = tmpOutPixelFormat;
            *outDataspace = tmpOutDataspace;
        });
}

void ComposerClient::getReadbackBufferFence(Display display, int32_t* outFence) {
    mClient->getReadbackBufferFence(display, [&](const auto& tmpError, const auto& tmpHandle) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get readback fence";
        const native_handle_t* nativeFenceHandle = tmpHandle.getNativeHandle();
        *outFence = dup(nativeFenceHandle->data[0]);
    });
}

std::vector<ColorMode> ComposerClient::getColorModes(Display display) {
    std::vector<ColorMode> modes;
    mClient->getColorModes_2_2(display, [&](const auto& tmpError, const auto& tmpModes) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get color modes";
        modes = tmpModes;
    });
    return modes;
}

std::vector<RenderIntent> ComposerClient::getRenderIntents(Display display, ColorMode mode) {
    std::vector<RenderIntent> intents;
    mClient->getRenderIntents(display, mode, [&](const auto& tmpError, const auto& tmpIntents) {
        ASSERT_EQ(Error::NONE, tmpError) << "failed to get render intents";
        intents = tmpIntents;
    });
    return intents;
}

void ComposerClient::setColorMode(Display display, ColorMode mode, RenderIntent intent) {
    Error error = mClient->setColorMode_2_2(display, mode, intent);
    ASSERT_TRUE(error == Error::NONE || error == Error::UNSUPPORTED) << "failed to set color mode";
}

std::array<float, 16> ComposerClient::getDataspaceSaturationMatrix(Dataspace dataspace) {
    std::array<float, 16> matrix;
    mClient->getDataspaceSaturationMatrix(
        dataspace, [&](const auto& tmpError, const auto& tmpMatrix) {
            ASSERT_EQ(Error::NONE, tmpError) << "failed to get datasapce saturation matrix";
            std::copy_n(tmpMatrix.data(), matrix.size(), matrix.begin());
        });

    return matrix;
}

Gralloc::Gralloc() {
    [this] {
        ALOGD("Attempting to initialize gralloc3");
        ASSERT_NO_FATAL_FAILURE(mGralloc3 = std::make_shared<Gralloc3>("default", "default",
                                                                       /*errOnFailure=*/false));
        if (mGralloc3->getMapper() == nullptr || mGralloc3->getAllocator() == nullptr) {
            mGralloc3 = nullptr;
            ALOGD("Failed to create gralloc3, initializing gralloc2_1");
            mGralloc2_1 = std::make_shared<Gralloc2_1>(/*errOnFailure*/ false);
            if (!mGralloc2_1->getMapper()) {
                mGralloc2_1 = nullptr;
                ALOGD("Failed to create gralloc2_1, initializing gralloc2");
                ASSERT_NO_FATAL_FAILURE(mGralloc2 = std::make_shared<Gralloc2>());
            }
        }
    }();
}

bool Gralloc::validateBufferSize(const native_handle_t* bufferHandle, uint32_t width,
                                 uint32_t height, uint32_t layerCount, PixelFormat format,
                                 uint64_t usage, uint32_t stride) {
    if (mGralloc3) {
        IMapper3::BufferDescriptorInfo info{};
        info.width = width;
        info.height = height;
        info.layerCount = layerCount;
        info.format = static_cast<android::hardware::graphics::common::V1_2::PixelFormat>(format);
        info.usage = usage;
        return mGralloc3->validateBufferSize(bufferHandle, info, stride);
    } else if (mGralloc2_1) {
        IMapper2_1::BufferDescriptorInfo info{};
        info.width = width;
        info.height = height;
        info.layerCount = layerCount;
        info.format = static_cast<android::hardware::graphics::common::V1_1::PixelFormat>(format);
        info.usage = usage;
        return mGralloc2_1->validateBufferSize(bufferHandle, info, stride);
    } else {
        return true;
    }
}

}  // namespace vts
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
