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

#ifndef LOG_TAG
#warning "GrallocLoader.h included without LOG_TAG"
#endif

#include <memory>
#include <mutex>
#include <unordered_set>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <mapper-hal/2.0/Mapper.h>
#include <mapper-passthrough/2.0/Gralloc0Hal.h>
#include <mapper-passthrough/2.0/Gralloc1Hal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

class GrallocImportedBufferPool {
   public:
    static GrallocImportedBufferPool& getInstance() {
        // GraphicBufferMapper in framework is expected to be valid (and
        // leaked) during process termination.  We need to make sure IMapper,
        // and in turn, GrallocImportedBufferPool is valid as well.  Create
        // imported buffer pool on the heap (and let it leak) for the purpose.
        // Besides, all IMapper instances must share the same pool.  Make it a
        // singleton.
        //
        // However, there is no way to make sure gralloc0/gralloc1 are valid
        // during process termination.  Any use of static/global object in
        // gralloc0/gralloc1 that may be destructed during process termination
        // is potentially broken.
        static GrallocImportedBufferPool* singleton = new GrallocImportedBufferPool;
        return *singleton;
    }

    void* add(native_handle_t* bufferHandle) {
        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.insert(bufferHandle).second ? bufferHandle : nullptr;
    }

    native_handle_t* remove(void* buffer) {
        auto bufferHandle = static_cast<native_handle_t*>(buffer);

        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.erase(bufferHandle) == 1 ? bufferHandle : nullptr;
    }

    const native_handle_t* get(void* buffer) {
        auto bufferHandle = static_cast<const native_handle_t*>(buffer);

        std::lock_guard<std::mutex> lock(mMutex);
        return mBufferHandles.count(bufferHandle) == 1 ? bufferHandle : nullptr;
    }

   private:
    std::mutex mMutex;
    std::unordered_set<const native_handle_t*> mBufferHandles;
};

// Inherit from V2_*::hal::Mapper and override imported buffer management functions
template <typename T>
class GrallocMapper : public T {
   protected:
    void* addImportedBuffer(native_handle_t* bufferHandle) override {
        return GrallocImportedBufferPool::getInstance().add(bufferHandle);
    }

    native_handle_t* removeImportedBuffer(void* buffer) override {
        return GrallocImportedBufferPool::getInstance().remove(buffer);
    }

    const native_handle_t* getImportedBuffer(void* buffer) const override {
        return GrallocImportedBufferPool::getInstance().get(buffer);
    }
};

class GrallocLoader {
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

    // load the gralloc module
    static const hw_module_t* loadModule() {
        const hw_module_t* module;
        int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (error) {
            ALOGE("failed to get gralloc module");
            return nullptr;
        }

        return module;
    }

    // return the major api version of the module
    static int getModuleMajorApiVersion(const hw_module_t* module) {
        return (module->module_api_version >> 8) & 0xff;
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
        auto mapper = std::make_unique<GrallocMapper<hal::Mapper>>();
        return mapper->init(std::move(hal)) ? mapper.release() : nullptr;
    }
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
