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

#pragma once

#include <memory>
#include <vector>

#include <VtsHalHidlTargetTestBase.h>
#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#include <composer-vts/2.2/ComposerVts.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace vts {

using V2_1::Display;
using V2_3::IComposer;
using V2_3::IComposerClient;

class ComposerClient;

// A wrapper to IComposer.
class Composer : public V2_2::vts::Composer {
   public:
    Composer();
    explicit Composer(const std::string& name);

    std::unique_ptr<ComposerClient> createClient();

   protected:
    explicit Composer(const sp<IComposer>& composer);

   private:
    const sp<IComposer> mComposer;
};

// A wrapper to IComposerClient.
class ComposerClient : public V2_2::vts::ComposerClient {
   public:
    explicit ComposerClient(const sp<IComposerClient>& client)
        : V2_2::vts::ComposerClient(client), mClient(client) {}

    bool getDisplayIdentificationData(Display display, uint8_t* outPort,
                                      std::vector<uint8_t>* outData);

   private:
    const sp<IComposerClient> mClient;
};

}  // namespace vts
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
