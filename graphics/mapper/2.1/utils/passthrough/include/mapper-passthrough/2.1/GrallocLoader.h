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

#include <log/log.h>
#include <mapper-hal/2.1/Mapper.h>
#include <mapper-hal/2.1/MapperHal.h>
#include <mapper-passthrough/2.0/GrallocLoader.h>
#include <mapper-passthrough/2.1/Gralloc0Hal.h>
#include <mapper-passthrough/2.1/Gralloc1Hal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace passthrough {

class GrallocLoader : public V2_0::passthrough::GrallocLoader {
   public:
    static IMapper* load() {
        const hw_module_t* module = loadModule();
        if (!module) {
            return nullptr;
        }
        auto hal = createHal(module);
        if (!hal) {
            return nullptr;
        }
        return createMapper(std::move(hal));
    }

    // create a MapperHal instance
    static std::unique_ptr<hal::MapperHal> createHal(const hw_module_t* module) {
        int major = getModuleMajorApiVersion(module);
        switch (major) {
            case 1: {
                auto hal = std::make_unique<Gralloc1Hal>();
                return hal->initWithModule(module) ? std::move(hal) : nullptr;
            }
            case 0: {
                auto hal = std::make_unique<Gralloc0Hal>();
                return hal->initWithModule(module) ? std::move(hal) : nullptr;
            }
            default:
                ALOGE("unknown gralloc module major version %d", major);
                return nullptr;
        }
    }

    // create an IAllocator instance
    static IMapper* createMapper(std::unique_ptr<hal::MapperHal> hal) {
        auto mapper = std::make_unique<V2_0::passthrough::GrallocMapper<hal::Mapper>>();
        return mapper->init(std::move(hal)) ? mapper.release() : nullptr;
    }
};

}  // namespace passthrough
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
