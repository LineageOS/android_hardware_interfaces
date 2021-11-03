/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_TRACKER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_TRACKER_H

#include <android-base/macros.h>
#include <android-base/thread_annotations.h>

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stack>
#include <utility>
#include <vector>

#include "nnapi/hal/aidl/HalInterfaces.h"
#include "nnapi/hal/aidl/ValidateHal.h"

namespace android::nn {

// This class manages a CPU buffer allocated on heap and provides validation methods.
class AidlManagedBuffer {
  public:
    static std::shared_ptr<AidlManagedBuffer> create(uint32_t size,
                                                     std::set<AidlHalPreparedModelRole> roles,
                                                     const Operand& operand);

    // Prefer AidlManagedBuffer::create.
    AidlManagedBuffer(std::unique_ptr<uint8_t[]> buffer, uint32_t size,
                      std::set<AidlHalPreparedModelRole> roles, const Operand& operand);

    uint8_t* getPointer() const { return kBuffer.get(); }
    uint32_t getSize() const { return kSize; }

    // "poolIndex" is the index of this buffer in the request.pools.
    ErrorStatus validateRequest(uint32_t poolIndex, const Request& request,
                                const aidl_hal::IPreparedModel* preparedModel) const;

    // "size" is the byte size of the Memory provided to the copyFrom or copyTo method.
    ErrorStatus validateCopyFrom(const std::vector<uint32_t>& dimensions, uint32_t size) const;
    ErrorStatus validateCopyTo(uint32_t size) const;

    bool updateDimensions(const std::vector<uint32_t>& dimensions);
    void setInitialized(bool initialized);

  private:
    mutable std::mutex mMutex;
    const std::unique_ptr<uint8_t[]> kBuffer;
    const uint32_t kSize;
    const std::set<AidlHalPreparedModelRole> kRoles;
    const OperandType kOperandType;
    const std::vector<uint32_t> kInitialDimensions;
    std::vector<uint32_t> mUpdatedDimensions GUARDED_BY(mMutex);
    bool mInitialized GUARDED_BY(mMutex) = false;
};

// Keep track of all AidlManagedBuffers and assign each with a unique token.
class AidlBufferTracker : public std::enable_shared_from_this<AidlBufferTracker> {
    DISALLOW_COPY_AND_ASSIGN(AidlBufferTracker);

  public:
    // A RAII class to help manage the lifetime of the token.
    // It is only supposed to be constructed in AidlBufferTracker::add.
    class Token {
        DISALLOW_COPY_AND_ASSIGN(Token);

      public:
        Token(uint32_t token, std::shared_ptr<AidlBufferTracker> tracker)
            : kToken(token), kBufferTracker(std::move(tracker)) {}
        ~Token() { kBufferTracker->free(kToken); }
        uint32_t get() const { return kToken; }

      private:
        const uint32_t kToken;
        const std::shared_ptr<AidlBufferTracker> kBufferTracker;
    };

    // The factory of AidlBufferTracker. This ensures that the AidlBufferTracker is always managed
    // by a shared_ptr.
    static std::shared_ptr<AidlBufferTracker> create() {
        return std::make_shared<AidlBufferTracker>();
    }

    // Prefer AidlBufferTracker::create.
    AidlBufferTracker() : mTokenToBuffers(1) {}

    std::unique_ptr<Token> add(std::shared_ptr<AidlManagedBuffer> buffer);
    std::shared_ptr<AidlManagedBuffer> get(uint32_t token) const;

  private:
    void free(uint32_t token);

    mutable std::mutex mMutex;
    std::stack<uint32_t, std::vector<uint32_t>> mFreeTokens GUARDED_BY(mMutex);

    // Since the tokens are allocated in a non-sparse way, we use a vector to represent the mapping.
    // The index of the vector is the token. When the token gets freed, the corresponding entry is
    // set to nullptr. mTokenToBuffers[0] is always set to nullptr because 0 is an invalid token.
    std::vector<std::shared_ptr<AidlManagedBuffer>> mTokenToBuffers GUARDED_BY(mMutex);
};

}  // namespace android::nn

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_TRACKER_H
