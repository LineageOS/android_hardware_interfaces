/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <utils/NativeHandle.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

namespace android {

class IGrallocHalWrapper;

// Reference: hardware/interfaces/graphics/mapper/2.0/vts/functional/
class GrallocWrapper {
   public:
    GrallocWrapper();
    ~GrallocWrapper();

    // After constructing this object, this function must be called to check the result. If it
    // returns false, other methods are not safe to call.
    bool isInitialized() const { return (mGrallocHal != nullptr); };

    std::string dumpDebugInfo();

    // Allocates a gralloc buffer suitable for direct channel sensors usage with the given size.
    // The buffer should be freed using freeBuffer when it's not needed anymore; otherwise it'll
    // be freed when this object is destroyed.
    // Returns a handle to the buffer, and a CPU-accessible pointer for reading. On failure, both
    // will be set to nullptr.
    std::pair<native_handle_t*, void*> allocate(uint32_t size);

    // Releases a gralloc buffer previously returned by allocate()
    void freeBuffer(native_handle_t* bufferHandle);

  private:
    std::unique_ptr<IGrallocHalWrapper> mGrallocHal;

    // Keep track of all cloned and imported handles.  When a test fails with
    // ASSERT_*, the destructor will free the handles for the test.
    std::unordered_set<native_handle_t*> mAllocatedBuffers;
};

}  // namespace android
