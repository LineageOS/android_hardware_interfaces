/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "ResilientPreparedModel.h"

#include "InvalidBurst.h"
#include "InvalidExecution.h"
#include "ResilientBurst.h"
#include "ResilientExecution.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientPreparedModel& resilientPreparedModel, const FnType& fn)
        -> decltype(fn(*resilientPreparedModel.getPreparedModel())) {
    auto preparedModel = resilientPreparedModel.getPreparedModel();
    auto result = fn(*preparedModel);

    // Immediately return if prepared model is not dead.
    if (result.has_value() || result.error().code != nn::ErrorStatus::DEAD_OBJECT) {
        return result;
    }

    // Attempt recovery and return if it fails.
    auto maybePreparedModel = resilientPreparedModel.recover(preparedModel.get());
    if (!maybePreparedModel.has_value()) {
        const auto& [message, code] = maybePreparedModel.error();
        std::ostringstream oss;
        oss << ", and failed to recover dead prepared model with error " << code << ": " << message;
        result.error().message += oss.str();
        return result;
    }
    preparedModel = std::move(maybePreparedModel).value();

    return fn(*preparedModel);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const ResilientPreparedModel>> ResilientPreparedModel::create(
        Factory makePreparedModel) {
    if (makePreparedModel == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientPreparedModel::create must have non-empty makePreparedModel";
    }
    auto preparedModel = NN_TRY(makePreparedModel());
    CHECK(preparedModel != nullptr);
    return std::make_shared<ResilientPreparedModel>(
            PrivateConstructorTag{}, std::move(makePreparedModel), std::move(preparedModel));
}

ResilientPreparedModel::ResilientPreparedModel(PrivateConstructorTag /*tag*/,
                                               Factory makePreparedModel,
                                               nn::SharedPreparedModel preparedModel)
    : kMakePreparedModel(std::move(makePreparedModel)), mPreparedModel(std::move(preparedModel)) {
    CHECK(kMakePreparedModel != nullptr);
    CHECK(mPreparedModel != nullptr);
}

nn::SharedPreparedModel ResilientPreparedModel::getPreparedModel() const {
    std::lock_guard guard(mMutex);
    return mPreparedModel;
}

nn::GeneralResult<nn::SharedPreparedModel> ResilientPreparedModel::recover(
        const nn::IPreparedModel* failingPreparedModel) const {
    std::lock_guard guard(mMutex);

    // Another caller updated the failing prepared model.
    if (mPreparedModel.get() != failingPreparedModel) {
        return mPreparedModel;
    }

    mPreparedModel = NN_TRY(kMakePreparedModel());
    return mPreparedModel;
}

nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>
ResilientPreparedModel::execute(const nn::Request& request, nn::MeasureTiming measure,
                                const nn::OptionalTimePoint& deadline,
                                const nn::OptionalDuration& loopTimeoutDuration) const {
    const auto fn = [&request, measure, &deadline,
                     &loopTimeoutDuration](const nn::IPreparedModel& preparedModel) {
        return preparedModel.execute(request, measure, deadline, loopTimeoutDuration);
    };
    return protect(*this, fn);
}

nn::GeneralResult<std::pair<nn::SyncFence, nn::ExecuteFencedInfoCallback>>
ResilientPreparedModel::executeFenced(const nn::Request& request,
                                      const std::vector<nn::SyncFence>& waitFor,
                                      nn::MeasureTiming measure,
                                      const nn::OptionalTimePoint& deadline,
                                      const nn::OptionalDuration& loopTimeoutDuration,
                                      const nn::OptionalDuration& timeoutDurationAfterFence) const {
    const auto fn = [&request, &waitFor, measure, &deadline, &loopTimeoutDuration,
                     &timeoutDurationAfterFence](const nn::IPreparedModel& preparedModel) {
        return preparedModel.executeFenced(request, waitFor, measure, deadline, loopTimeoutDuration,
                                           timeoutDurationAfterFence);
    };
    return protect(*this, fn);
}

nn::GeneralResult<nn::SharedExecution> ResilientPreparedModel::createReusableExecution(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
#if 0
    auto self = shared_from_this();
    ResilientExecution::Factory makeExecution =
            [preparedModel = std::move(self), request, measure, loopTimeoutDuration] {
        return preparedModel->createReusableExecutionInternal(request, measure, loopTimeoutDuration);
    };
    return ResilientExecution::create(std::move(makeExecution));
#else
    return createReusableExecutionInternal(request, measure, loopTimeoutDuration);
#endif
}

nn::GeneralResult<nn::SharedBurst> ResilientPreparedModel::configureExecutionBurst() const {
#if 0
    auto self = shared_from_this();
    ResilientBurst::Factory makeBurst =
            [preparedModel = std::move(self)]() -> nn::GeneralResult<nn::SharedBurst> {
        return preparedModel->configureExecutionBurst();
    };
    return ResilientBurst::create(std::move(makeBurst));
#else
    return configureExecutionBurstInternal();
#endif
}

nn::GeneralResult<nn::SharedExecution> ResilientPreparedModel::createReusableExecutionInternal(
        const nn::Request& request, nn::MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration) const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidExecution>();
    }
    const auto fn = [&request, measure,
                     &loopTimeoutDuration](const nn::IPreparedModel& preparedModel) {
        return preparedModel.createReusableExecution(request, measure, loopTimeoutDuration);
    };
    return protect(*this, fn);
}

std::any ResilientPreparedModel::getUnderlyingResource() const {
    return getPreparedModel()->getUnderlyingResource();
}

bool ResilientPreparedModel::isValidInternal() const {
    return true;
}

nn::GeneralResult<nn::SharedBurst> ResilientPreparedModel::configureExecutionBurstInternal() const {
    if (!isValidInternal()) {
        return std::make_shared<const InvalidBurst>();
    }
    const auto fn = [](const nn::IPreparedModel& preparedModel) {
        return preparedModel.configureExecutionBurst();
    };
    return protect(*this, fn);
}

}  // namespace android::hardware::neuralnetworks::utils
