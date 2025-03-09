/*
 * Copyright (C) 2023 The Android Open Source Project
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

#pragma once

#include <IVehicleHardware.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"

#include <grpc++/grpc++.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace android::hardware::automotive::vehicle::virtualization {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

class GRPCVehicleHardware : public IVehicleHardware {
  public:
    explicit GRPCVehicleHardware(std::string service_addr);

    ~GRPCVehicleHardware();

    // Get all the property configs.
    std::vector<aidlvhal::VehiclePropConfig> getAllPropertyConfigs() const override;

    // Get the config for the specified propId.
    std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
    getPropertyConfig(int32_t propId) const override;

    // Set property values asynchronously. Server could return before the property set requests
    // are sent to vehicle bus or before property set confirmation is received. The callback is
    // safe to be called after the function returns and is safe to be called in a different thread.
    aidlvhal::StatusCode setValues(std::shared_ptr<const SetValuesCallback> callback,
                                   const std::vector<aidlvhal::SetValueRequest>& requests) override;

    // Get property values asynchronously. Server could return before the property values are ready.
    // The callback is safe to be called after the function returns and is safe to be called in a
    // different thread.
    aidlvhal::StatusCode getValues(
            std::shared_ptr<const GetValuesCallback> callback,
            const std::vector<aidlvhal::GetValueRequest>& requests) const override;

    // Dump debug information in the server.
    DumpResult dump(const std::vector<std::string>& options) override;

    // Check whether the system is healthy, return {@code StatusCode::OK} for healthy.
    aidlvhal::StatusCode checkHealth() override;

    // Register a callback that would be called when there is a property change event from vehicle.
    void registerOnPropertyChangeEvent(
            std::unique_ptr<const PropertyChangeCallback> callback) override;

    // Register a callback that would be called when there is a property set error event from
    // vehicle.
    void registerOnPropertySetErrorEvent(
            std::unique_ptr<const PropertySetErrorCallback> callback) override;

    // Update the sample rate for the [propId, areaId] pair.
    aidlvhal::StatusCode updateSampleRate(int32_t propId, int32_t areaId,
                                          float sampleRate) override;

    aidlvhal::StatusCode subscribe(aidlvhal::SubscribeOptions options) override;

    aidlvhal::StatusCode unsubscribe(int32_t propId, int32_t areaId) override;

    bool waitForConnected(std::chrono::milliseconds waitTime);

  protected:
    std::shared_mutex mCallbackMutex;
    std::unique_ptr<const PropertyChangeCallback> mOnPropChange;

  private:
    friend class GRPCVehicleHardwareUnitTest;

    std::string mServiceAddr;
    std::shared_ptr<::grpc::Channel> mGrpcChannel;
    std::unique_ptr<proto::VehicleServer::StubInterface> mGrpcStub;
    std::thread mValuePollingThread;

    std::unique_ptr<const PropertySetErrorCallback> mOnSetErr;

    std::mutex mShutdownMutex;
    std::condition_variable mShutdownCV;
    std::atomic<bool> mShuttingDownFlag{false};

    mutable std::mutex mLatestUpdateTimestampsMutex;

    // A map from [propId, areaId] to the latest timestamp this property is updated.
    // The key is a tuple, the first element is the external timestamp (timestamp set by VHAL
    // server), the second element is the Android timestamp (elapsedRealtimeNano).
    mutable std::unordered_map<PropIdAreaId, std::pair<int64_t, int64_t>, PropIdAreaIdHash>
            mLatestUpdateTimestamps GUARDED_BY(mLatestUpdateTimestampsMutex);

    // Only used for unit testing.
    GRPCVehicleHardware(std::unique_ptr<proto::VehicleServer::StubInterface> stub,
                        bool startValuePollingLoop);

    void ValuePollingLoop();
    void pollValue();

    aidlvhal::StatusCode getValuesWithRetry(const std::vector<aidlvhal::GetValueRequest>& requests,
                                            std::vector<aidlvhal::GetValueResult>* results,
                                            size_t retryCount) const;

    // Check the external timestamp of propValue against the latest updated external timestamp, if
    // this is an outdated value, return false. Otherwise, update the external timestamp to the
    // Android timestamp and return true.
    bool setAndroidTimestamp(aidlvhal::VehiclePropValue* propValue) const;
};

}  // namespace android::hardware::automotive::vehicle::virtualization
