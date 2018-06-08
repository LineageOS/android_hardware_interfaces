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
#warning "HwcLoader.h included without LOG_TAG"
#endif

#include <composer-hal/2.2/Composer.h>
#include <composer-hal/2.2/ComposerHal.h>
#include <composer-passthrough/2.1/HwcLoader.h>
#include <composer-passthrough/2.2/HwcHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_2 {
namespace passthrough {

class HwcLoader : public V2_1::passthrough::HwcLoader {
   public:
    static IComposer* load() {
        const hw_module_t* module = loadModule();
        if (!module) {
            return nullptr;
        }

        auto hal = createHalWithAdapter(module);
        if (!hal) {
            return nullptr;
        }

        return createComposer(std::move(hal)).release();
    }

    // create a ComposerHal instance
    static std::unique_ptr<hal::ComposerHal> createHal(const hw_module_t* module) {
        auto hal = std::make_unique<HwcHal>();
        return hal->initWithModule(module) ? std::move(hal) : nullptr;
    }

    // create a ComposerHal instance, insert an adapter if necessary
    static std::unique_ptr<hal::ComposerHal> createHalWithAdapter(const hw_module_t* module) {
        bool adapted;
        hwc2_device_t* device = openDeviceWithAdapter(module, &adapted);
        if (!device) {
            return nullptr;
        }
        auto hal = std::make_unique<HwcHal>();
        return hal->initWithDevice(std::move(device), !adapted) ? std::move(hal) : nullptr;
    }

    // create an IComposer instance
    static std::unique_ptr<IComposer> createComposer(std::unique_ptr<hal::ComposerHal> hal) {
        return hal::Composer::create(std::move(hal));
    }
};

}  // namespace passthrough
}  // namespace V2_2
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
