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

#include "MockVehicleHardware.h"
#include "MockVehicleCallback.h"

#include <utils/Log.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

MockVehicleHardware::MockVehicleHardware() {
    mRecurrentTimer = std::make_unique<RecurrentTimer>();
}

MockVehicleHardware::~MockVehicleHardware() {
    std::unique_lock<std::mutex> lk(mLock);
    mCv.wait(lk, [this] { return mThreadCount == 0; });
    mRecurrentTimer.reset();
}

std::vector<VehiclePropConfig> MockVehicleHardware::getAllPropertyConfigs() const {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mPropertyConfigs;
}

StatusCode MockVehicleHardware::setValues(std::shared_ptr<const SetValuesCallback> callback,
                                          const std::vector<SetValueRequest>& requests) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    if (StatusCode status = handleRequestsLocked(__func__, callback, requests, &mSetValueRequests,
                                                 &mSetValueResponses);
        status != StatusCode::OK) {
        return status;
    }
    if (mPropertyChangeCallback == nullptr) {
        return StatusCode::OK;
    }
    std::vector<VehiclePropValue> values;
    for (auto& request : requests) {
        values.push_back(request.value);
    }
    (*mPropertyChangeCallback)(values);
    return StatusCode::OK;
}

StatusCode MockVehicleHardware::getValues(std::shared_ptr<const GetValuesCallback> callback,
                                          const std::vector<GetValueRequest>& requests) const {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    if (mGetValueResponder != nullptr) {
        return mGetValueResponder(callback, requests);
    }
    return handleRequestsLocked(__func__, callback, requests, &mGetValueRequests,
                                &mGetValueResponses);
}

void MockVehicleHardware::setDumpResult(DumpResult result) {
    mDumpResult = result;
}

DumpResult MockVehicleHardware::dump(const std::vector<std::string>&) {
    return mDumpResult;
}

StatusCode MockVehicleHardware::checkHealth() {
    return StatusCode::OK;
}

StatusCode MockVehicleHardware::subscribe(SubscribeOptions options) {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mSubscribeOptions.push_back(options);
    }
    for (int32_t areaId : options.areaIds) {
        if (auto status = subscribePropIdAreaId(options.propId, areaId, options.sampleRate);
            status != StatusCode::OK) {
            return status;
        }
    }
    return StatusCode::OK;
}

std::vector<SubscribeOptions> MockVehicleHardware::getSubscribeOptions() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mSubscribeOptions;
}

void MockVehicleHardware::clearSubscribeOptions() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mSubscribeOptions.clear();
}

StatusCode MockVehicleHardware::subscribePropIdAreaId(int32_t propId, int32_t areaId,
                                                      float sampleRateHz) {
    if (sampleRateHz == 0) {
        // on-change property.
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mSubOnChangePropIdAreaIds.insert(std::pair<int32_t, int32_t>(propId, areaId));
        return StatusCode::OK;
    }

    // continuous property.
    std::shared_ptr<std::function<void()>> action;

    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        if (mRecurrentActions[propId][areaId] != nullptr) {
            // Remove the previous action register for this [propId, areaId].
            mRecurrentTimer->unregisterTimerCallback(mRecurrentActions[propId][areaId]);
        }

        // We are sure 'propertyChangeCallback' would be alive because we would unregister timer
        // before destroying 'this' which owns mPropertyChangeCallback.
        const PropertyChangeCallback* propertyChangeCallback = mPropertyChangeCallback.get();
        action = std::make_shared<std::function<void()>>([propertyChangeCallback, propId, areaId] {
            std::vector<VehiclePropValue> values = {
                    {
                            .areaId = areaId,
                            .prop = propId,
                    },
            };
            (*propertyChangeCallback)(values);
        });
        // Store the action in a map so that we could remove the action later.
        mRecurrentActions[propId][areaId] = action;
    }

    // In mock implementation, we generate a new property change event for this property at sample
    // rate.
    int64_t interval = static_cast<int64_t>(1'000'000'000. / sampleRateHz);
    mRecurrentTimer->registerTimerCallback(interval, action);
    return StatusCode::OK;
}

StatusCode MockVehicleHardware::unsubscribe(int32_t propId, int32_t areaId) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    // For on-change property.
    mSubOnChangePropIdAreaIds.erase(std::make_pair(propId, areaId));
    // for continuous property.
    if (mRecurrentActions[propId][areaId] != nullptr) {
        // Remove the previous action register for this [propId, areaId].
        mRecurrentTimer->unregisterTimerCallback(mRecurrentActions[propId][areaId]);
        mRecurrentActions[propId].erase(areaId);
        if (mRecurrentActions[propId].empty()) {
            mRecurrentActions.erase(propId);
        }
    }
    return StatusCode::OK;
}

std::set<std::pair<int32_t, int32_t>> MockVehicleHardware::getSubscribedOnChangePropIdAreaIds() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::set<std::pair<int32_t, int32_t>> propIdAreaIds;
    propIdAreaIds = mSubOnChangePropIdAreaIds;
    return propIdAreaIds;
}

std::set<std::pair<int32_t, int32_t>> MockVehicleHardware::getSubscribedContinuousPropIdAreaIds() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::set<std::pair<int32_t, int32_t>> propIdAreaIds;
    for (const auto& [propId, actionByAreaId] : mRecurrentActions) {
        for (const auto& [areaId, _] : actionByAreaId) {
            propIdAreaIds.insert(std::make_pair(propId, areaId));
        }
    }
    return propIdAreaIds;
}

void MockVehicleHardware::registerOnPropertyChangeEvent(
        std::unique_ptr<const PropertyChangeCallback> callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mPropertyChangeCallback = std::move(callback);
}

void MockVehicleHardware::registerOnPropertySetErrorEvent(
        std::unique_ptr<const PropertySetErrorCallback> callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mPropertySetErrorCallback = std::move(callback);
}

void MockVehicleHardware::setPropertyConfigs(const std::vector<VehiclePropConfig>& configs) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mPropertyConfigs = configs;
}

void MockVehicleHardware::addGetValueResponses(const std::vector<GetValueResult>& responses) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mGetValueResponses.push_back(responses);
}

void MockVehicleHardware::addSetValueResponses(const std::vector<SetValueResult>& responses) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mSetValueResponses.push_back(responses);
}

void MockVehicleHardware::setGetValueResponder(
        std::function<StatusCode(std::shared_ptr<const GetValuesCallback>,
                                 const std::vector<GetValueRequest>&)>&& responder) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mGetValueResponder = responder;
}

std::vector<GetValueRequest> MockVehicleHardware::nextGetValueRequests() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::optional<std::vector<GetValueRequest>> request = pop(mGetValueRequests);
    if (!request.has_value()) {
        return std::vector<GetValueRequest>();
    }
    return std::move(request.value());
}

std::vector<SetValueRequest> MockVehicleHardware::nextSetValueRequests() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::optional<std::vector<SetValueRequest>> request = pop(mSetValueRequests);
    if (!request.has_value()) {
        return std::vector<SetValueRequest>();
    }
    return std::move(request.value());
}

void MockVehicleHardware::setStatus(const char* functionName, StatusCode status) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mStatusByFunctions[functionName] = status;
}

void MockVehicleHardware::setSleepTime(int64_t timeInNano) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mSleepTime = timeInNano;
}

void MockVehicleHardware::setPropertyOnChangeEventBatchingWindow(std::chrono::nanoseconds window) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    mEventBatchingWindow = window;
}

std::chrono::nanoseconds MockVehicleHardware::getPropertyOnChangeEventBatchingWindow() {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    return mEventBatchingWindow;
}

template <class ResultType>
StatusCode MockVehicleHardware::returnResponse(
        std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
        std::list<std::vector<ResultType>>* storedResponses) const {
    if (storedResponses->size() > 0) {
        (*callback)(std::move(storedResponses->front()));
        storedResponses->pop_front();
        return StatusCode::OK;
    } else {
        ALOGE("no more response");
        return StatusCode::INTERNAL_ERROR;
    }
}

template StatusCode MockVehicleHardware::returnResponse<GetValueResult>(
        std::shared_ptr<const std::function<void(std::vector<GetValueResult>)>> callback,
        std::list<std::vector<GetValueResult>>* storedResponses) const;

template StatusCode MockVehicleHardware::returnResponse<SetValueResult>(
        std::shared_ptr<const std::function<void(std::vector<SetValueResult>)>> callback,
        std::list<std::vector<SetValueResult>>* storedResponses) const;

template <class RequestType, class ResultType>
StatusCode MockVehicleHardware::handleRequestsLocked(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
        const std::vector<RequestType>& requests,
        std::list<std::vector<RequestType>>* storedRequests,
        std::list<std::vector<ResultType>>* storedResponses) const {
    storedRequests->push_back(requests);
    if (auto it = mStatusByFunctions.find(functionName); it != mStatusByFunctions.end()) {
        if (StatusCode status = it->second; status != StatusCode::OK) {
            return status;
        }
    }

    if (mSleepTime != 0) {
        int64_t sleepTime = mSleepTime;
        mThreadCount++;
        std::thread t([this, callback, sleepTime, storedResponses]() {
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
            returnResponse(callback, storedResponses);
            mThreadCount--;
            mCv.notify_one();
        });
        // Detach the thread here so we do not have to maintain the thread object. mThreadCount
        // and mCv make sure we wait for all threads to end before we exit.
        t.detach();
        return StatusCode::OK;

    } else {
        return returnResponse(callback, storedResponses);
    }
}

template StatusCode MockVehicleHardware::handleRequestsLocked<GetValueRequest, GetValueResult>(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<GetValueResult>)>> callback,
        const std::vector<GetValueRequest>& requests,
        std::list<std::vector<GetValueRequest>>* storedRequests,
        std::list<std::vector<GetValueResult>>* storedResponses) const;

template StatusCode MockVehicleHardware::handleRequestsLocked<SetValueRequest, SetValueResult>(
        const char* functionName,
        std::shared_ptr<const std::function<void(std::vector<SetValueResult>)>> callback,
        const std::vector<SetValueRequest>& requests,
        std::list<std::vector<SetValueRequest>>* storedRequests,
        std::list<std::vector<SetValueResult>>* storedResponses) const;

void MockVehicleHardware::sendOnPropertySetErrorEvent(
        const std::vector<SetValueErrorEvent>& errorEvents) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    (*mPropertySetErrorCallback)(errorEvents);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
