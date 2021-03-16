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

#include <android/binder_status.h>
#include <nnapi/Result.h>

namespace aidl::android::hardware::neuralnetworks::utils {
namespace {

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

}  // namespace

nn::GeneralResult<Memory> clone(const Memory& memory) {
    common::NativeHandle nativeHandle;
    nativeHandle.ints = memory.handle.ints;
    nativeHandle.fds.reserve(memory.handle.fds.size());
    for (const auto& fd : memory.handle.fds) {
        const int newFd = dup(fd.get());
        if (newFd < 0) {
            return NN_ERROR() << "Couldn't dup a file descriptor";
        }
        nativeHandle.fds.emplace_back(newFd);
    }
    return Memory{
            .handle = std::move(nativeHandle),
            .size = memory.size,
            .name = memory.name,
    };
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
