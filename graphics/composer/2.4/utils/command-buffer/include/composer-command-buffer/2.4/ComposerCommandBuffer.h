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

#pragma once

#ifndef LOG_TAG
#warn "ComposerCommandBuffer.h included without LOG_TAG"
#endif

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
#include <composer-command-buffer/2.3/ComposerCommandBuffer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_4 {

using android::hardware::MessageQueue;
using android::hardware::graphics::composer::V2_4::Error;
using android::hardware::graphics::composer::V2_4::IComposerClient;

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase : public V2_3::CommandWriterBase {
  public:
    static constexpr uint16_t kSetClientTargetPropertyLength = 2;

    CommandWriterBase(uint32_t initialMaxSize) : V2_3::CommandWriterBase(initialMaxSize) {}

    void setClientTargetProperty(
            const IComposerClient::ClientTargetProperty& clientTargetProperty) {
        beginCommand(IComposerClient::Command::SET_CLIENT_TARGET_PROPERTY,
                     kSetClientTargetPropertyLength);
        writeSigned(static_cast<int32_t>(clientTargetProperty.pixelFormat));
        writeSigned(static_cast<int32_t>(clientTargetProperty.dataspace));
        endCommand();
    }
};

// This class helps parse a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandReaderBase : public V2_3::CommandReaderBase {
  public:
    CommandReaderBase() : V2_3::CommandReaderBase() {}
};

}  // namespace V2_4
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
