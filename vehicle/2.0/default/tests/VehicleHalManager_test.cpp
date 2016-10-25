/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <unordered_map>
#include <iostream>

#include <gtest/gtest.h>

#include <vehicle_hal_manager/VehiclePropConfigIndex.h>
#include <VehicleHal.h>
#include <vehicle_hal_manager/VehicleHalManager.h>
#include "vehicle_hal_manager/SubscriptionManager.h"

#include "VehicleHalTestUtils.h"

namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {

namespace {

using namespace std::placeholders;

class MockedVehicleHal : public VehicleHal {
public:
    MockedVehicleHal() {
        mConfigs.assign(std::begin(kVehicleProperties),
                        std::end(kVehicleProperties));
    }

    std::vector<VehiclePropConfig> listProperties() override {
        return mConfigs;
    }

    VehiclePropValuePtr get(VehicleProperty property,
                 int32_t areaId,
                 status_t* outStatus) override {
        *outStatus = OK;
        return getValuePool()->obtain(mValues[property]);
    }

    status_t set(const VehiclePropValue& propValue) override {
        mValues[propValue.prop] = propValue;
        return OK;
    }

    status_t subscribe(VehicleProperty property,
                       int32_t areas,
                       float sampleRate) override {
        return OK;
    }

    status_t unsubscribe(VehicleProperty property) override {
        return OK;
    }

    void sendPropEvent(recyclable_ptr<VehiclePropValue> value) {
        doHalEvent(std::move(value));
    }

private:
    std::vector<VehiclePropConfig> mConfigs;
    std::unordered_map<VehicleProperty, VehiclePropValue> mValues;
};

class VehicleHalManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        hal.reset(new MockedVehicleHal);
        manager.reset(new VehicleHalManager(hal.get()));

        objectPool = hal->getValuePool();
    }

    void TearDown() override {
        manager.reset(nullptr);
        hal.reset(nullptr);
    }

public:
    VehiclePropValuePool* objectPool;
    std::unique_ptr<MockedVehicleHal> hal;
    std::unique_ptr<VehicleHalManager> manager;
};

class HalClientVectorTest : public ::testing::Test {
public:
    HalClientVector clients;
};

TEST_F(VehicleHalManagerTest, getPropConfigs) {
    hidl_vec<VehicleProperty> properties = init_hidl_vec(
        { VehicleProperty::HVAC_FAN_SPEED,VehicleProperty::INFO_MAKE} );
    bool called = false;
    manager->getPropConfigs(properties,
                            [&called] (const hidl_vec<VehiclePropConfig>& c) {
        ASSERT_EQ(2u, c.size());
        called = true;
    });
    ASSERT_TRUE(called);  // Verify callback received.

    called = false;
    manager->getPropConfigs(init_hidl_vec({VehicleProperty::HVAC_FAN_SPEED}),
                            [&called] (const hidl_vec<VehiclePropConfig>& c) {
        ASSERT_EQ(1u, c.size());
        ASSERT_EQ(toString(kVehicleProperties[1]), toString(c[0]));
        called = true;
    });
    ASSERT_TRUE(called);  // Verify callback received.
}

TEST_F(VehicleHalManagerTest, getAllPropConfigs) {
    bool called = false;
    manager->getAllPropConfigs(
            [&called] (const hidl_vec<VehiclePropConfig>& propConfigs) {
        ASSERT_EQ(arraysize(kVehicleProperties), propConfigs.size());

        for (size_t i = 0; i < propConfigs.size(); i++) {
            ASSERT_EQ(toString(kVehicleProperties[i]),
                      toString(propConfigs[i]));
        }
        called = true;
    });
    ASSERT_TRUE(called);  // Verify callback received.
}

TEST_F(VehicleHalManagerTest, subscribe) {
    const VehicleProperty PROP = VehicleProperty::DISPLAY_BRIGHTNESS;

    sp<MockedVehicleCallback> cb = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> options = init_hidl_vec(
        {
            SubscribeOptions {
                .propId = PROP,
                .flags = SubscribeFlags::DEFAULT
            },
        });

    StatusCode res = manager->subscribe(cb, options);
    ASSERT_EQ(StatusCode::OK, res);

    auto unsubscribedValue = objectPool->obtain(VehiclePropertyType::INT32);
    unsubscribedValue->prop = VehicleProperty::HVAC_FAN_SPEED;

    hal->sendPropEvent(std::move(unsubscribedValue));
    auto& receivedEnvents = cb->getReceivedEvents();

    ASSERT_TRUE(cb->waitForExpectedEvents(0)) << " Unexpected events received: "
              << receivedEnvents.size()
              << (receivedEnvents.size() > 0
                      ? toString(receivedEnvents.front()[0]) : "");

    auto subscribedValue = objectPool->obtain(VehiclePropertyType::INT32);
    subscribedValue->prop = PROP;
    subscribedValue->value.int32Values[0] = 42;

    cb->reset();
    VehiclePropValue actualValue(*subscribedValue.get());
    hal->sendPropEvent(std::move(subscribedValue));

    ASSERT_TRUE(cb->waitForExpectedEvents(1)) << "Events received: "
            << receivedEnvents.size();

    ASSERT_EQ(toString(actualValue),
              toString(cb->getReceivedEvents().front()[0]));
}

TEST_F(HalClientVectorTest, basic) {
    sp<IVehicleCallback> callback1 = new MockedVehicleCallback();

    sp<HalClient> c1 = new HalClient(callback1, 10, 20);
    sp<HalClient> c2 = new HalClient(callback1, 10, 20);

    clients.addOrUpdate(c1);
    clients.addOrUpdate(c1);
    clients.addOrUpdate(c2);
    ASSERT_EQ(2u, clients.size());
    ASSERT_FALSE(clients.isEmpty());
    ASSERT_GE(0, clients.indexOf(c1));
    ASSERT_GE(0, clients.remove(c1));
    ASSERT_GE(0, clients.indexOf(c1));
    ASSERT_GE(0, clients.remove(c1));
    ASSERT_GE(0, clients.remove(c2));

    ASSERT_TRUE(clients.isEmpty());
}

}  // namespace anonymous

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
