/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <android-base/unique_fd.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <log/log.h>

#include <mutex>
#include <optional>
#include <type_traits>

/**
 * Helper utilities for providing an IMapper-StableC implementation.
 */

namespace vendor::mapper {

/**
 * Extend from this interface to provide Version 5 of the IMapper interface
 */
struct IMapperV5Impl {
    static const auto version = AIMAPPER_VERSION_5;
    virtual ~IMapperV5Impl() = default;

    virtual AIMapper_Error importBuffer(const native_handle_t* _Nonnull handle,
                                        buffer_handle_t _Nullable* _Nonnull outBufferHandle) = 0;

    virtual AIMapper_Error freeBuffer(buffer_handle_t _Nonnull buffer) = 0;

    virtual AIMapper_Error getTransportSize(buffer_handle_t _Nonnull buffer,
                                            uint32_t* _Nonnull outNumFds,
                                            uint32_t* _Nonnull outNumInts) = 0;

    virtual AIMapper_Error lock(buffer_handle_t _Nonnull buffer, uint64_t cpuUsage,
                                ARect accessRegion, int acquireFence,
                                void* _Nullable* _Nonnull outData) = 0;

    virtual AIMapper_Error unlock(buffer_handle_t _Nonnull buffer, int* _Nonnull releaseFence) = 0;

    virtual AIMapper_Error flushLockedBuffer(buffer_handle_t _Nonnull buffer) = 0;

    virtual AIMapper_Error rereadLockedBuffer(buffer_handle_t _Nonnull buffer) = 0;

    virtual int32_t getMetadata(buffer_handle_t _Nonnull buffer, AIMapper_MetadataType metadataType,
                                void* _Nullable destBuffer, size_t destBufferSize) = 0;

    virtual int32_t getStandardMetadata(buffer_handle_t _Nonnull buffer,
                                        int64_t standardMetadataType, void* _Nullable destBuffer,
                                        size_t destBufferSize) = 0;

    virtual AIMapper_Error setMetadata(buffer_handle_t _Nonnull buffer,
                                       AIMapper_MetadataType metadataType,
                                       const void* _Nonnull metadata, size_t metadataSize) = 0;

    virtual AIMapper_Error setStandardMetadata(buffer_handle_t _Nonnull buffer,
                                               int64_t standardMetadataType,
                                               const void* _Nonnull metadata,
                                               size_t metadataSize) = 0;

    virtual AIMapper_Error listSupportedMetadataTypes(
            const AIMapper_MetadataTypeDescription* _Nullable* _Nonnull outDescriptionList,
            size_t* _Nonnull outNumberOfDescriptions) = 0;

    virtual AIMapper_Error dumpBuffer(buffer_handle_t _Nonnull bufferHandle,
                                      AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                                      void* _Null_unspecified context) = 0;

    virtual AIMapper_Error dumpAllBuffers(
            AIMapper_BeginDumpBufferCallback _Nonnull beginDumpBufferCallback,
            AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
            void* _Null_unspecified context) = 0;

    virtual AIMapper_Error getReservedRegion(buffer_handle_t _Nonnull buffer,
                                             void* _Nullable* _Nonnull outReservedRegion,
                                             uint64_t* _Nonnull outReservedSize) = 0;
};

namespace provider {
#ifndef __cpp_inline_variables
#error "Only C++17 & newer is supported; inline variables is missing"
#endif

inline void* _Nullable sIMapperInstance = nullptr;
}  // namespace provider

template <typename IMPL>
class IMapperProvider {
  private:
    static_assert(IMPL::version >= AIMAPPER_VERSION_5, "Must be at least AIMAPPER_VERSION_5");
    static_assert(std::is_final_v<IMPL>, "Implementation must be final");
    static_assert(std::is_constructible_v<IMPL>, "Implementation must have a no-args constructor");

    std::once_flag mLoadOnceFlag;
    IMPL* _Nullable mImpl;
    AIMapper* _Nullable mMapper;

    static IMPL& impl() {
        return *reinterpret_cast<IMapperProvider<IMPL>*>(provider::sIMapperInstance)->mImpl;
    }

    void bindV5() {
        mMapper->v5 = {
                .importBuffer = [](const native_handle_t* _Nonnull handle,
                                   buffer_handle_t _Nullable* _Nonnull outBufferHandle)
                        -> AIMapper_Error { return impl().importBuffer(handle, outBufferHandle); },

                .freeBuffer = [](buffer_handle_t _Nonnull buffer) -> AIMapper_Error {
                    return impl().freeBuffer(buffer);
                },

                .getTransportSize = [](buffer_handle_t _Nonnull buffer,
                                       uint32_t* _Nonnull outNumFds,
                                       uint32_t* _Nonnull outNumInts) -> AIMapper_Error {
                    return impl().getTransportSize(buffer, outNumFds, outNumInts);
                },

                .lock = [](buffer_handle_t _Nonnull buffer, uint64_t cpuUsage, ARect accessRegion,
                           int acquireFence, void* _Nullable* _Nonnull outData) -> AIMapper_Error {
                    return impl().lock(buffer, cpuUsage, accessRegion, acquireFence, outData);
                },

                .unlock = [](buffer_handle_t _Nonnull buffer, int* _Nonnull releaseFence)
                        -> AIMapper_Error { return impl().unlock(buffer, releaseFence); },

                .flushLockedBuffer = [](buffer_handle_t _Nonnull buffer) -> AIMapper_Error {
                    return impl().flushLockedBuffer(buffer);
                },

                .rereadLockedBuffer = [](buffer_handle_t _Nonnull buffer) -> AIMapper_Error {
                    return impl().rereadLockedBuffer(buffer);
                },

                .getMetadata = [](buffer_handle_t _Nonnull buffer,
                                  AIMapper_MetadataType metadataType, void* _Nullable destBuffer,
                                  size_t destBufferSize) -> int32_t {
                    return impl().getMetadata(buffer, metadataType, destBuffer, destBufferSize);
                },

                .getStandardMetadata = [](buffer_handle_t _Nonnull buffer,
                                          int64_t standardMetadataType, void* _Nullable destBuffer,
                                          size_t destBufferSize) -> int32_t {
                    return impl().getStandardMetadata(buffer, standardMetadataType, destBuffer,
                                                      destBufferSize);
                },

                .setMetadata = [](buffer_handle_t _Nonnull buffer,
                                  AIMapper_MetadataType metadataType, const void* _Nonnull metadata,
                                  size_t metadataSize) -> AIMapper_Error {
                    return impl().setMetadata(buffer, metadataType, metadata, metadataSize);
                },

                .setStandardMetadata =
                        [](buffer_handle_t _Nonnull buffer, int64_t standardMetadataType,
                           const void* _Nonnull metadata, size_t metadataSize) -> AIMapper_Error {
                    return impl().setStandardMetadata(buffer, standardMetadataType, metadata,
                                                      metadataSize);
                },

                .listSupportedMetadataTypes =
                        [](const AIMapper_MetadataTypeDescription* _Nullable* _Nonnull outDescriptionList,
                           size_t* _Nonnull outNumberOfDescriptions) -> AIMapper_Error {
                    return impl().listSupportedMetadataTypes(outDescriptionList,
                                                             outNumberOfDescriptions);
                },

                .dumpBuffer = [](buffer_handle_t _Nonnull bufferHandle,
                                 AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                                 void* _Null_unspecified context) -> AIMapper_Error {
                    return impl().dumpBuffer(bufferHandle, dumpBufferCallback, context);
                },

                .dumpAllBuffers =
                        [](AIMapper_BeginDumpBufferCallback _Nonnull beginDumpBufferCallback,
                           AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                           void* _Null_unspecified context) {
                            return impl().dumpAllBuffers(beginDumpBufferCallback,
                                                         dumpBufferCallback, context);
                        },

                .getReservedRegion = [](buffer_handle_t _Nonnull buffer,
                                        void* _Nullable* _Nonnull outReservedRegion,
                                        uint64_t* _Nonnull outReservedSize) -> AIMapper_Error {
                    return impl().getReservedRegion(buffer, outReservedRegion, outReservedSize);
                },
        };
    }

  public:
    explicit IMapperProvider() = default;

    AIMapper_Error load(AIMapper* _Nullable* _Nonnull outImplementation) {
        std::call_once(mLoadOnceFlag, [this] {
            LOG_ALWAYS_FATAL_IF(provider::sIMapperInstance != nullptr,
                                "AIMapper implementation already loaded!");
            provider::sIMapperInstance = this;
            mImpl = new IMPL();
            mMapper = new AIMapper();
            mMapper->version = IMPL::version;
            if (IMPL::version >= AIMAPPER_VERSION_5) {
                bindV5();
            }
        });
        *outImplementation = mMapper;
        return AIMAPPER_ERROR_NONE;
    }
};

}  // namespace vendor::mapper
