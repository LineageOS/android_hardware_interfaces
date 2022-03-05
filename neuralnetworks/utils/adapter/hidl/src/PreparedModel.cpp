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

#include "PreparedModel.h"

#include "Burst.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.2/IBurstCallback.h>
#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IFencedExecutionCallback.h>
#include <android/hardware/neuralnetworks/1.3/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/1.0/Utils.h>
#include <nnapi/hal/1.2/Utils.h>
#include <nnapi/hal/1.3/Conversions.h>
#include <nnapi/hal/1.3/Utils.h>

#include <memory>
#include <thread>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {
namespace {

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

class FencedExecutionCallback final : public V1_3::IFencedExecutionCallback {
  public:
    explicit FencedExecutionCallback(const nn::ExecuteFencedInfoCallback& callback)
        : kCallback(callback) {
        CHECK(callback != nullptr);
    }

    Return<void> getExecutionInfo(getExecutionInfo_cb cb) override {
        const auto result = kCallback();
        if (!result.has_value()) {
            const auto& [message, code] = result.error();
            const auto status =
                    V1_3::utils::convert(code).value_or(V1_3::ErrorStatus::GENERAL_FAILURE);
            LOG(ERROR) << message;
            cb(status, V1_2::utils::kNoTiming, V1_2::utils::kNoTiming);
            return Void();
        }
        const auto [timingLaunched, timingFenced] = result.value();
        const auto hidlTimingLaunched = V1_3::utils::convert(timingLaunched).value();
        const auto hidlTimingFenced = V1_3::utils::convert(timingFenced).value();
        cb(V1_3::ErrorStatus::NONE, hidlTimingLaunched, hidlTimingFenced);
        return Void();
    }

  private:
    const nn::ExecuteFencedInfoCallback kCallback;
};

using ExecutionResult = nn::ExecutionResult<std::pair<std::vector<nn::OutputShape>, nn::Timing>>;

void notify(V1_0::IExecutionCallback* callback, nn::ErrorStatus status,
            const std::vector<nn::OutputShape>& /*outputShapes*/, const nn::Timing& /*timing*/) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_0::utils::convert(status).value();
        const auto ret = callback->notify(hidlStatus);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_0::IExecutionCallback::notify failed with " << ret.description();
        }
    }
}

void notify(V1_2::IExecutionCallback* callback, nn::ErrorStatus status,
            const std::vector<nn::OutputShape>& outputShapes, const nn::Timing& timing) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_2::utils::convert(status).value();
        const auto hidlOutputShapes = V1_2::utils::convert(outputShapes).value();
        const auto hidlTiming = V1_2::utils::convert(timing).value();
        const auto ret = callback->notify_1_2(hidlStatus, hidlOutputShapes, hidlTiming);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_2::IExecutionCallback::notify_1_2 failed with " << ret.description();
        }
    }
}

void notify(V1_3::IExecutionCallback* callback, nn::ErrorStatus status,
            const std::vector<nn::OutputShape>& outputShapes, const nn::Timing& timing) {
    if (callback != nullptr) {
        const auto hidlStatus = V1_3::utils::convert(status).value();
        const auto hidlOutputShapes = V1_3::utils::convert(outputShapes).value();
        const auto hidlTiming = V1_3::utils::convert(timing).value();
        const auto ret = callback->notify_1_3(hidlStatus, hidlOutputShapes, hidlTiming);
        if (!ret.isOk()) {
            LOG(ERROR) << "V1_3::IExecutionCallback::notify_1_3 failed with " << ret.description();
        }
    }
}

template <typename CallbackType>
void notify(CallbackType* callback, ExecutionResult result) {
    if (!result.has_value()) {
        const auto [message, status, outputShapes] = std::move(result).error();
        LOG(ERROR) << message;
        notify(callback, status, outputShapes, {});
    } else {
        const auto [outputShapes, timing] = std::move(result).value();
        notify(callback, nn::ErrorStatus::NONE, outputShapes, timing);
    }
}

nn::GeneralResult<void> execute(const nn::SharedPreparedModel& preparedModel,
                                const V1_0::Request& request,
                                const sp<V1_0::IExecutionCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    const auto nnRequest = NN_TRY(convertInput(request));

    auto result = preparedModel->execute(nnRequest, nn::MeasureTiming::NO, {}, {}, {}, {});

    if (!result.ok() && result.error().code == nn::ErrorStatus::INVALID_ARGUMENT) {
        const auto& [message, code, outputShapes] = result.error();
        return nn::error(code) << message;
    }

    notify(callback.get(), std::move(result));
    return {};
}

nn::GeneralResult<void> execute_1_2(const nn::SharedPreparedModel& preparedModel,
                                    const V1_0::Request& request, V1_2::MeasureTiming measure,
                                    const sp<V1_2::IExecutionCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasure = NN_TRY(convertInput(measure));

    auto result = preparedModel->execute(nnRequest, nnMeasure, {}, {}, {}, {});

    if (!result.ok() && result.error().code == nn::ErrorStatus::INVALID_ARGUMENT) {
        const auto& [message, code, outputShapes] = result.error();
        return nn::error(code) << message;
    }

    notify(callback.get(), std::move(result));
    return {};
}

nn::GeneralResult<void> execute_1_3(const nn::SharedPreparedModel& preparedModel,
                                    const V1_3::Request& request, V1_2::MeasureTiming measure,
                                    const V1_3::OptionalTimePoint& deadline,
                                    const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
                                    const sp<V1_3::IExecutionCallback>& callback) {
    if (callback.get() == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT) << "Invalid callback";
    }

    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasure = NN_TRY(convertInput(measure));
    const auto nnDeadline = NN_TRY(convertInput(deadline));
    const auto nnLoopTimeoutDuration = NN_TRY(convertInput(loopTimeoutDuration));

    auto result =
            preparedModel->execute(nnRequest, nnMeasure, nnDeadline, nnLoopTimeoutDuration, {}, {});

    if (!result.ok() && result.error().code == nn::ErrorStatus::INVALID_ARGUMENT) {
        const auto& [message, code, outputShapes] = result.error();
        return nn::error(code) << message;
    }

    notify(callback.get(), std::move(result));
    return {};
}

nn::ExecutionResult<std::pair<hidl_vec<V1_2::OutputShape>, V1_2::Timing>> executeSynchronously(
        const nn::SharedPreparedModel& preparedModel, const V1_0::Request& request,
        V1_2::MeasureTiming measure) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasure = NN_TRY(convertInput(measure));

    const auto [outputShapes, timing] =
            NN_TRY(preparedModel->execute(nnRequest, nnMeasure, {}, {}, {}, {}));

    auto hidlOutputShapes = NN_TRY(V1_2::utils::convert(outputShapes));
    const auto hidlTiming = NN_TRY(V1_2::utils::convert(timing));
    return std::make_pair(std::move(hidlOutputShapes), hidlTiming);
}

nn::ExecutionResult<std::pair<hidl_vec<V1_2::OutputShape>, V1_2::Timing>> executeSynchronously_1_3(
        const nn::SharedPreparedModel& preparedModel, const V1_3::Request& request,
        V1_2::MeasureTiming measure, const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnMeasure = NN_TRY(convertInput(measure));
    const auto nnDeadline = NN_TRY(convertInput(deadline));
    const auto nnLoopTimeoutDuration = NN_TRY(convertInput(loopTimeoutDuration));

    const auto [outputShapes, timing] = NN_TRY(preparedModel->execute(
            nnRequest, nnMeasure, nnDeadline, nnLoopTimeoutDuration, {}, {}));

    auto hidlOutputShapes = NN_TRY(V1_3::utils::convert(outputShapes));
    const auto hidlTiming = NN_TRY(V1_3::utils::convert(timing));
    return std::make_pair(std::move(hidlOutputShapes), hidlTiming);
}

nn::GeneralResult<std::vector<nn::SyncFence>> convertSyncFences(
        const hidl_vec<hidl_handle>& handles) {
    auto nnHandles = NN_TRY(convertInput(handles));
    std::vector<nn::SyncFence> syncFences;
    syncFences.reserve(handles.size());
    for (auto&& handle : nnHandles) {
        if (auto syncFence = nn::SyncFence::create(std::move(handle)); !syncFence.ok()) {
            return nn::error(nn::ErrorStatus::INVALID_ARGUMENT) << std::move(syncFence).error();
        } else {
            syncFences.push_back(std::move(syncFence).value());
        }
    }
    return syncFences;
}

nn::GeneralResult<sp<V1_2::IBurstContext>> configureExecutionBurst(
        const nn::SharedPreparedModel& preparedModel, const sp<V1_2::IBurstCallback>& callback,
        const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel) {
    auto burstExecutor = NN_TRY(preparedModel->configureExecutionBurst());
    return Burst::create(callback, requestChannel, resultChannel, std::move(burstExecutor),
                         V1_2::utils::getBurstServerPollingTimeWindow());
}

nn::GeneralResult<std::pair<hidl_handle, sp<V1_3::IFencedExecutionCallback>>> executeFenced(
        const nn::SharedPreparedModel& preparedModel, const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor, V1_2::MeasureTiming measure,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const V1_3::OptionalTimeoutDuration& duration) {
    const auto nnRequest = NN_TRY(convertInput(request));
    const auto nnWaitFor = NN_TRY(convertSyncFences(waitFor));
    const auto nnMeasure = NN_TRY(convertInput(measure));
    const auto nnDeadline = NN_TRY(convertInput(deadline));
    const auto nnLoopTimeoutDuration = NN_TRY(convertInput(loopTimeoutDuration));
    const auto nnDuration = NN_TRY(convertInput(duration));

    auto [syncFence, executeFencedCallback] =
            NN_TRY(preparedModel->executeFenced(nnRequest, nnWaitFor, nnMeasure, nnDeadline,
                                                nnLoopTimeoutDuration, nnDuration, {}, {}));

    auto hidlSyncFence = NN_TRY(V1_3::utils::convert(syncFence.getSharedHandle()));
    auto hidlExecuteFencedCallback = sp<FencedExecutionCallback>::make(executeFencedCallback);
    return std::make_pair(std::move(hidlSyncFence), std::move(hidlExecuteFencedCallback));
}

}  // namespace

PreparedModel::PreparedModel(nn::SharedPreparedModel preparedModel)
    : kPreparedModel(std::move(preparedModel)) {
    CHECK(kPreparedModel != nullptr);
}

nn::SharedPreparedModel PreparedModel::getUnderlyingPreparedModel() const {
    return kPreparedModel;
}

Return<V1_0::ErrorStatus> PreparedModel::execute(const V1_0::Request& request,
                                                 const sp<V1_0::IExecutionCallback>& callback) {
    auto result = adapter::execute(kPreparedModel, request, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::execute failed with " << code << ": " << message;
        notify(callback.get(), code, {}, {});
        return V1_0::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_0::ErrorStatus> PreparedModel::execute_1_2(const V1_0::Request& request,
                                                     V1_2::MeasureTiming measure,
                                                     const sp<V1_2::IExecutionCallback>& callback) {
    auto result = adapter::execute_1_2(kPreparedModel, request, measure, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::execute_1_2 failed with " << code << ": " << message;
        notify(callback.get(), code, {}, {});
        return V1_2::utils::convert(code).value();
    }
    return V1_0::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> PreparedModel::execute_1_3(
        const V1_3::Request& request, V1_2::MeasureTiming measure,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const sp<V1_3::IExecutionCallback>& callback) {
    auto result = adapter::execute_1_3(kPreparedModel, request, measure, deadline,
                                       loopTimeoutDuration, callback);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::execute_1_3 failed with " << code << ": " << message;
        notify(callback.get(), code, {}, {});
        return V1_3::utils::convert(code).value();
    }
    return V1_3::ErrorStatus::NONE;
}

Return<void> PreparedModel::executeSynchronously(const V1_0::Request& request,
                                                 V1_2::MeasureTiming measure,
                                                 executeSynchronously_cb cb) {
    auto result = adapter::executeSynchronously(kPreparedModel, request, measure);
    if (!result.has_value()) {
        auto [message, code, outputShapes] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::executeSynchronously failed with " << code << ": "
                   << message;
        cb(V1_2::utils::convert(code).value(), V1_2::utils::convert(outputShapes).value(),
           V1_2::utils::kNoTiming);
        return Void();
    }
    auto [outputShapes, timing] = std::move(result).value();
    cb(V1_0::ErrorStatus::NONE, outputShapes, timing);
    return Void();
}

Return<void> PreparedModel::executeSynchronously_1_3(
        const V1_3::Request& request, V1_2::MeasureTiming measure,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration, executeSynchronously_1_3_cb cb) {
    auto result = adapter::executeSynchronously_1_3(kPreparedModel, request, measure, deadline,
                                                    loopTimeoutDuration);
    if (!result.has_value()) {
        auto [message, code, outputShapes] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::executeSynchronously_1_3 failed with " << code
                   << ": " << message;
        cb(V1_3::utils::convert(code).value(), V1_3::utils::convert(outputShapes).value(),
           V1_2::utils::kNoTiming);
        return Void();
    }
    auto [outputShapes, timing] = std::move(result).value();
    cb(V1_3::ErrorStatus::NONE, outputShapes, timing);
    return Void();
}

Return<void> PreparedModel::configureExecutionBurst(
        const sp<V1_2::IBurstCallback>& callback,
        const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
        configureExecutionBurst_cb cb) {
    auto result = adapter::configureExecutionBurst(kPreparedModel, callback, requestChannel,
                                                   resultChannel);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::configureExecutionBurst failed with " << code << ": "
                   << message;
        cb(V1_2::utils::convert(code).value(), nullptr);
        return Void();
    }
    const auto burstContext = std::move(result).value();
    cb(V1_0::ErrorStatus::NONE, burstContext);
    return Void();
}

Return<void> PreparedModel::executeFenced(const V1_3::Request& request,
                                          const hidl_vec<hidl_handle>& waitFor,
                                          V1_2::MeasureTiming measure,
                                          const V1_3::OptionalTimePoint& deadline,
                                          const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
                                          const V1_3::OptionalTimeoutDuration& duration,
                                          executeFenced_cb callback) {
    auto result = adapter::executeFenced(kPreparedModel, request, waitFor, measure, deadline,
                                         loopTimeoutDuration, duration);
    if (!result.has_value()) {
        auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::PreparedModel::executeFenced failed with " << code << ": "
                   << message;
        callback(V1_3::utils::convert(code).value(), {}, nullptr);
        return Void();
    }
    auto [syncFence, executeFencedCallback] = std::move(result).value();
    callback(V1_3::ErrorStatus::NONE, syncFence, executeFencedCallback);
    return Void();
}

}  // namespace android::hardware::neuralnetworks::adapter
