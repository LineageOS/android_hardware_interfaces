/*
 * Copyright 2017 The Android Open Source Project
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

#include <allocator-hal/2.0/AllocatorHal.h>

struct hw_module_t;

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

class GrallocLoader {
   public:
    static IAllocator* load() {
        const hw_module_t* module = loadModule();
        if (!module) {
            return nullptr;
        }
        auto hal = createHal(module);
        if (!hal) {
            return nullptr;
        }
        return createAllocator(std::move(hal));
    }

    // load the gralloc module
    static const hw_module_t* loadModule();

    // return the major api version of the module
    static int getModuleMajorApiVersion(const hw_module_t* module);

    // create an AllocatorHal instance
    static std::unique_ptr<hal::AllocatorHal> createHal(const hw_module_t* module);

    // create an IAllocator instance
    static IAllocator* createAllocator(std::unique_ptr<hal::AllocatorHal> hal);
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
