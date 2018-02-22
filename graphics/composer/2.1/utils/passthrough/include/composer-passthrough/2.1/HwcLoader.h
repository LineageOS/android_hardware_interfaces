/*
 * Copyright 2016 The Android Open Source Project
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

#include <memory>

#include <composer-hal/2.1/Composer.h>
#include <composer-passthrough/2.1/HwcHal.h>
#include <hardware/fb.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware/hwcomposer2.h>
#include <hwc2on1adapter/HWC2On1Adapter.h>
#include <hwc2onfbadapter/HWC2OnFbAdapter.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace passthrough {

class HwcLoader {
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

        return createComposer(std::move(hal));
    }

    // load hwcomposer2 module
    static const hw_module_t* loadModule() {
        const hw_module_t* module;
        int error = hw_get_module(HWC_HARDWARE_MODULE_ID, &module);
        if (error) {
            ALOGI("falling back to gralloc module");
            error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        }

        if (error) {
            ALOGE("failed to get hwcomposer or gralloc module");
            return nullptr;
        }

        return module;
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
    static IComposer* createComposer(std::unique_ptr<hal::ComposerHal> hal) {
        return hal::Composer::create(std::move(hal)).release();
    }

   protected:
    // open hwcomposer2 device, install an adapter if necessary
    static hwc2_device_t* openDeviceWithAdapter(const hw_module_t* module, bool* outAdapted) {
        if (module->id && std::string(module->id) == GRALLOC_HARDWARE_MODULE_ID) {
            *outAdapted = true;
            return adaptGrallocModule(module);
        }

        hw_device_t* device;
        int error = module->methods->open(module, HWC_HARDWARE_COMPOSER, &device);
        if (error) {
            ALOGE("failed to open hwcomposer device: %s", strerror(-error));
            return nullptr;
        }

        int major = (device->version >> 24) & 0xf;
        if (major != 2) {
            *outAdapted = true;
            return adaptHwc1Device(std::move(reinterpret_cast<hwc_composer_device_1*>(device)));
        }

        *outAdapted = false;
        return reinterpret_cast<hwc2_device_t*>(device);
    }

   private:
    static hwc2_device_t* adaptGrallocModule(const hw_module_t* module) {
        framebuffer_device_t* device;
        int error = framebuffer_open(module, &device);
        if (error) {
            ALOGE("failed to open framebuffer device: %s", strerror(-error));
            return nullptr;
        }

        return new HWC2OnFbAdapter(device);
    }

    static hwc2_device_t* adaptHwc1Device(hwc_composer_device_1* device) {
        int minor = (device->common.version >> 16) & 0xf;
        if (minor < 1) {
            ALOGE("hwcomposer 1.0 is not supported");
            device->common.close(&device->common);
            return nullptr;
        }

        return new HWC2On1Adapter(device);
    }
};

}  // namespace passthrough
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
