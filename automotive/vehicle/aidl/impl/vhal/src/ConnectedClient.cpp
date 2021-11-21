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
        ALOGE("failed to call callback, error: %s, code: %d", callbackStatus.getMessage(),
              callbackStatus.getServiceSpecificError());
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
                              const std::vector<ResultType>& results) {
    ResultsType parcelableResults;
    ScopedAStatus status = vectorToStableLargeParcelable(results, &parcelableResults);
    if (status.isOk()) {
        if (ScopedAStatus callbackStatus = callCallback(callback, parcelableResults);
            !callbackStatus.isOk()) {
            ALOGE("failed to call callback, error: %s, code: %d", status.getMessage(),
                  status.getServiceSpecificError());
        }
        return;
    }
    int statusCode = status.getServiceSpecificError();
    ALOGE("failed to marshal result into large parcelable, error: "
          "%s, code: %d",
          status.getMessage(), statusCode);
    sendGetOrSetValueResultsSeparately<ResultType, ResultsType>(callback, results);
}

// Specify the functions for GetValues and SetValues types.
template void sendGetOrSetValueResult<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const GetValueResult& result);
template void sendGetOrSetValueResult<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const SetValueResult& result);

template void sendGetOrSetValueResults<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<GetValueResult>& results);
template void sendGetOrSetValueResults<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<SetValueResult>& results);

template void sendGetOrSetValueResultsSeparately<GetValueResult, GetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<GetValueResult>& results);
template void sendGetOrSetValueResultsSeparately<SetValueResult, SetValueResults>(
        std::shared_ptr<IVehicleCallback> callback, const std::vector<SetValueResult>& results);

}  // namespace

ConnectedClient::ConnectedClient(std::shared_ptr<IVehicleCallback> callback)
    : mCallback(callback) {}

template <class ResultType, class ResultsType>
GetSetValuesClient<ResultType, ResultsType>::GetSetValuesClient(
        std::shared_ptr<IVehicleCallback> callback)
    : ConnectedClient(callback) {
    mResultCallback = std::make_shared<const std::function<void(std::vector<ResultType>)>>(
            [callback](std::vector<ResultType> results) {
                return sendGetOrSetValueResults<ResultType, ResultsType>(callback, results);
            });
}

template <class ResultType, class ResultsType>
std::shared_ptr<const std::function<void(std::vector<ResultType>)>>
GetSetValuesClient<ResultType, ResultsType>::getResultCallback() {
    return mResultCallback;
}

template <class ResultType, class ResultsType>
void GetSetValuesClient<ResultType, ResultsType>::sendResults(
        const std::vector<ResultType>& results) {
    return sendGetOrSetValueResults<ResultType, ResultsType>(mCallback, results);
}

template <class ResultType, class ResultsType>
void GetSetValuesClient<ResultType, ResultsType>::sendResultsSeparately(
        const std::vector<ResultType>& results) {
    return sendGetOrSetValueResultsSeparately<ResultType, ResultsType>(mCallback, results);
}

template class GetSetValuesClient<GetValueResult, GetValueResults>;
template class GetSetValuesClient<SetValueResult, SetValueResults>;

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
