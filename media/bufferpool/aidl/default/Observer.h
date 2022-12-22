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

#include <map>
#include <memory>
#include <mutex>
#include <aidl/android/hardware/media/bufferpool2/BnObserver.h>
#include <bufferpool2/BufferPoolTypes.h>

namespace aidl::android::hardware::media::bufferpool2::implementation {

class BufferPoolClient;

struct Observer : public BnObserver {
    ::ndk::ScopedAStatus onMessage(int64_t in_connectionId, int32_t in_msgId) override;

    ~Observer();

    void addClient(ConnectionId connectionId,
                   const std::weak_ptr<BufferPoolClient> &wclient);

    void delClient(ConnectionId connectionId);

private:
    Observer();

    friend class ::ndk::SharedRefBase;

    std::mutex mLock;
    std::map<ConnectionId, const std::weak_ptr<BufferPoolClient>> mClients;
};

}  // namespace aidl::android::hardware::media::bufferpool2::implementation

