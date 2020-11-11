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

#include <nnapi/Result.h>

namespace aidl::android::hardware::neuralnetworks::utils {

using ::android::nn::GeneralResult;

GeneralResult<Model> copyModel(const Model& model) {
    Model newModel{
            .main = model.main,
            .referenced = model.referenced,
            .operandValues = model.operandValues,
            .pools = {},
            .relaxComputationFloat32toFloat16 = model.relaxComputationFloat32toFloat16,
            .extensionNameToPrefix = model.extensionNameToPrefix,
    };
    newModel.pools.reserve(model.pools.size());
    for (const auto& pool : model.pools) {
        common::NativeHandle nativeHandle;
        nativeHandle.ints = pool.handle.ints;
        nativeHandle.fds.reserve(pool.handle.fds.size());
        for (const auto& fd : pool.handle.fds) {
            const int newFd = dup(fd.get());
            if (newFd == -1) {
                return NN_ERROR() << "Couldn't dup a file descriptor.";
            }
            nativeHandle.fds.emplace_back(newFd);
        }
        Memory memory = {
                .handle = std::move(nativeHandle),
                .size = pool.size,
                .name = pool.name,
        };
        newModel.pools.push_back(std::move(memory));
    }
    return newModel;
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
