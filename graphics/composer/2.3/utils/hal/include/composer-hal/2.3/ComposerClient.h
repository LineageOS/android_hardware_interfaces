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
#warning "ComposerClient.h included without LOG_TAG"
#endif

#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#include <composer-hal/2.3/ComposerClient.h>
#include <composer-hal/2.3/ComposerCommandEngine.h>
#include <composer-hal/2.3/ComposerHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace hal {

namespace detail {

// ComposerClientImpl implements V2_*::IComposerClient on top of V2_*::ComposerHal
template <typename Interface, typename Hal>
class ComposerClientImpl : public V2_2::hal::detail::ComposerClientImpl<Interface, Hal> {
   public:
    static std::unique_ptr<ComposerClientImpl> create(Hal* hal) {
        auto client = std::make_unique<ComposerClientImpl>(hal);
        return client->init() ? std::move(client) : nullptr;
    }

    ComposerClientImpl(Hal* hal) : BaseType2_2(hal) {}

    // IComposerClient 2.3 interface

    Return<void> getDisplayIdentificationData(
        Display display, IComposerClient::getDisplayIdentificationData_cb hidl_cb) override {
        uint8_t port = 0;
        std::vector<uint8_t> data;
        Error error = mHal->getDisplayIdentificationData(display, &port, &data);
        hidl_cb(error, port, data);
        return Void();
    }

    Return<void> executeCommands_2_3(uint32_t inLength, const hidl_vec<hidl_handle>& inHandles,
                                     IComposerClient::executeCommands_2_2_cb hidl_cb) override {
        std::lock_guard<std::mutex> lock(mCommandEngineMutex);
        bool outChanged = false;
        uint32_t outLength = 0;
        hidl_vec<hidl_handle> outHandles;
        Error error =
            mCommandEngine->execute(inLength, inHandles, &outChanged, &outLength, &outHandles);

        hidl_cb(error, outChanged, outLength, outHandles);

        mCommandEngine->reset();

        return Void();
    }

   private:
    using BaseType2_2 = V2_2::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1 = V2_1::hal::detail::ComposerClientImpl<Interface, Hal>;
    using BaseType2_1::mCommandEngine;
    using BaseType2_1::mCommandEngineMutex;
    using BaseType2_1::mHal;
};

}  // namespace detail

using ComposerClient = detail::ComposerClientImpl<IComposerClient, ComposerHal>;

}  // namespace hal
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
