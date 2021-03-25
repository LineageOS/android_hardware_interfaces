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

#include "Utils.h"

#include <aidl/android/hardware/common/Ashmem.h>
#include <aidl/android/hardware/common/MappableFile.h>
#include <aidl/android/hardware/graphics/common/HardwareBuffer.h>
#include <android/binder_auto_utils.h>
#include <android/binder_status.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

nn::GeneralResult<ndk::ScopedFileDescriptor> clone(const ndk::ScopedFileDescriptor& fd);
using utils::clone;

template <typename Type>
nn::GeneralResult<std::vector<Type>> cloneVec(const std::vector<Type>& arguments) {
    std::vector<Type> clonedObjects;
    clonedObjects.reserve(arguments.size());
    for (const auto& argument : arguments) {
        clonedObjects.push_back(NN_TRY(clone(argument)));
    }
    return clonedObjects;
}

template <typename Type>
nn::GeneralResult<std::vector<Type>> clone(const std::vector<Type>& arguments) {
    return cloneVec(arguments);
}

nn::GeneralResult<ndk::ScopedFileDescriptor> clone(const ndk::ScopedFileDescriptor& fd) {
    auto duplicatedFd = NN_TRY(nn::dupFd(fd.get()));
    return ndk::ScopedFileDescriptor(duplicatedFd.release());
}

nn::GeneralResult<common::NativeHandle> clone(const common::NativeHandle& handle) {
    return common::NativeHandle{
            .fds = NN_TRY(cloneVec(handle.fds)),
            .ints = handle.ints,
    };
}

}  // namespace

nn::GeneralResult<Memory> clone(const Memory& memory) {
    switch (memory.getTag()) {
        case Memory::Tag::ashmem: {
            const auto& ashmem = memory.get<Memory::Tag::ashmem>();
            auto handle = common::Ashmem{
                    .fd = NN_TRY(clone(ashmem.fd)),
                    .size = ashmem.size,
            };
            return Memory::make<Memory::Tag::ashmem>(std::move(handle));
        }
        case Memory::Tag::mappableFile: {
            const auto& memFd = memory.get<Memory::Tag::mappableFile>();
            auto handle = common::MappableFile{
                    .length = memFd.length,
                    .prot = memFd.prot,
                    .fd = NN_TRY(clone(memFd.fd)),
                    .offset = memFd.offset,
            };
            return Memory::make<Memory::Tag::mappableFile>(std::move(handle));
        }
        case Memory::Tag::hardwareBuffer: {
            const auto& hardwareBuffer = memory.get<Memory::Tag::hardwareBuffer>();
            auto handle = graphics::common::HardwareBuffer{
                    .description = hardwareBuffer.description,
                    .handle = NN_TRY(clone(hardwareBuffer.handle)),
            };
            return Memory::make<Memory::Tag::hardwareBuffer>(std::move(handle));
        }
    }
    return (NN_ERROR() << "Unrecognized Memory::Tag: " << memory.getTag())
            .
            operator nn::GeneralResult<Memory>();
}

nn::GeneralResult<RequestMemoryPool> clone(const RequestMemoryPool& requestPool) {
    using Tag = RequestMemoryPool::Tag;
    switch (requestPool.getTag()) {
        case Tag::pool:
            return RequestMemoryPool::make<Tag::pool>(NN_TRY(clone(requestPool.get<Tag::pool>())));
        case Tag::token:
            return RequestMemoryPool::make<Tag::token>(requestPool.get<Tag::token>());
    }
    // Using explicit type conversion because std::variant inside the RequestMemoryPool confuses the
    // compiler.
    return (NN_ERROR() << "Unrecognized request pool tag: " << requestPool.getTag())
            .
            operator nn::GeneralResult<RequestMemoryPool>();
}

nn::GeneralResult<Request> clone(const Request& request) {
    return Request{
            .inputs = request.inputs,
            .outputs = request.outputs,
            .pools = NN_TRY(clone(request.pools)),
    };
}

nn::GeneralResult<Model> clone(const Model& model) {
    return Model{
            .main = model.main,
            .referenced = model.referenced,
            .operandValues = model.operandValues,
            .pools = NN_TRY(clone(model.pools)),
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = model.extensionNameToPrefix,
    };
}

nn::GeneralResult<void> handleTransportError(const ndk::ScopedAStatus& ret) {
    if (ret.getStatus() == STATUS_DEAD_OBJECT) {
        return nn::error(nn::ErrorStatus::DEAD_OBJECT)
               << "Binder transaction returned STATUS_DEAD_OBJECT: " << ret.getDescription();
    }
    if (ret.isOk()) {
        return {};
    }
    if (ret.getExceptionCode() != EX_SERVICE_SPECIFIC) {
        return nn::error(nn::ErrorStatus::GENERAL_FAILURE)
               << "Binder transaction returned exception: " << ret.getDescription();
    }
    return nn::error(static_cast<nn::ErrorStatus>(ret.getServiceSpecificError()))
           << ret.getMessage();
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
