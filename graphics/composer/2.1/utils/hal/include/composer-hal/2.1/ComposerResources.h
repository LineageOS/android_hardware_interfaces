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
#warning "ComposerResources.h included without LOG_TAG"
#endif

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

// wrapper for IMapper to import buffers and sideband streams
class ComposerHandleImporter {
   public:
    bool init() {
        mMapper = mapper::V2_0::IMapper::getService();
        ALOGE_IF(!mMapper, "failed to get mapper service");
        return mMapper != nullptr;
    }

    Error importBuffer(const native_handle_t* rawHandle, const native_handle_t** outBufferHandle) {
        if (!rawHandle || (!rawHandle->numFds && !rawHandle->numInts)) {
            *outBufferHandle = nullptr;
            return Error::NONE;
        }

        mapper::V2_0::Error error;
        const native_handle_t* bufferHandle;
        mMapper->importBuffer(rawHandle, [&](const auto& tmpError, const auto& tmpBufferHandle) {
            error = tmpError;
            bufferHandle = static_cast<const native_handle_t*>(tmpBufferHandle);
        });
        if (error != mapper::V2_0::Error::NONE) {
            return Error::NO_RESOURCES;
        }

        *outBufferHandle = bufferHandle;
        return Error::NONE;
    }

    void freeBuffer(const native_handle_t* bufferHandle) {
        if (bufferHandle) {
            mMapper->freeBuffer(static_cast<void*>(const_cast<native_handle_t*>(bufferHandle)));
        }
    }

    Error importStream(const native_handle_t* rawHandle, const native_handle_t** outStreamHandle) {
        const native_handle_t* streamHandle = nullptr;
        if (rawHandle) {
            streamHandle = native_handle_clone(rawHandle);
            if (!streamHandle) {
                return Error::NO_RESOURCES;
            }
        }

        *outStreamHandle = streamHandle;
        return Error::NONE;
    }

    void freeStream(const native_handle_t* streamHandle) {
        if (streamHandle) {
            native_handle_close(streamHandle);
            native_handle_delete(const_cast<native_handle_t*>(streamHandle));
        }
    }

   private:
    sp<mapper::V2_0::IMapper> mMapper;
};

class ComposerHandleCache {
   public:
    enum class HandleType {
        INVALID,
        BUFFER,
        STREAM,
    };

    ComposerHandleCache(ComposerHandleImporter& importer, HandleType type, uint32_t cacheSize)
        : mImporter(importer), mHandleType(type), mHandles(cacheSize, nullptr) {}

    // must be initialized later with initCache
    ComposerHandleCache(ComposerHandleImporter& importer) : mImporter(importer) {}

    ~ComposerHandleCache() {
        switch (mHandleType) {
            case HandleType::BUFFER:
                for (auto handle : mHandles) {
                    mImporter.freeBuffer(handle);
                }
                break;
            case HandleType::STREAM:
                for (auto handle : mHandles) {
                    mImporter.freeStream(handle);
                }
                break;
            default:
                break;
        }
    }

    ComposerHandleCache(const ComposerHandleCache&) = delete;
    ComposerHandleCache& operator=(const ComposerHandleCache&) = delete;

    bool initCache(HandleType type, uint32_t cacheSize) {
        // already initialized
        if (mHandleType != HandleType::INVALID) {
            return false;
        }

        mHandleType = type;
        mHandles.resize(cacheSize, nullptr);

        return true;
    }

    Error lookupCache(uint32_t slot, const native_handle_t** outHandle) {
        if (slot < mHandles.size()) {
            *outHandle = mHandles[slot];
            return Error::NONE;
        } else {
            return Error::BAD_PARAMETER;
        }
    }

    Error updateCache(uint32_t slot, const native_handle_t* handle,
                      const native_handle** outReplacedHandle) {
        if (slot < mHandles.size()) {
            auto& cachedHandle = mHandles[slot];
            *outReplacedHandle = cachedHandle;
            cachedHandle = handle;
            return Error::NONE;
        } else {
            return Error::BAD_PARAMETER;
        }
    }

    // when fromCache is true, look up in the cache; otherwise, update the cache
    Error getHandle(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                    const native_handle_t** outHandle, const native_handle** outReplacedHandle) {
        if (fromCache) {
            *outReplacedHandle = nullptr;
            return lookupCache(slot, outHandle);
        } else {
            *outHandle = inHandle;
            return updateCache(slot, inHandle, outReplacedHandle);
        }
    }

   private:
    ComposerHandleImporter& mImporter;
    HandleType mHandleType = HandleType::INVALID;
    std::vector<const native_handle_t*> mHandles;
};

// layer resource
class ComposerLayerResource {
   public:
    ComposerLayerResource(ComposerHandleImporter& importer, uint32_t bufferCacheSize)
        : mBufferCache(importer, ComposerHandleCache::HandleType::BUFFER, bufferCacheSize),
          mSidebandStreamCache(importer, ComposerHandleCache::HandleType::STREAM, 1) {}

    Error getBuffer(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                    const native_handle_t** outHandle, const native_handle** outReplacedHandle) {
        return mBufferCache.getHandle(slot, fromCache, inHandle, outHandle, outReplacedHandle);
    }

    Error getSidebandStream(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                            const native_handle_t** outHandle,
                            const native_handle** outReplacedHandle) {
        return mSidebandStreamCache.getHandle(slot, fromCache, inHandle, outHandle,
                                              outReplacedHandle);
    }

   protected:
    ComposerHandleCache mBufferCache;
    ComposerHandleCache mSidebandStreamCache;
};

// display resource
class ComposerDisplayResource {
   public:
    enum class DisplayType {
        PHYSICAL,
        VIRTUAL,
    };

    ComposerDisplayResource(DisplayType type, ComposerHandleImporter& importer,
                            uint32_t outputBufferCacheSize)
        : mType(type),
          mClientTargetCache(importer),
          mOutputBufferCache(importer, ComposerHandleCache::HandleType::BUFFER,
                             outputBufferCacheSize),
          mMustValidate(true) {}

    bool initClientTargetCache(uint32_t cacheSize) {
        return mClientTargetCache.initCache(ComposerHandleCache::HandleType::BUFFER, cacheSize);
    }

    bool isVirtual() const { return mType == DisplayType::VIRTUAL; }

    Error getClientTarget(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                          const native_handle_t** outHandle,
                          const native_handle** outReplacedHandle) {
        return mClientTargetCache.getHandle(slot, fromCache, inHandle, outHandle,
                                            outReplacedHandle);
    }

    Error getOutputBuffer(uint32_t slot, bool fromCache, const native_handle_t* inHandle,
                          const native_handle_t** outHandle,
                          const native_handle** outReplacedHandle) {
        return mOutputBufferCache.getHandle(slot, fromCache, inHandle, outHandle,
                                            outReplacedHandle);
    }

    bool addLayer(Layer layer, std::unique_ptr<ComposerLayerResource> layerResource) {
        auto result = mLayerResources.emplace(layer, std::move(layerResource));
        return result.second;
    }

    bool removeLayer(Layer layer) { return mLayerResources.erase(layer) > 0; }

    ComposerLayerResource* findLayerResource(Layer layer) {
        auto layerIter = mLayerResources.find(layer);
        if (layerIter == mLayerResources.end()) {
            return nullptr;
        }

        return layerIter->second.get();
    }

    std::vector<Layer> getLayers() const {
        std::vector<Layer> layers;
        layers.reserve(mLayerResources.size());
        for (const auto& layerKey : mLayerResources) {
            layers.push_back(layerKey.first);
        }
        return layers;
    }

    void setMustValidateState(bool mustValidate) { mMustValidate = mustValidate; }

    bool mustValidate() const { return mMustValidate; }

   protected:
    const DisplayType mType;
    ComposerHandleCache mClientTargetCache;
    ComposerHandleCache mOutputBufferCache;
    bool mMustValidate;

    std::unordered_map<Layer, std::unique_ptr<ComposerLayerResource>> mLayerResources;
};

class ComposerResources {
   private:
    template <bool isBuffer>
    class ReplacedHandle;

   public:
    static std::unique_ptr<ComposerResources> create() {
        auto resources = std::make_unique<ComposerResources>();
        return resources->init() ? std::move(resources) : nullptr;
    }

    ComposerResources() = default;
    virtual ~ComposerResources() = default;

    bool init() { return mImporter.init(); }

    using RemoveDisplay =
        std::function<void(Display display, bool isVirtual, const std::vector<Layer>& layers)>;
    void clear(RemoveDisplay removeDisplay) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        for (const auto& displayKey : mDisplayResources) {
            Display display = displayKey.first;
            const ComposerDisplayResource& displayResource = *displayKey.second;
            removeDisplay(display, displayResource.isVirtual(), displayResource.getLayers());
        }
        mDisplayResources.clear();
    }

    Error addPhysicalDisplay(Display display) {
        auto displayResource =
            createDisplayResource(ComposerDisplayResource::DisplayType::PHYSICAL, 0);

        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        auto result = mDisplayResources.emplace(display, std::move(displayResource));
        return result.second ? Error::NONE : Error::BAD_DISPLAY;
    }

    Error addVirtualDisplay(Display display, uint32_t outputBufferCacheSize) {
        auto displayResource = createDisplayResource(ComposerDisplayResource::DisplayType::VIRTUAL,
                                                     outputBufferCacheSize);

        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        auto result = mDisplayResources.emplace(display, std::move(displayResource));
        return result.second ? Error::NONE : Error::BAD_DISPLAY;
    }

    Error removeDisplay(Display display) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        return mDisplayResources.erase(display) > 0 ? Error::NONE : Error::BAD_DISPLAY;
    }

    Error setDisplayClientTargetCacheSize(Display display, uint32_t clientTargetCacheSize) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
        if (!displayResource) {
            return Error::BAD_DISPLAY;
        }

        return displayResource->initClientTargetCache(clientTargetCacheSize) ? Error::NONE
                                                                             : Error::BAD_PARAMETER;
    }

    Error addLayer(Display display, Layer layer, uint32_t bufferCacheSize) {
        auto layerResource = createLayerResource(bufferCacheSize);

        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
        if (!displayResource) {
            return Error::BAD_DISPLAY;
        }

        return displayResource->addLayer(layer, std::move(layerResource)) ? Error::NONE
                                                                          : Error::BAD_LAYER;
    }

    Error removeLayer(Display display, Layer layer) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
        if (!displayResource) {
            return Error::BAD_DISPLAY;
        }

        return displayResource->removeLayer(layer) ? Error::NONE : Error::BAD_LAYER;
    }

    using ReplacedBufferHandle = ReplacedHandle<true>;
    using ReplacedStreamHandle = ReplacedHandle<false>;

    Error getDisplayClientTarget(Display display, uint32_t slot, bool fromCache,
                                 const native_handle_t* rawHandle,
                                 const native_handle_t** outBufferHandle,
                                 ReplacedBufferHandle* outReplacedBuffer) {
        return getHandle<Cache::CLIENT_TARGET>(display, 0, slot, fromCache, rawHandle,
                                               outBufferHandle, outReplacedBuffer);
    }

    Error getDisplayOutputBuffer(Display display, uint32_t slot, bool fromCache,
                                 const native_handle_t* rawHandle,
                                 const native_handle_t** outBufferHandle,
                                 ReplacedBufferHandle* outReplacedBuffer) {
        return getHandle<Cache::OUTPUT_BUFFER>(display, 0, slot, fromCache, rawHandle,
                                               outBufferHandle, outReplacedBuffer);
    }

    Error getLayerBuffer(Display display, Layer layer, uint32_t slot, bool fromCache,
                         const native_handle_t* rawHandle, const native_handle_t** outBufferHandle,
                         ReplacedBufferHandle* outReplacedBuffer) {
        return getHandle<Cache::LAYER_BUFFER>(display, layer, slot, fromCache, rawHandle,
                                              outBufferHandle, outReplacedBuffer);
    }

    Error getLayerSidebandStream(Display display, Layer layer, const native_handle_t* rawHandle,
                                 const native_handle_t** outStreamHandle,
                                 ReplacedStreamHandle* outReplacedStream) {
        return getHandle<Cache::LAYER_SIDEBAND_STREAM>(display, layer, 0, false, rawHandle,
                                                       outStreamHandle, outReplacedStream);
    }

    void setDisplayMustValidateState(Display display, bool mustValidate) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        auto* displayResource = findDisplayResourceLocked(display);
        if (displayResource) {
            displayResource->setMustValidateState(mustValidate);
        }
    }

    bool mustValidateDisplay(Display display) {
        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);
        auto* displayResource = findDisplayResourceLocked(display);
        if (displayResource) {
            return displayResource->mustValidate();
        }
        return false;
    }

   protected:
    virtual std::unique_ptr<ComposerDisplayResource> createDisplayResource(
        ComposerDisplayResource::DisplayType type, uint32_t outputBufferCacheSize) {
        return std::make_unique<ComposerDisplayResource>(type, mImporter, outputBufferCacheSize);
    }

    virtual std::unique_ptr<ComposerLayerResource> createLayerResource(uint32_t bufferCacheSize) {
        return std::make_unique<ComposerLayerResource>(mImporter, bufferCacheSize);
    }

    ComposerDisplayResource* findDisplayResourceLocked(Display display) {
        auto iter = mDisplayResources.find(display);
        if (iter == mDisplayResources.end()) {
            return nullptr;
        }
        return iter->second.get();
    }

    ComposerHandleImporter mImporter;

    std::mutex mDisplayResourcesMutex;
    std::unordered_map<Display, std::unique_ptr<ComposerDisplayResource>> mDisplayResources;

   private:
    enum class Cache {
        CLIENT_TARGET,
        OUTPUT_BUFFER,
        LAYER_BUFFER,
        LAYER_SIDEBAND_STREAM,
    };

    // When a buffer in the cache is replaced by a new one, we must keep it
    // alive until it has been replaced in ComposerHal.
    template <bool isBuffer>
    class ReplacedHandle {
       public:
        ReplacedHandle() = default;
        ReplacedHandle(const ReplacedHandle&) = delete;
        ReplacedHandle& operator=(const ReplacedHandle&) = delete;

        ~ReplacedHandle() { reset(); }

        void reset(ComposerHandleImporter* importer = nullptr,
                   const native_handle_t* handle = nullptr) {
            if (mHandle) {
                if (isBuffer) {
                    mImporter->freeBuffer(mHandle);
                } else {
                    mImporter->freeStream(mHandle);
                }
            }

            mImporter = importer;
            mHandle = handle;
        }

       private:
        ComposerHandleImporter* mImporter = nullptr;
        const native_handle_t* mHandle = nullptr;
    };

    template <Cache cache, bool isBuffer>
    Error getHandle(Display display, Layer layer, uint32_t slot, bool fromCache,
                    const native_handle_t* rawHandle, const native_handle_t** outHandle,
                    ReplacedHandle<isBuffer>* outReplacedHandle) {
        Error error;

        // import the raw handle (or ignore raw handle when fromCache is true)
        const native_handle_t* importedHandle = nullptr;
        if (!fromCache) {
            error = (isBuffer) ? mImporter.importBuffer(rawHandle, &importedHandle)
                               : mImporter.importStream(rawHandle, &importedHandle);
            if (error != Error::NONE) {
                return error;
            }
        }

        std::lock_guard<std::mutex> lock(mDisplayResourcesMutex);

        // find display/layer resource
        const bool needLayerResource =
            (cache == Cache::LAYER_BUFFER || cache == Cache::LAYER_SIDEBAND_STREAM);
        ComposerDisplayResource* displayResource = findDisplayResourceLocked(display);
        ComposerLayerResource* layerResource = (displayResource && needLayerResource)
                                                   ? displayResource->findLayerResource(layer)
                                                   : nullptr;

        // lookup or update cache
        const native_handle_t* replacedHandle = nullptr;
        if (displayResource && (!needLayerResource || layerResource)) {
            switch (cache) {
                case Cache::CLIENT_TARGET:
                    error = displayResource->getClientTarget(slot, fromCache, importedHandle,
                                                             outHandle, &replacedHandle);
                    break;
                case Cache::OUTPUT_BUFFER:
                    error = displayResource->getOutputBuffer(slot, fromCache, importedHandle,
                                                             outHandle, &replacedHandle);
                    break;
                case Cache::LAYER_BUFFER:
                    error = layerResource->getBuffer(slot, fromCache, importedHandle, outHandle,
                                                     &replacedHandle);
                    break;
                case Cache::LAYER_SIDEBAND_STREAM:
                    error = layerResource->getSidebandStream(slot, fromCache, importedHandle,
                                                             outHandle, &replacedHandle);
                    break;
                default:
                    error = Error::BAD_PARAMETER;
                    break;
            }

            if (error != Error::NONE) {
                ALOGW("invalid cache %d slot %d", int(cache), int(slot));
            }
        } else if (!displayResource) {
            error = Error::BAD_DISPLAY;
        } else {
            error = Error::BAD_LAYER;
        }

        // clean up on errors
        if (error != Error::NONE) {
            if (!fromCache) {
                if (isBuffer) {
                    mImporter.freeBuffer(importedHandle);
                } else {
                    mImporter.freeStream(importedHandle);
                }
            }
            return error;
        }

        outReplacedHandle->reset(&mImporter, replacedHandle);

        return Error::NONE;
    }
};

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
