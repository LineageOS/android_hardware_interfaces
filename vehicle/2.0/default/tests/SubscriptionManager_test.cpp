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

class SubscriptionManagerTest : public ::testing::Test {
public:
    SubscriptionManager manager;

    const VehicleProperty PROP1 = VehicleProperty::HVAC_FAN_SPEED;
    const VehicleProperty PROP2 = VehicleProperty::DISPLAY_BRIGHTNESS;

    sp<IVehicleCallback> cb1 = new MockedVehicleCallback();
    sp<IVehicleCallback> cb2 = new MockedVehicleCallback();
    sp<IVehicleCallback> cb3 = new MockedVehicleCallback();

    hidl_vec<SubscribeOptions> subscrToProp1 = init_hidl_vec(
        {
            SubscribeOptions {
                .propId = PROP1,
                .vehicleAreas = val(VehicleAreaZone::ROW_1_LEFT),
                .flags = SubscribeFlags::HAL_EVENT
            },
        });

    hidl_vec<SubscribeOptions> subscrToProp2 = init_hidl_vec(
        {
            SubscribeOptions {
                .propId = PROP2,
                .flags = SubscribeFlags::HAL_EVENT
            },
        });

    hidl_vec<SubscribeOptions> subscrToProp1and2 = init_hidl_vec(
        {
            SubscribeOptions {
                .propId = PROP1,
                .vehicleAreas = val(VehicleAreaZone::ROW_1_LEFT),
                .flags = SubscribeFlags::HAL_EVENT
            },
            SubscribeOptions {
                .propId = PROP2,
                .flags = SubscribeFlags::HAL_EVENT
            },
        });

    static std::list<sp<IVehicleCallback>> extractCallbacks(
            const std::list<sp<HalClient>>& clients) {
        std::list<sp<IVehicleCallback>> callbacks;
        for (auto c : clients) {
            callbacks.push_back(c->getCallback());
        }
        return callbacks;
    }

    std::list<sp<HalClient>> clientsToProp1() {
        return manager.getSubscribedClients(PROP1,
                                            val(VehicleAreaZone::ROW_1_LEFT),
                                            SubscribeFlags::DEFAULT);
    }

    std::list<sp<HalClient>> clientsToProp2() {
        return manager.getSubscribedClients(PROP2, 0,
                                            SubscribeFlags::DEFAULT);
    }
};


TEST_F(SubscriptionManagerTest, multipleClients) {
    manager.addOrUpdateSubscription(cb1, subscrToProp1);
    manager.addOrUpdateSubscription(cb2, subscrToProp1);

    auto clients = manager.getSubscribedClients(
            PROP1,
            val(VehicleAreaZone::ROW_1_LEFT),
            SubscribeFlags::HAL_EVENT);

    ASSERT_ALL_EXISTS({cb1, cb2}, extractCallbacks(clients));
}

TEST_F(SubscriptionManagerTest, negativeCases) {
    manager.addOrUpdateSubscription(cb1, subscrToProp1);

    // Wrong zone
    auto clients = manager.getSubscribedClients(
            PROP1,
            val(VehicleAreaZone::ROW_2_LEFT),
            SubscribeFlags::HAL_EVENT);
    ASSERT_TRUE(clients.empty());

    // Wrong prop
    clients = manager.getSubscribedClients(
            VehicleProperty::AP_POWER_BOOTUP_REASON,
            val(VehicleAreaZone::ROW_1_LEFT),
            SubscribeFlags::HAL_EVENT);
    ASSERT_TRUE(clients.empty());

    // Wrong flag
    clients = manager.getSubscribedClients(
            PROP1,
            val(VehicleAreaZone::ROW_1_LEFT),
            SubscribeFlags::SET_CALL);
    ASSERT_TRUE(clients.empty());
}

TEST_F(SubscriptionManagerTest, mulipleSubscriptions) {
    manager.addOrUpdateSubscription(cb1, subscrToProp1);

    auto clients = manager.getSubscribedClients(
            PROP1,
            val(VehicleAreaZone::ROW_1_LEFT),
            SubscribeFlags::DEFAULT);
    ASSERT_EQ((size_t) 1, clients.size());
    ASSERT_EQ(cb1, clients.front()->getCallback());

    // Same property, but different zone, to make sure we didn't unsubscribe
    // from previous zone.
    manager.addOrUpdateSubscription(cb1, init_hidl_vec(
        {
            SubscribeOptions {
                .propId = PROP1,
                .vehicleAreas = val(VehicleAreaZone::ROW_2),
                .flags = SubscribeFlags::DEFAULT
            }
        }));

    clients = manager.getSubscribedClients(PROP1,
                                           val(VehicleAreaZone::ROW_1_LEFT),
                                           SubscribeFlags::DEFAULT);
    ASSERT_ALL_EXISTS({cb1}, extractCallbacks(clients));

    clients = manager.getSubscribedClients(PROP1,
                                           val(VehicleAreaZone::ROW_2),
                                           SubscribeFlags::DEFAULT);
    ASSERT_ALL_EXISTS({cb1}, extractCallbacks(clients));
}

TEST_F(SubscriptionManagerTest, unsubscribe) {
    manager.addOrUpdateSubscription(cb1, subscrToProp1);
    manager.addOrUpdateSubscription(cb2, subscrToProp2);
    manager.addOrUpdateSubscription(cb3, subscrToProp1and2);

    ASSERT_ALL_EXISTS({cb1, cb3}, extractCallbacks(clientsToProp1()));
    ASSERT_ALL_EXISTS({cb2, cb3}, extractCallbacks(clientsToProp2()));

    ASSERT_FALSE(manager.unsubscribe(cb1, PROP1));
    ASSERT_ALL_EXISTS({cb3}, extractCallbacks(clientsToProp1()));

    // Make sure nothing changed in PROP2 so far.
    ASSERT_ALL_EXISTS({cb2, cb3}, extractCallbacks(clientsToProp2()));

    // No one subscribed to PROP1, subscription for PROP2 is not affected.
    ASSERT_TRUE(manager.unsubscribe(cb3, PROP1));
    ASSERT_ALL_EXISTS({cb2, cb3}, extractCallbacks(clientsToProp2()));

    ASSERT_FALSE(manager.unsubscribe(cb3, PROP2));
    ASSERT_ALL_EXISTS({cb2}, extractCallbacks(clientsToProp2()));

    // The last client unsubscribed from this property.
    ASSERT_TRUE(manager.unsubscribe(cb2, PROP2));

    // No one was subscribed, return false.
    ASSERT_FALSE(manager.unsubscribe(cb1, PROP1));
}

}  // namespace anonymous

}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android
