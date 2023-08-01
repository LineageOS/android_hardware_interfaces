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

#include <memory>

#include <aidl/android/hardware/media/bufferpool2/BnConnection.h>
#include <bufferpool2/BufferPoolTypes.h>

namespace aidl::android::hardware::media::bufferpool2::implementation {

struct Accessor;

struct Connection : public BnConnection {
    // Methods from ::aidl::android::hardware::media::bufferpool2::IConnection.
    ::ndk::ScopedAStatus fetch(const std::vector<::aidl::android::hardware::media::bufferpool2::IConnection::FetchInfo>& in_fetchInfos, std::vector<::aidl::android::hardware::media::bufferpool2::IConnection::FetchResult>* _aidl_return) override;

    // Methods from ::aidl::android::hardware::media::bufferpool2::IConnection.
    ::ndk::ScopedAStatus sync() override;

    /**
     * Invalidates all buffers which are active and/or are ready to be recycled.
     */
    BufferPoolStatus flush();

    /**
     * Allocates a buffer using the specified parameters. Recycles a buffer if
     * it is possible. The returned buffer can be transferred to other remote
     * clients(Connection).
     *
     * @param params    allocation parameters.
     * @param bufferId  Id of the allocated buffer.
     * @param handle    native handle of the allocated buffer.
     *
     * @return OK if a buffer is successfully allocated.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus allocate(const std::vector<uint8_t> &params,
                          BufferId *bufferId, const native_handle_t **handle);

    /**
     * Processes pending buffer status messages and performs periodic cache cleaning
     * from bufferpool.
     *
     * @param clearCache    if clearCache is true, bufferpool frees all buffers
     *                      waiting to be recycled.
     */
    void cleanUp(bool clearCache);

    /** Destructs a connection. */
    ~Connection();

    /** Creates a connection. */
    Connection();

    /**
     * Initializes with the specified buffer pool and the connection id.
     * The connection id should be unique in the whole system.
     *
     * @param accessor      the specified buffer pool.
     * @param connectionId  Id.
     */
    void initialize(const std::shared_ptr<Accessor> &accessor, ConnectionId connectionId);

    enum : uint32_t {
        SYNC_BUFFERID = UINT32_MAX,
    };

private:
    bool mInitialized;
    std::shared_ptr<Accessor> mAccessor;
    ConnectionId mConnectionId;

    bool fetch(
        uint64_t transactionId,
        uint32_t bufferId,
        std::vector<::aidl::android::hardware::media::bufferpool2::IConnection::FetchResult>
                *result);
};

}  // namespace aidl::android::hardware::media::bufferpool2::implementation
