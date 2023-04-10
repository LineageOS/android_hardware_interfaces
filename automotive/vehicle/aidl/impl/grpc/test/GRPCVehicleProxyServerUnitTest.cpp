// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GRPCVehicleHardware.h"
#include "GRPCVehicleProxyServer.h"
#include "IVehicleHardware.h"
#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace android::hardware::automotive::vehicle::virtualization {

const std::string kFakeServerAddr = "0.0.0.0:54321";

class VehicleHardwareForTest : public IVehicleHardware {
  public:
    void registerOnPropertyChangeEvent(
            std::unique_ptr<const PropertyChangeCallback> callback) override {
        mOnProp = std::move(callback);
    }

    void onPropertyEvent(
            std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropValue> values) {
        if (mOnProp) {
            (*mOnProp)(std::move(values));
        }
    }

    // Functions that we do not care.
    std::vector<aidl::android::hardware::automotive::vehicle::VehiclePropConfig>
    getAllPropertyConfigs() const override {
        return {};
    }

    aidl::android::hardware::automotive::vehicle::StatusCode setValues(
            std::shared_ptr<const SetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::SetValueRequest>&
                    requests) override {
        return aidl::android::hardware::automotive::vehicle::StatusCode::OK;
    }

    aidl::android::hardware::automotive::vehicle::StatusCode getValues(
            std::shared_ptr<const GetValuesCallback> callback,
            const std::vector<aidl::android::hardware::automotive::vehicle::GetValueRequest>&
                    requests) const override {
        return aidl::android::hardware::automotive::vehicle::StatusCode::OK;
    }

    DumpResult dump(const std::vector<std::string>& options) override { return {}; }

    aidl::android::hardware::automotive::vehicle::StatusCode checkHealth() override {
        return aidl::android::hardware::automotive::vehicle::StatusCode::OK;
    }

    void registerOnPropertySetErrorEvent(
            std::unique_ptr<const PropertySetErrorCallback> callback) override {}

  private:
    std::unique_ptr<const PropertyChangeCallback> mOnProp;
};

TEST(GRPCVehicleProxyServerUnitTest, ClientConnectDisconnect) {
    auto testHardware = std::make_unique<VehicleHardwareForTest>();
    // HACK: manipulate the underlying hardware via raw pointer for testing.
    auto* testHardwareRaw = testHardware.get();
    auto vehicleServer =
            std::make_unique<GrpcVehicleProxyServer>(kFakeServerAddr, std::move(testHardware));
    vehicleServer->Start();

    constexpr auto kWaitForConnectionMaxTime = std::chrono::seconds(5);
    constexpr auto kWaitForStreamStartTime = std::chrono::seconds(1);
    constexpr auto kWaitForUpdateDeliveryTime = std::chrono::milliseconds(100);

    auto updateReceived1 = std::make_shared<bool>(false);
    auto vehicleHardware1 = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware1->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [updateReceived1](const auto&) { *updateReceived1 = true; }));
    EXPECT_TRUE(vehicleHardware1->waitForConnected(kWaitForConnectionMaxTime));
    std::this_thread::sleep_for(kWaitForStreamStartTime);

    // Client hardware 1 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    testHardwareRaw->onPropertyEvent({});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_TRUE(*updateReceived1);

    // Reset.
    *updateReceived1 = false;

    auto updateReceived2 = std::make_shared<bool>(false);
    auto vehicleHardware2 = std::make_unique<GRPCVehicleHardware>(kFakeServerAddr);
    vehicleHardware2->registerOnPropertyChangeEvent(
            std::make_unique<const IVehicleHardware::PropertyChangeCallback>(
                    [updateReceived2](const auto&) { *updateReceived2 = true; }));
    EXPECT_TRUE(vehicleHardware2->waitForConnected(kWaitForConnectionMaxTime));
    std::this_thread::sleep_for(kWaitForStreamStartTime);

    // Both client hardware 1 and 2 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    EXPECT_FALSE(*updateReceived2);
    testHardwareRaw->onPropertyEvent({});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_TRUE(*updateReceived1);
    EXPECT_TRUE(*updateReceived2);

    // Reset.
    *updateReceived1 = false;
    *updateReceived2 = false;

    vehicleHardware1.reset();

    // Client 1 exited, only client hardware 2 received update from the server.
    EXPECT_FALSE(*updateReceived1);
    EXPECT_FALSE(*updateReceived2);
    testHardwareRaw->onPropertyEvent({});
    // Wait for the update delivery.
    std::this_thread::sleep_for(kWaitForUpdateDeliveryTime);
    EXPECT_FALSE(*updateReceived1);
    EXPECT_TRUE(*updateReceived2);

    vehicleServer->Shutdown().Wait();
}

}  // namespace android::hardware::automotive::vehicle::virtualization
