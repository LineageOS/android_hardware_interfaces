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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleHardware_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleHardware_H_

#include <IVehicleHardware.h>
#include <RecurrentTimer.h>
#include <VehicleHalTypes.h>

#include <android-base/thread_annotations.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

class MockVehicleHardware final : public IVehicleHardware {
  public:
    MockVehicleHardware();

    ~MockVehicleHardware();

    std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
    getAllPropertyConfigs() const override;
    aidl::android::hardware::automotive::vehicle::StatusCode setValues(
            std::shared_ptr<const SetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::SetValueRequest>&
                    requests) override;
    aidl::android::hardware::automotive::vehicle::StatusCode getValues(
            std::shared_ptr<const GetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>&
                    requests) const override;
    DumpResult dump(const std::vector<std::string>&) override;
    aidl::android::hardware::automotive::vehicle::StatusCode checkHealth() override;
    void registerOnPropertyChangeEvent(
            std::unique_ptr<const PropertyChangeCallback> callback) override;
    void registerOnPropertySetErrorEvent(std::unique_ptr<const PropertySetErrorCallback>) override;
    aidl::android::hardware::automotive::vehicle::StatusCode subscribe(
            aidl::android::hardware::automotive::vehicle::SubscribeOptions options) override;
    aidl::android::hardware::automotive::vehicle::StatusCode unsubscribe(int32_t propId,
                                                                         int32_t areaId) override;
    std::chrono::nanoseconds getPropertyOnChangeEventBatchingWindow() override;

    // Test functions.
    void setPropertyConfigs(
            const std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropConfig>&
                    configs);
    void addGetValueResponses(
            const std::vector<aidl::android::hardware::automotive::vehicle::GetValueResult>&
                    responses);
    void addSetValueResponses(
            const std::vector<aidl::android::hardware::automotive::vehicle::SetValueResult>&
                    responses);
    void setGetValueResponder(
            std::function<aidl::android::hardware::automotive::vehicle::StatusCode(
                    std::shared_ptr<const GetValuesCallback>,
                    const std::vector<
                            aidl::android::hardware::automotive::vehicle::GetValueRequest>&)>&&
                    responder);
    std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>
    nextGetValueRequests();
    std::vector<aidl::android::hardware::automotive::vehicle::SetValueRequest>
    nextSetValueRequests();
    void setStatus(const char* functionName,
                   aidl::android::hardware::automotive::vehicle::StatusCode status);
    void setSleepTime(int64_t timeInNano);
    void setDumpResult(DumpResult result);
    void sendOnPropertySetErrorEvent(const std::vector<SetValueErrorEvent>& errorEvents);
    void setPropertyOnChangeEventBatchingWindow(std::chrono::nanoseconds window);

    std::set<std::pair<int32_t, int32_t>> getSubscribedOnChangePropIdAreaIds();
    std::set<std::pair<int32_t, int32_t>> getSubscribedContinuousPropIdAreaIds();
    std::vector<aidl::android::hardware::automotive::vehicle::SubscribeOptions>
    getSubscribeOptions();
    void clearSubscribeOptions();

  private:
    mutable std::mutex mLock;
    mutable std::condition_variable mCv;
    mutable std::atomic<int> mThreadCount;
    std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropConfig> mPropertyConfigs
            GUARDED_BY(mLock);
    mutable std::list<std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>>
            mGetValueRequests GUARDED_BY(mLock);
    mutable std::list<std::vector<aidl::android::hardware::automotive::vehicle::GetValueResult>>
            mGetValueResponses GUARDED_BY(mLock);
    mutable std::list<std::vector<aidl::android::hardware::automotive::vehicle::SetValueRequest>>
            mSetValueRequests GUARDED_BY(mLock);
    mutable std::list<std::vector<aidl::android::hardware::automotive::vehicle::SetValueResult>>
            mSetValueResponses GUARDED_BY(mLock);
    std::unordered_map<const char*, aidl::android::hardware::automotive::vehicle::StatusCode>
            mStatusByFunctions GUARDED_BY(mLock);
    int64_t mSleepTime GUARDED_BY(mLock) = 0;
    std::unique_ptr<const PropertyChangeCallback> mPropertyChangeCallback GUARDED_BY(mLock);
    std::unique_ptr<const PropertySetErrorCallback> mPropertySetErrorCallback GUARDED_BY(mLock);
    std::function<aidl::android::hardware::automotive::vehicle::StatusCode(
            std::shared_ptr<const GetValuesCallback>,
            const std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>&)>
            mGetValueResponder GUARDED_BY(mLock);
    std::chrono::nanoseconds mEventBatchingWindow GUARDED_BY(mLock) = std::chrono::nanoseconds(0);
    std::set<std::pair<int32_t, int32_t>> mSubOnChangePropIdAreaIds GUARDED_BY(mLock);
    std::vector<aidl::android::hardware::automotive::vehicle::SubscribeOptions> mSubscribeOptions
            GUARDED_BY(mLock);

    template <class ResultType>
    aidl::android::hardware::automotive::vehicle::StatusCode returnResponse(
            std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
            std::list<std::vector<ResultType>>* storedResponses) const;
    template <class RequestType, class ResultType>
    aidl::android::hardware::automotive::vehicle::StatusCode handleRequestsLocked(
            const char* functionName,
            std::shared_ptr<const std::function<void(std::vector<ResultType>)>> callback,
            const std::vector<RequestType>& requests,
            std::list<std::vector<RequestType>>* storedRequests,
            std::list<std::vector<ResultType>>* storedResponses) const REQUIRES(mLock);
    aidl::android::hardware::automotive::vehicle::StatusCode subscribePropIdAreaId(
            int32_t propId, int32_t areaId, float sampleRateHz);

    DumpResult mDumpResult;

    // RecurrentTimer is thread-safe.
    std::shared_ptr<RecurrentTimer> mRecurrentTimer;
    std::unordered_map<int32_t, std::unordered_map<int32_t, std::shared_ptr<std::function<void()>>>>
            mRecurrentActions GUARDED_BY(mLock);
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_test_MockVehicleHardware_H_
