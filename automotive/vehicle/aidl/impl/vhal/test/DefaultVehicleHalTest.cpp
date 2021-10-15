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

#include "DefaultVehicleHal.h"

#include <IVehicleHardware.h>
#include <LargeParcelableBase.h>
#include <aidl/android/hardware/automotive/vehicle/IVehicle.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::IVehicle;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

using ::android::automotive::car_binder_lib::LargeParcelableBase;
using ::android::base::Result;

using ::testing::Eq;
using ::testing::WhenSortedBy;

class MockVehicleHardware final : public IVehicleHardware {
  public:
    std::vector<VehiclePropConfig> getAllPropertyConfigs() const override {
        return mPropertyConfigs;
    }

    StatusCode setValues(std::function<void(const std::vector<SetValueResult>&)>&&,
                         const std::vector<SetValueRequest>&) override {
        // TODO(b/200737967): mock this.
        return StatusCode::OK;
    }

    StatusCode getValues(std::function<void(const std::vector<GetValueResult>&)>&&,
                         const std::vector<GetValueRequest>&) const override {
        // TODO(b/200737967): mock this.
        return StatusCode::OK;
    }

    DumpResult dump(const std::vector<std::string>&) override {
        // TODO(b/200737967): mock this.
        return DumpResult{};
    }

    StatusCode checkHealth() override {
        // TODO(b/200737967): mock this.
        return StatusCode::OK;
    }

    void registerOnPropertyChangeEvent(
            std::function<void(const std::vector<VehiclePropValue>&)>&&) override {
        // TODO(b/200737967): mock this.
    }

    void registerOnPropertySetErrorEvent(
            std::function<void(const std::vector<SetValueErrorEvent>&)>&&) override {
        // TODO(b/200737967): mock this.
    }

    // Test functions.
    void setPropertyConfigs(const std::vector<VehiclePropConfig>& configs) {
        mPropertyConfigs = configs;
    }

  private:
    std::vector<VehiclePropConfig> mPropertyConfigs;
};

struct PropConfigCmp {
    bool operator()(const VehiclePropConfig& a, const VehiclePropConfig& b) const {
        return (a.prop < b.prop);
    }
} propConfigCmp;

}  // namespace

TEST(DefaultVehicleHalTest, testGetAllPropConfigsSmall) {
    auto testConfigs = std::vector<VehiclePropConfig>({
            VehiclePropConfig{
                    .prop = 1,
            },
            VehiclePropConfig{
                    .prop = 2,
            },
    });

    auto hardware = std::make_unique<MockVehicleHardware>();
    hardware->setPropertyConfigs(testConfigs);
    auto vhal = ::ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getAllPropConfigs(&output);

    ASSERT_TRUE(status.isOk());
    ASSERT_THAT(output.payloads, WhenSortedBy(propConfigCmp, Eq(testConfigs)));
}

TEST(DefaultVehicleHalTest, testGetAllPropConfigsLarge) {
    std::vector<VehiclePropConfig> testConfigs;
    // 10000 VehiclePropConfig exceeds 4k memory limit, so it would be sent through shared memory.
    for (size_t i = 0; i < 10000; i++) {
        testConfigs.push_back(VehiclePropConfig{
                .prop = static_cast<int32_t>(i),
        });
    }

    auto hardware = std::make_unique<MockVehicleHardware>();
    hardware->setPropertyConfigs(testConfigs);
    auto vhal = ::ndk::SharedRefBase::make<DefaultVehicleHal>(std::move(hardware));
    std::shared_ptr<IVehicle> client = IVehicle::fromBinder(vhal->asBinder());

    VehiclePropConfigs output;
    auto status = client->getAllPropConfigs(&output);

    ASSERT_TRUE(status.isOk());
    ASSERT_TRUE(output.payloads.empty());
    Result<std::optional<std::vector<VehiclePropConfig>>> result =
            LargeParcelableBase::stableLargeParcelableToParcelableVector<VehiclePropConfig>(
                    output.sharedMemoryFd);
    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.value().has_value());
    ASSERT_EQ(result.value().value(), testConfigs);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
