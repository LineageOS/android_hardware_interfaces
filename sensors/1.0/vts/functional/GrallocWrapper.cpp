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

#define LOG_TAG "GrallocWrapper"

#include "GrallocWrapper.h"

#include <utils/Log.h>

namespace android {

GrallocWrapper::GrallocWrapper() { init(); }

void GrallocWrapper::init() {
  mAllocator = allocator2::IAllocator::getService();
  if (mAllocator == nullptr) {
    ALOGE("Failed to get allocator service");
  }

  mMapper = mapper2::IMapper::getService();
  if (mMapper == nullptr) {
    ALOGE("Failed to get mapper service");
  }
  if (mMapper->isRemote()) {
    ALOGE("Mapper is not in passthrough mode");
  }
}

GrallocWrapper::~GrallocWrapper() {
  for (auto bufferHandle : mClonedBuffers) {
    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    native_handle_close(buffer);
    native_handle_delete(buffer);
  }
  mClonedBuffers.clear();

  for (auto bufferHandle : mImportedBuffers) {
    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    if (mMapper->freeBuffer(buffer) != mapper2::Error::NONE) {
      ALOGE("Failed to free buffer %p", buffer);
    }
  }
  mImportedBuffers.clear();
}

sp<allocator2::IAllocator> GrallocWrapper::getAllocator() const {
  return mAllocator;
}

std::string GrallocWrapper::dumpDebugInfo() {
  std::string debugInfo;
  mAllocator->dumpDebugInfo(
      [&](const auto& tmpDebugInfo) { debugInfo = tmpDebugInfo.c_str(); });

  return debugInfo;
}

const native_handle_t* GrallocWrapper::cloneBuffer(
    const hardware::hidl_handle& rawHandle) {
  const native_handle_t* bufferHandle =
      native_handle_clone(rawHandle.getNativeHandle());

  if (bufferHandle) {
    mClonedBuffers.insert(bufferHandle);
  }
  return bufferHandle;
}

std::vector<const native_handle_t*> GrallocWrapper::allocate(
    const mapper2::BufferDescriptor& descriptor, uint32_t count, bool import,
    uint32_t* outStride) {
  std::vector<const native_handle_t*> bufferHandles;
  bufferHandles.reserve(count);
  mAllocator->allocate(
      descriptor, count,
      [&](const auto& tmpError, const auto& tmpStride, const auto& tmpBuffers) {
        if (mapper2::Error::NONE != tmpError) {
          ALOGE("Failed to allocate buffers");
        }
        if (count != tmpBuffers.size()) {
          ALOGE("Invalid buffer array");
        }

        for (uint32_t i = 0; i < count; i++) {
          if (import) {
            bufferHandles.push_back(importBuffer(tmpBuffers[i]));
          } else {
            bufferHandles.push_back(cloneBuffer(tmpBuffers[i]));
          }
        }

        if (outStride) {
          *outStride = tmpStride;
        }
      });

  return bufferHandles;
}

const native_handle_t* GrallocWrapper::allocate(
    const mapper2::IMapper::BufferDescriptorInfo& descriptorInfo, bool import,
    uint32_t* outStride) {
  mapper2::BufferDescriptor descriptor = createDescriptor(descriptorInfo);
  ALOGE("QQ");
  auto buffers = allocate(descriptor, 1, import, outStride);
  return buffers[0];
}

sp<mapper2::IMapper> GrallocWrapper::getMapper() const { return mMapper; }

mapper2::BufferDescriptor GrallocWrapper::createDescriptor(
    const mapper2::IMapper::BufferDescriptorInfo& descriptorInfo) {
  mapper2::BufferDescriptor descriptor;
  mMapper->createDescriptor(
      descriptorInfo, [&](const auto& tmpError, const auto& tmpDescriptor) {
        if (tmpError != mapper2::Error::NONE) {
          ALOGE("Failed to create descriptor");
        }
        descriptor = tmpDescriptor;
      });

  return descriptor;
}

const native_handle_t* GrallocWrapper::importBuffer(
    const hardware::hidl_handle& rawHandle) {
  const native_handle_t* bufferHandle = nullptr;
  mMapper->importBuffer(
      rawHandle, [&](const auto& tmpError, const auto& tmpBuffer) {
        if (tmpError != mapper2::Error::NONE) {
          ALOGE("Failed to import buffer %p", rawHandle.getNativeHandle());
        }
        bufferHandle = static_cast<const native_handle_t*>(tmpBuffer);
      });

  if (bufferHandle) {
    mImportedBuffers.insert(bufferHandle);
  }

  return bufferHandle;
}

void GrallocWrapper::freeBuffer(const native_handle_t* bufferHandle) {
  auto buffer = const_cast<native_handle_t*>(bufferHandle);

  if (mImportedBuffers.erase(bufferHandle)) {
    mapper2::Error error = mMapper->freeBuffer(buffer);
    if (error != mapper2::Error::NONE) {
      ALOGE("Failed to free %p", buffer);
    }
  } else {
    mClonedBuffers.erase(bufferHandle);
    native_handle_close(buffer);
    native_handle_delete(buffer);
  }
}

void* GrallocWrapper::lock(const native_handle_t* bufferHandle,
                           uint64_t cpuUsage,
                           const mapper2::IMapper::Rect& accessRegion,
                           int acquireFence) {
  auto buffer = const_cast<native_handle_t*>(bufferHandle);

  NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
  hardware::hidl_handle acquireFenceHandle;
  if (acquireFence >= 0) {
    auto h = native_handle_init(acquireFenceStorage, 1, 0);
    h->data[0] = acquireFence;
    acquireFenceHandle = h;
  }

  void* data = nullptr;
  mMapper->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
                [&](const auto& tmpError, const auto& tmpData) {
                  if (tmpError != mapper2::Error::NONE) {
                    ALOGE("Failed to lock buffer %p", buffer);
                  }
                  data = tmpData;
                });

  if (acquireFence >= 0) {
    close(acquireFence);
  }

  return data;
}

int GrallocWrapper::unlock(const native_handle_t* bufferHandle) {
  auto buffer = const_cast<native_handle_t*>(bufferHandle);

  int releaseFence = -1;
  mMapper->unlock(buffer, [&](const auto& tmpError,
                              const auto& tmpReleaseFence) {
    if (tmpError != mapper2::Error::NONE) {
      ALOGE("Failed to unlock buffer %p", buffer);
    }

    auto fenceHandle = tmpReleaseFence.getNativeHandle();
    if (fenceHandle) {
      if (fenceHandle->numInts != 0) {
        ALOGE("Invalid fence handle %p", fenceHandle);
      }
      if (fenceHandle->numFds == 1) {
        releaseFence = dup(fenceHandle->data[0]);
        if (releaseFence < 0){
          ALOGE("Failed to dup fence fd");
        }
      } else {
        if (fenceHandle->numFds != 0) {
          ALOGE("Invalid fence handle %p", fenceHandle);
        }
      }
    }
  });

  return releaseFence;
}

}  // namespace android
