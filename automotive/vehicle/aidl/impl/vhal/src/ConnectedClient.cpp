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

#include "ConnectedClient.h"
#include "ParcelableUtils.h"

#include <VehicleHalTypes.h>

#include <utils/Log.h>

#include <inttypes.h>
#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropError;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropErrors;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValues;
using ::android::base::Result;
using ::ndk::ScopedAStatus;

// A function to call the specific callback based on results type.
template <class T>
ScopedAStatus callCallback(std::shared_ptr<IVehicleCallback> callback, const T& results);

template <>
ScopedAStatus callCallback<GetValueResults>(std::shared_ptr<IVehicleCallback> callback,
                                            const GetValueResults& results) {
    return callback->onGetValues(results);
}

template <>
ScopedAStatus callCallback<SetValueResults>(std::shared_ptr<IVehicleCallback> callback,
                                            const SetValueResults& results) {
    return callback->onSetValues(results);
}

// Send a single GetValue/SetValue result through the callback.
template <class ResultType, class ResultsType>
void sendGetOrSetValueResult(std::shared_ptr<IVehicleCallback> callback, const ResultType& result) {
    ResultsType parcelableResults;
    parcelableResults.payloads.resize(1);
    parcelableResults.payloads[0] = result;
    if (ScopedAStatus callbackStatus = callCallback(callback, parcelableResults);
        !callbackStatus.isOk()) {
        ALOGE("failed to call GetOrSetValueResult callback, client ID: %p, error: %s, "
              "exception: %d, service specific error: %d",
              callback->asBinder().get(), callbackStatus.getMessage(),
              callbackStatus.getExceptionCode(), callbackStatus.getServiceSpecificError());
    }
}

// Send all the GetValue/SetValue results through callback, one result in each callback invocation.
template <class ResultType, class ResultsType>
void sendGetOrSetValueResultsSeparately(std::shared_ptr<IVehicleCallback> callback,
                                        const std::vector<ResultType>& results) {
    for (const auto& result : results) {
        sendGetOrSetValueResult<ResultType, ResultsType>(callback, result);
    }
}

// Send all the GetValue/SetValue results through callback in a single callback invocation.
template <class ResultType, class ResultsType>
void sendGetOrSetValueResults(std::shared_ptr<IVehicleCallback> callback,
                              std::vector<ResultType>&& results) {
    ResultsType parcelableResults;
    ScopedAStatus status = vectorToStableLargeParcelable(std::move(results), &parcelableResults);
    if (status.isOk()) {
        if (ScopedAStatus callbackStatus = callCallback(callback, parcelableResults);
            !callbackStatus.isOk()) {
            ALOGE("failed to call GetOrSetValueResults callback, client ID: %p, error: %s, "
                  "exception: %d, service specific error: %d",
                  callback->asBinder().get(), callbackStatus.getMessage(),
                  callbackStatus.getExceptionCode(), callbackStatus.getServiceSpecificError());
        }
        return;
    }
    int statusCode = status.getServiceSpecificError();
    ALOGE("failed to marshal result into large parcelable, error: "
          "%s, code: %d",
          status.getMessage(), statusCode);
    sendGetOrSetValueResultsSeparately<ResultType, ResultsType>(callback,
                                                                parcelableResults.payloads);
}

// The timeout callback for GetValues/SetValues.
template <class ResultType, class ResultsType>
void onTimeout(
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        const std::unordered_set<int64_t>& timeoutIds) {
    std::vector<ResultType> timeoutResults;
    for (int64_t requestId : timeoutIds) {
        ALOGD("hardware request timeout, request ID: %" PRId64, requestId);
        timeoutResults.push_back({
                .requestId = requestId,
                .status = StatusCode::TRY_AGAIN,
        });
    }
    sendGetOrSetValueResults<ResultType, ResultsType>(callback, std::move(timeoutResults));
}

// The on-results callback for GetValues/SetValues.
template <class ResultType, class ResultsType>
void getOrSetValuesCallback(
        const void* clientId,
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        std::vector<ResultType>&& results, std::shared_ptr<PendingRequestPool> requestPool) {
    std::unordered_set<int64_t> requestIds;
    for (const auto& result : results) {
        requestIds.insert(result.requestId);
    }

    auto finishedRequests = requestPool->tryFinishRequests(clientId, requestIds);

    auto it = results.begin();
    while (it != results.end()) {
        int64_t requestId = it->requestId;
        if (finishedRequests.find(requestId) == finishedRequests.end()) {
            ALOGD("no pending request for the result from hardware, "
                  "possibly already time-out, ID: %" PRId64,
                  requestId);
            it = results.erase(it);
        } else {
            it++;
        }
    }

    if (!results.empty()) {
        sendGetOrSetValueResults<ResultType, ResultsType>(callback, std::move(results));
    }
}

// Specify the functions for GetValues and SetValues types.
template void sendGetOrSetValueResult<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const GetValueResult& result);
template void sendGetOrSetValueResult<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const SetValueResult& result);

template void sendGetOrSetValueResults<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, std::vector<GetValueResult>&& results);
template void sendGetOrSetValueResults<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, std::vector<SetValueResult>&& results);

template void sendGetOrSetValueResultsSeparately<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<GetValueResult>& results);
template void sendGetOrSetValueResultsSeparately<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<SetValueResult>& results);

template void onTimeout<GetValueResult, GetValueResults>(
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        const std::unordered_set<int64_t>& timeoutIds);
template void onTimeout<SetValueResult, SetValueResults>(
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        const std::unordered_set<int64_t>& timeoutIds);

template void getOrSetValuesCallback<GetValueResult, GetValueResults>(
        const void* clientId,
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        std::vector<GetValueResult>&& results, std::shared_ptr<PendingRequestPool> requestPool);
template void getOrSetValuesCallback<SetValueResult, SetValueResults>(
        const void* clientId,
        std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback> callback,
        std::vector<SetValueResult>&& results, std::shared_ptr<PendingRequestPool> requestPool);

}  // namespace

ConnectedClient::ConnectedClient(std::shared_ptr<PendingRequestPool> requestPool,
                                 std::shared_ptr<IVehicleCallback> callback)
    : mRequestPool(requestPool), mCallback(callback) {}

const void* ConnectedClient::id() {
    return reinterpret_cast<const void*>(this);
}

VhalResult<void> ConnectedClient::addRequests(const std::unordered_set<int64_t>& requestIds) {
    return mRequestPool->addRequests(id(), requestIds, getTimeoutCallback());
}

std::unordered_set<int64_t> ConnectedClient::tryFinishRequests(
        const std::unordered_set<int64_t>& requestIds) {
    return mRequestPool->tryFinishRequests(id(), requestIds);
}

template <class ResultType, class ResultsType>
GetSetValuesClient<ResultType, ResultsType>::GetSetValuesClient(
        std::shared_ptr<PendingRequestPool> requestPool, std::shared_ptr<IVehicleCallback> callback)
    : ConnectedClient(requestPool, callback) {
    mTimeoutCallback = std::make_shared<const PendingRequestPool::TimeoutCallbackFunc>(
            [callback](const std::unordered_set<int64_t>& timeoutIds) {
                return onTimeout<ResultType, ResultsType>(callback, timeoutIds);
            });
    auto requestPoolCopy = mRequestPool;
    const void* clientId = id();
    mResultCallback = std::make_shared<const std::function<void(std::vector<ResultType>)>>(
            [clientId, callback, requestPoolCopy](std::vector<ResultType> results) {
                return getOrSetValuesCallback<ResultType, ResultsType>(
                        clientId, callback, std::move(results), requestPoolCopy);
            });
}

template <class ResultType, class ResultsType>
std::shared_ptr<const std::function<void(std::vector<ResultType>)>>
GetSetValuesClient<ResultType, ResultsType>::getResultCallback() {
    return mResultCallback;
}

template <class ResultType, class ResultsType>
std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc>
GetSetValuesClient<ResultType, ResultsType>::getTimeoutCallback() {
    return mTimeoutCallback;
}

template <class ResultType, class ResultsType>
void GetSetValuesClient<ResultType, ResultsType>::sendResults(std::vector<ResultType>&& results) {
    return sendGetOrSetValueResults<ResultType, ResultsType>(mCallback, std::move(results));
}

template <class ResultType, class ResultsType>
void GetSetValuesClient<ResultType, ResultsType>::sendResultsSeparately(
        const std::vector<ResultType>& results) {
    return sendGetOrSetValueResultsSeparately<ResultType, ResultsType>(mCallback, results);
}

template class GetSetValuesClient<GetValueResult, GetValueResults>;
template class GetSetValuesClient<SetValueResult, SetValueResults>;

SubscriptionClient::SubscriptionClient(std::shared_ptr<PendingRequestPool> requestPool,
                                       std::shared_ptr<IVehicleCallback> callback)
    : ConnectedClient(requestPool, callback) {
    mTimeoutCallback = std::make_shared<const PendingRequestPool::TimeoutCallbackFunc>(
            [](std::unordered_set<int64_t> timeoutIds) {
                for (int64_t id : timeoutIds) {
                    ALOGW("subscribe: requests with IDs: %" PRId64
                          " has timed-out, not client informed, "
                          "possibly one of recurrent requests for this subscription failed",
                          id);
                }
            });
    auto requestPoolCopy = mRequestPool;
    const void* clientId = reinterpret_cast<const void*>(this);
    mResultCallback = std::make_shared<const IVehicleHardware::GetValuesCallback>(
            [clientId, callback, requestPoolCopy](std::vector<GetValueResult> results) {
                onGetValueResults(clientId, callback, requestPoolCopy, results);
            });
}

std::shared_ptr<const std::function<void(std::vector<GetValueResult>)>>
SubscriptionClient::getResultCallback() {
    return mResultCallback;
}

std::shared_ptr<const PendingRequestPool::TimeoutCallbackFunc>
SubscriptionClient::getTimeoutCallback() {
    return mTimeoutCallback;
}

void SubscriptionClient::sendUpdatedValues(std::shared_ptr<IVehicleCallback> callback,
                                           std::vector<VehiclePropValue>&& updatedValues) {
    if (updatedValues.empty()) {
        return;
    }

    // TODO(b/205189110): Use memory pool here and fill in sharedMemoryId.
    VehiclePropValues vehiclePropValues;
    int32_t sharedMemoryFileCount = 0;
    ScopedAStatus status =
            vectorToStableLargeParcelable(std::move(updatedValues), &vehiclePropValues);
    if (!status.isOk()) {
        int statusCode = status.getServiceSpecificError();
        ALOGE("subscribe: failed to marshal result into large parcelable, error: "
              "%s, code: %d",
              status.getMessage(), statusCode);
        return;
    }

    if (ScopedAStatus callbackStatus =
                callback->onPropertyEvent(vehiclePropValues, sharedMemoryFileCount);
        !callbackStatus.isOk()) {
        ALOGE("subscribe: failed to call onPropertyEvent callback, client ID: %p, error: %s, "
              "exception: %d, service specific error: %d",
              callback->asBinder().get(), callbackStatus.getMessage(),
              callbackStatus.getExceptionCode(), callbackStatus.getServiceSpecificError());
    }
}

void SubscriptionClient::sendPropertySetErrors(std::shared_ptr<IVehicleCallback> callback,
                                               std::vector<VehiclePropError>&& vehiclePropErrors) {
    if (vehiclePropErrors.empty()) {
        return;
    }

    VehiclePropErrors vehiclePropErrorsLargeParcelable;
    ScopedAStatus status = vectorToStableLargeParcelable(std::move(vehiclePropErrors),
                                                         &vehiclePropErrorsLargeParcelable);
    if (!status.isOk()) {
        int statusCode = status.getServiceSpecificError();
        ALOGE("subscribe: failed to marshal result into large parcelable, error: "
              "%s, code: %d",
              status.getMessage(), statusCode);
        return;
    }

    if (ScopedAStatus callbackStatus =
                callback->onPropertySetError(vehiclePropErrorsLargeParcelable);
        !callbackStatus.isOk()) {
        ALOGE("subscribe: failed to call onPropertySetError callback, client ID: %p, error: %s, "
              "exception: %d, service specific error: %d",
              callback->asBinder().get(), callbackStatus.getMessage(),
              callbackStatus.getExceptionCode(), callbackStatus.getServiceSpecificError());
    }
}

void SubscriptionClient::onGetValueResults(const void* clientId,
                                           std::shared_ptr<IVehicleCallback> callback,
                                           std::shared_ptr<PendingRequestPool> requestPool,
                                           std::vector<GetValueResult> results) {
    std::unordered_set<int64_t> requestIds;
    for (const auto& result : results) {
        requestIds.insert(result.requestId);
    }

    auto finishedRequests = requestPool->tryFinishRequests(clientId, requestIds);
    std::vector<VehiclePropValue> propValues;
    for (auto& result : results) {
        int64_t requestId = result.requestId;
        if (finishedRequests.find(requestId) == finishedRequests.end()) {
            ALOGE("subscribe[%" PRId64
                  "]: no pending request for the result from hardware, "
                  "possibly already time-out",
                  requestId);
            continue;
        }
        if (result.status != StatusCode::OK) {
            ALOGE("subscribe[%" PRId64
                  "]: hardware returns non-ok status for getValues, status: "
                  "%d",
                  requestId, toInt(result.status));
            continue;
        }
        if (!result.prop.has_value()) {
            ALOGE("subscribe[%" PRId64 "]: no prop value in getValues result", requestId);
            continue;
        }
        propValues.push_back(std::move(result.prop.value()));
    }

    sendUpdatedValues(callback, std::move(propValues));
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
