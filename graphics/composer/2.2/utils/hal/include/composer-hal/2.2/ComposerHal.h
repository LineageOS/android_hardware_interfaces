/*
 * Copyright 2018 The Android Open Source Project
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

#pragma once

#include <android-base/unique_fd.h>
#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#include <composer-hal/2.1/ComposerHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace hal {

using common::V1_0::Dataspace;
using common::V1_0::PixelFormat;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;

class ComposerHal : public V2_1::hal::ComposerHal {
   public:
    // superceded by setPowerMode_2_2
    Error setPowerMode(Display display, V2_1::IComposerClient::PowerMode mode) override {
        return setPowerMode_2_2(display, static_cast<IComposerClient::PowerMode>(mode));
    }

    virtual Error getPerFrameMetadataKeys(
        Display display, std::vector<IComposerClient::PerFrameMetadataKey>* outKeys) = 0;
    virtual Error setPerFrameMetadata(
        Display display, const std::vector<IComposerClient::PerFrameMetadata>& metadata) = 0;

    virtual Error getReadbackBufferAttributes(Display display, PixelFormat* outFormat,
                                              Dataspace* outDataspace) = 0;
    virtual Error setReadbackBuffer(Display display, const native_handle_t* bufferHandle,
                                    base::unique_fd fenceFd) = 0;
    virtual Error getReadbackBufferFence(Display display, base::unique_fd* outFenceFd) = 0;

    virtual Error setPowerMode_2_2(Display display, IComposerClient::PowerMode mode) = 0;

    virtual Error setLayerFloatColor(Display display, Layer layer,
                                     IComposerClient::FloatColor color) = 0;
};

}  // namespace hal
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
