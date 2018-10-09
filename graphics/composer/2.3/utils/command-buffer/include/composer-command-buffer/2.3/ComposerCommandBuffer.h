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

#ifndef LOG_TAG
#warn "ComposerCommandBuffer.h included without LOG_TAG"
#endif

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {

using android::hardware::MessageQueue;
using android::hardware::graphics::common::V1_2::Dataspace;
using android::hardware::graphics::composer::V2_1::Error;
using android::hardware::graphics::composer::V2_1::IComposerCallback;
using android::hardware::graphics::composer::V2_1::Layer;
using android::hardware::graphics::composer::V2_3::IComposerClient;

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase : public V2_2::CommandWriterBase {
   public:
    void setLayerDataspace(Dataspace dataspace) {
        setLayerDataspaceInternal(static_cast<int32_t>(dataspace));
    }

    void setClientTarget(uint32_t slot, const native_handle_t* target, int acquireFence,
                         Dataspace dataspace, const std::vector<IComposerClient::Rect>& damage) {
        setClientTargetInternal(slot, target, acquireFence, static_cast<int32_t>(dataspace),
                                damage);
    }

    CommandWriterBase(uint32_t initialMaxSize) : V2_2::CommandWriterBase(initialMaxSize) {}

    static constexpr uint16_t kSetLayerColorTransformLength = 16;
    void setLayerColorTransform(const float* matrix) {
        beginCommand_2_3(IComposerClient::Command::SET_LAYER_COLOR_TRANSFORM,
                         kSetLayerColorTransformLength);
        for (int i = 0; i < 16; i++) {
            writeFloat(matrix[i]);
        }
        endCommand();
    }

   protected:
    void beginCommand_2_3(IComposerClient::Command command, uint16_t length) {
        V2_2::CommandWriterBase::beginCommand_2_2(
            static_cast<V2_2::IComposerClient::Command>(static_cast<int32_t>(command)), length);
    }
};

// This class helps parse a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandReaderBase : public V2_2::CommandReaderBase {
   public:
    CommandReaderBase() : V2_2::CommandReaderBase(){};
};

}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
