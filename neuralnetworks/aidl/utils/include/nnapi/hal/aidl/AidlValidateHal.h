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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_VALIDATE_HAL_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_VALIDATE_HAL_H

#include <memory>
#include <set>
#include <tuple>
#include <vector>

#include "AidlHalInterfaces.h"
#include "nnapi/TypeUtils.h"
#include "nnapi/Validation.h"

namespace android {
namespace nn {

using AidlHalPreparedModelRole = std::tuple<const aidl_hal::IPreparedModel*, IOType, uint32_t>;

bool validateMemoryDesc(
        const aidl_hal::BufferDesc& desc,
        const std::vector<std::shared_ptr<aidl_hal::IPreparedModel>>& preparedModels,
        const std::vector<aidl_hal::BufferRole>& inputRoles,
        const std::vector<aidl_hal::BufferRole>& outputRoles,
        std::function<const aidl_hal::Model*(const std::shared_ptr<aidl_hal::IPreparedModel>&)>
                getModel,
        std::set<AidlHalPreparedModelRole>* preparedModelRoles, aidl_hal::Operand* combinedOperand);

}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_VALIDATE_HAL_H
