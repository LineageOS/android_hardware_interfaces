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

#include "SubscriptionManager.h"

#include <MockVehicleHardware.h>
#include <VehicleHalTypes.h>

#include <aidl/android/hardware/automotive/vehicle/BnVehicleCallback.h>
#include <android-base/thread_annotations.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <float.h>
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::BnVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::GetValueResults;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueResults;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropErrors;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValues;
using ::ndk::ScopedAStatus;
using ::ndk::SpAIBinder;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

class PropertyCallback final : public BnVehicleCallback {
  public:
    ScopedAStatus onGetValues(const GetValueResults&) override { return ScopedAStatus::ok(); }

    ScopedAStatus onSetValues(const SetValueResults&) override { return ScopedAStatus::ok(); }

    ScopedAStatus onPropertyEvent(const VehiclePropValues& values, int32_t) override {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        for (const auto& value : values.payloads) {
            mEvents.push_back(value);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus onPropertySetError(const VehiclePropErrors&) override {
        return ScopedAStatus::ok();
    }

    // Test functions.
    std::list<VehiclePropValue> getEvents() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        return mEvents;
    }

    void clearEvents() {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mEvents.clear();
    }

  private:
    std::mutex mLock;
    std::list<VehiclePropValue> mEvents GUARDED_BY(mLock);
};

class SubscriptionManagerTest : public testing::Test {
  public:
    void SetUp() override {
        mHardware = std::make_shared<MockVehicleHardware>();
        mManager = std::make_unique<SubscriptionManager>(mHardware.get());
        mCallback = ndk::SharedRefBase::make<PropertyCallback>();
        // Keep the local binder alive.
        mBinder = mCallback->asBinder();
        mCallbackClient = IVehicleCallback::fromBinder(mBinder);
        std::shared_ptr<IVehicleCallback> callbackClient = mCallbackClient;
        mHardware->registerOnPropertyChangeEvent(
                std::make_unique<IVehicleHardware::PropertyChangeCallback>(
                        [callbackClient](std::vector<VehiclePropValue> updatedValues) {
                            VehiclePropValues values = {
                                    .payloads = std::move(updatedValues),
                            };
                            callbackClient->onPropertyEvent(values, 0);
                        }));
    }

    SubscriptionManager* getManager() { return mManager.get(); }

    std::shared_ptr<IVehicleCallback> getCallbackClient() { return mCallbackClient; }

    PropertyCallback* getCallback() { return mCallback.get(); }

    std::list<VehiclePropValue> getEvents() { return getCallback()->getEvents(); }

    void clearEvents() { return getCallback()->clearEvents(); }

    std::shared_ptr<MockVehicleHardware> getHardware() { return mHardware; }

  private:
    std::unique_ptr<SubscriptionManager> mManager;
    std::shared_ptr<PropertyCallback> mCallback;
    std::shared_ptr<IVehicleCallback> mCallbackClient;
    std::shared_ptr<MockVehicleHardware> mHardware;
    SpAIBinder mBinder;
};

TEST_F(SubscriptionManagerTest, testSubscribeGlobalContinuous) {
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
    }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribedContinuousPropIdAreaIds(),
                UnorderedElementsAre(std::pair<int32_t, int32_t>(0, 0)));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    ASSERT_GE(getEvents().size(), static_cast<size_t>(9));
    EXPECT_EQ(getEvents().back().prop, 0);
    EXPECT_EQ(getEvents().back().areaId, 0);
}

TEST_F(SubscriptionManagerTest, testSubscribeMultiplePropsGlobalContinuous) {
    std::vector<SubscribeOptions> options = {{
                                                     .propId = 0,
                                                     .areaIds = {0},
                                                     .sampleRate = 10.0,
                                             },
                                             {
                                                     .propId = 1,
                                                     .areaIds = {0},
                                                     .sampleRate = 20.0,
                                             }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    size_t event0Count = 0;
    size_t event1Count = 0;

    for (const auto& event : getEvents()) {
        if (event.prop == 0) {
            event0Count++;
        } else {
            event1Count++;
        }
    }

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    EXPECT_GE(event0Count, static_cast<size_t>(9));
    // Theoretically trigger 20 times, but check for at least 15 times to be stable.
    EXPECT_GE(event1Count, static_cast<size_t>(15));
}

TEST_F(SubscriptionManagerTest, testOverrideSubscriptionContinuous) {
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 20.0,
    }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    // Override sample rate to be 10.0.
    options[0].sampleRate = 10.0;
    result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    EXPECT_GE(getEvents().size(), static_cast<size_t>(9));
    EXPECT_LE(getEvents().size(), static_cast<size_t>(15));
}

TEST_F(SubscriptionManagerTest, testSubscribeMultipleAreasContinuous) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1},
                    .sampleRate = 10.0,
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    size_t area0Count = 0;
    size_t area1Count = 0;

    for (const auto& event : getEvents()) {
        if (event.areaId == 0) {
            area0Count++;
        } else {
            area1Count++;
        }
    }

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    EXPECT_GE(area0Count, static_cast<size_t>(9));
    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    EXPECT_GE(area1Count, static_cast<size_t>(9));
}

TEST_F(SubscriptionManagerTest, testUnsubscribeGlobalContinuous) {
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 100.0,
    }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    result = getManager()->unsubscribe(getCallbackClient()->asBinder().get());
    ASSERT_TRUE(result.ok()) << "failed to unsubscribe: " << result.error().message();

    ASSERT_EQ(getHardware()->getSubscribedContinuousPropIdAreaIds().size(), 0u);

    // Wait for the last events to come.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    clearEvents();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_TRUE(getEvents().empty());
}

TEST_F(SubscriptionManagerTest, testUnsubscribeMultipleAreas) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1, 2, 3, 4},
                    .sampleRate = 10.0,
            },
            {
                    .propId = 1,
                    .areaIds = {0},
                    .sampleRate = 10.0,
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    result = getManager()->unsubscribe(getCallbackClient()->asBinder().get(),
                                       std::vector<int32_t>({0}));
    ASSERT_TRUE(result.ok()) << "failed to unsubscribe: " << result.error().message();

    // Wait for the last events to come.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    clearEvents();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Theoretically trigger 10 times, but check for at least 9 times to be stable.
    EXPECT_GE(getEvents().size(), static_cast<size_t>(9));

    for (const auto& event : getEvents()) {
        EXPECT_EQ(event.prop, 1);
    }
}

TEST_F(SubscriptionManagerTest, testUnsubscribeByCallback) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1, 2, 3, 4},
                    .sampleRate = 10.0,
            },
            {
                    .propId = 1,
                    .areaIds = {0},
                    .sampleRate = 10.0,
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    result = getManager()->unsubscribe(getCallbackClient()->asBinder().get());
    ASSERT_TRUE(result.ok()) << "failed to unsubscribe: " << result.error().message();

    // Wait for the last events to come.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    clearEvents();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_TRUE(getEvents().empty());
}

TEST_F(SubscriptionManagerTest, testUnsubscribeUnsubscribedPropId) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1, 2, 3, 4},
            },
            {
                    .propId = 1,
                    .areaIds = {0},
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, false);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    // Property ID: 2 was not subscribed.
    result = getManager()->unsubscribe(getCallbackClient()->asBinder().get(),
                                       std::vector<int32_t>({0, 1, 2}));
    ASSERT_TRUE(result.ok()) << "unsubscribe an unsubscribed property must do nothing";

    std::vector<VehiclePropValue> updatedValues = {
            {
                    .prop = 0,
                    .areaId = 0,
            },
            {
                    .prop = 1,
                    .areaId = 0,
            },
    };
    auto clients = getManager()->getSubscribedClients(std::vector<VehiclePropValue>(updatedValues));

    ASSERT_EQ(clients.size(), 0u) << "all subscribed properties must be unsubscribed";
}

TEST_F(SubscriptionManagerTest, testSubscribeOnchange) {
    std::vector<SubscribeOptions> options1 = {
            {
                    .propId = 0,
                    .areaIds = {0, 1},
            },
            {
                    .propId = 1,
                    .areaIds = {0},
            },
    };
    std::vector<SubscribeOptions> options2 = {
            {
                    .propId = 0,
                    .areaIds = {0},
            },
    };

    SpAIBinder binder1 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client1 = IVehicleCallback::fromBinder(binder1);
    SpAIBinder binder2 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client2 = IVehicleCallback::fromBinder(binder2);
    auto result = getManager()->subscribe(client1, options1, false);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();
    result = getManager()->subscribe(client2, options2, false);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();
    ASSERT_THAT(getHardware()->getSubscribedOnChangePropIdAreaIds(),
                UnorderedElementsAre(std::pair<int32_t, int32_t>(0, 0),
                                     std::pair<int32_t, int32_t>(0, 1),
                                     std::pair<int32_t, int32_t>(1, 0)));
    ASSERT_EQ(getHardware()->getSubscribedContinuousPropIdAreaIds().size(), 0u);

    std::vector<VehiclePropValue> updatedValues = {
            {
                    .prop = 0,
                    .areaId = 0,
            },
            {
                    .prop = 0,
                    .areaId = 1,
            },
            {
                    .prop = 1,
                    .areaId = 0,
            },
            {
                    .prop = 1,
                    .areaId = 1,
            },
    };
    auto clients = getManager()->getSubscribedClients(std::vector<VehiclePropValue>(updatedValues));

    ASSERT_THAT(clients[client1],
                UnorderedElementsAre(updatedValues[0], updatedValues[1], updatedValues[2]));
    ASSERT_THAT(clients[client2], ElementsAre(updatedValues[0]));
}

TEST_F(SubscriptionManagerTest, testSubscribeInvalidOption) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1, 2, 3, 4},
                    // invalid sample rate.
                    .sampleRate = 0.0,
            },
            {
                    .propId = 1,
                    .areaIds = {0},
                    .sampleRate = 10.0,
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_FALSE(result.ok()) << "subscribe with invalid sample rate must fail";
    ASSERT_TRUE(getManager()
                        ->getSubscribedClients({{
                                                        .prop = 0,
                                                        .areaId = 0,
                                                },
                                                {
                                                        .prop = 1,
                                                        .areaId = 0,
                                                }})
                        .empty())
            << "no property should be subscribed if error is returned";
}

TEST_F(SubscriptionManagerTest, testSubscribeNoAreaIds) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {},
                    .sampleRate = 1.0,
            },
            {
                    .propId = 1,
                    .areaIds = {0},
                    .sampleRate = 10.0,
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_FALSE(result.ok()) << "subscribe with invalid sample rate must fail";
    ASSERT_TRUE(getManager()
                        ->getSubscribedClients({{
                                .prop = 1,
                                .areaId = 0,
                        }})
                        .empty())
            << "no property should be subscribed if error is returned";
}

TEST_F(SubscriptionManagerTest, testUnsubscribeOnchange) {
    std::vector<SubscribeOptions> options = {
            {
                    .propId = 0,
                    .areaIds = {0, 1},
            },
            {
                    .propId = 1,
                    .areaIds = {0},
            },
    };

    auto result = getManager()->subscribe(getCallbackClient(), options, false);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    result = getManager()->unsubscribe(getCallbackClient()->asBinder().get(),
                                       std::vector<int32_t>({0}));
    ASSERT_TRUE(result.ok()) << "failed to unsubscribe: " << result.error().message();

    std::vector<VehiclePropValue> updatedValues = {
            {
                    .prop = 0,
                    .areaId = 0,
            },
            {
                    .prop = 1,
                    .areaId = 0,
            },
    };
    auto clients = getManager()->getSubscribedClients(std::vector<VehiclePropValue>(updatedValues));

    ASSERT_THAT(clients[getCallbackClient()], ElementsAre(updatedValues[1]));
    ASSERT_THAT(getHardware()->getSubscribedOnChangePropIdAreaIds(),
                UnorderedElementsAre(std::pair<int32_t, int32_t>(1, 0)));
}

TEST_F(SubscriptionManagerTest, testCheckSampleRateHzValid) {
    ASSERT_TRUE(SubscriptionManager::checkSampleRateHz(1.0));
}

TEST_F(SubscriptionManagerTest, testCheckSampleRateHzInvalidTooSmall) {
    ASSERT_FALSE(SubscriptionManager::checkSampleRateHz(FLT_MIN));
}

TEST_F(SubscriptionManagerTest, testCheckSampleRateHzInvalidZero) {
    ASSERT_FALSE(SubscriptionManager::checkSampleRateHz(0));
}

TEST_F(SubscriptionManagerTest, testSubscribe_enableVur) {
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = true,
    }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(), ElementsAre(options[0]));
}

TEST_F(SubscriptionManagerTest, testSubscribe_VurStateChange) {
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = true,
    }};

    auto result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(), ElementsAre(options[0]));

    getHardware()->clearSubscribeOptions();
    result = getManager()->subscribe(getCallbackClient(), options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_TRUE(getHardware()->getSubscribeOptions().empty());

    std::vector<SubscribeOptions> newOptions = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = false,
    }};
    result = getManager()->subscribe(getCallbackClient(), newOptions, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(), ElementsAre(newOptions[0]));
}

TEST_F(SubscriptionManagerTest, testSubscribe_enableVur_filterUnchangedEvents) {
    SpAIBinder binder1 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client1 = IVehicleCallback::fromBinder(binder1);
    SpAIBinder binder2 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client2 = IVehicleCallback::fromBinder(binder2);
    SubscribeOptions client1Option = {
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = false,
    };
    auto result = getManager()->subscribe(client1, {client1Option}, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(), UnorderedElementsAre(client1Option));

    getHardware()->clearSubscribeOptions();
    SubscribeOptions client2Option = {
            .propId = 0,
            .areaIds = {0, 1},
            .sampleRate = 20.0,
            .enableVariableUpdateRate = true,
    };

    result = getManager()->subscribe(client2, {client2Option}, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(),
                UnorderedElementsAre(
                        SubscribeOptions{
                                .propId = 0,
                                .areaIds = {0},
                                .sampleRate = 20.0,
                                // This is enabled for client2, but disabled for client1.
                                .enableVariableUpdateRate = false,
                        },
                        SubscribeOptions{
                                .propId = 0,
                                .areaIds = {1},
                                .sampleRate = 20.0,
                                .enableVariableUpdateRate = true,
                        }));

    std::vector<VehiclePropValue> propertyEvents = {{
                                                            .prop = 0,
                                                            .areaId = 0,
                                                            .value = {.int32Values = {0}},
                                                            .timestamp = 1,
                                                    },
                                                    {
                                                            .prop = 0,
                                                            .areaId = 1,
                                                            .value = {.int32Values = {1}},
                                                            .timestamp = 1,
                                                    }};
    auto clients =
            getManager()->getSubscribedClients(std::vector<VehiclePropValue>(propertyEvents));

    ASSERT_THAT(clients[client1], UnorderedElementsAre(propertyEvents[0]));
    ASSERT_THAT(clients[client2], UnorderedElementsAre(propertyEvents[0], propertyEvents[1]));

    // If the same property events happen again with a new timestamp.
    // VUR is disabled for client1, enabled for client2.
    clients = getManager()->getSubscribedClients({{
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {0}},
            .timestamp = 2,
    }});

    ASSERT_FALSE(clients.find(client1) == clients.end())
            << "Must not filter out property events if VUR is not enabled";
    ASSERT_TRUE(clients.find(client2) == clients.end())
            << "Must filter out property events if VUR is enabled";
}

TEST_F(SubscriptionManagerTest, testSubscribe_enableVur_mustNotFilterStatusChange) {
    SpAIBinder binder1 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client1 = IVehicleCallback::fromBinder(binder1);
    SpAIBinder binder2 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client2 = IVehicleCallback::fromBinder(binder2);
    SubscribeOptions client1Option = {
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = false,
    };
    auto result = getManager()->subscribe(client1, {client1Option}, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(), UnorderedElementsAre(client1Option));

    getHardware()->clearSubscribeOptions();
    SubscribeOptions client2Option = {
            .propId = 0,
            .areaIds = {0, 1},
            .sampleRate = 20.0,
            .enableVariableUpdateRate = true,
    };

    result = getManager()->subscribe(client2, {client2Option}, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    ASSERT_THAT(getHardware()->getSubscribeOptions(),
                UnorderedElementsAre(
                        SubscribeOptions{
                                .propId = 0,
                                .areaIds = {0},
                                .sampleRate = 20.0,
                                // This is enabled for client2, but disabled for client1.
                                .enableVariableUpdateRate = false,
                        },
                        SubscribeOptions{
                                .propId = 0,
                                .areaIds = {1},
                                .sampleRate = 20.0,
                                .enableVariableUpdateRate = true,
                        }));

    VehiclePropValue propValue1 = {
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {0}},
            .timestamp = 1,
    };
    auto clients = getManager()->getSubscribedClients(std::vector<VehiclePropValue>({propValue1}));

    ASSERT_THAT(clients[client1], UnorderedElementsAre(propValue1));

    // A new event with the same value, but different status must not be filtered out.
    VehiclePropValue propValue2 = {
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {0}},
            .status = VehiclePropertyStatus::UNAVAILABLE,
            .timestamp = 2,
    };
    clients = getManager()->getSubscribedClients({propValue2});

    ASSERT_THAT(clients[client1], UnorderedElementsAre(propValue2))
            << "Must not filter out property events that has status change";
}

TEST_F(SubscriptionManagerTest, testSubscribe_enableVur_timestampUpdated_filterOutdatedEvent) {
    SpAIBinder binder1 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client1 = IVehicleCallback::fromBinder(binder1);
    SpAIBinder binder2 = ndk::SharedRefBase::make<PropertyCallback>()->asBinder();
    std::shared_ptr<IVehicleCallback> client2 = IVehicleCallback::fromBinder(binder2);
    std::vector<SubscribeOptions> options = {{
            .propId = 0,
            .areaIds = {0},
            .sampleRate = 10.0,
            .enableVariableUpdateRate = true,
    }};

    // client1 subscribe with VUR enabled.
    auto result = getManager()->subscribe(client1, options, true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    // Let client2 subscribe with VUR disabled so that we enabled VUR in DefaultVehicleHal layer.
    result = getManager()->subscribe(client2,
                                     {{
                                             .propId = 0,
                                             .areaIds = {0},
                                             .sampleRate = 10.0,
                                             .enableVariableUpdateRate = false,
                                     }},
                                     true);
    ASSERT_TRUE(result.ok()) << "failed to subscribe: " << result.error().message();

    VehiclePropValue value0 = {
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {0}},
            .timestamp = 1,
    };
    auto clients = getManager()->getSubscribedClients({value0});

    ASSERT_THAT(clients[client1], UnorderedElementsAre(value0));

    // A new event with the same value arrived. This must update timestamp to 3.
    VehiclePropValue value1 = {
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {0}},
            .timestamp = 3,
    };
    clients = getManager()->getSubscribedClients({value1});

    ASSERT_TRUE(clients.find(client1) == clients.end())
            << "Must filter out duplicate property events if VUR is enabled";

    // The latest timestamp is 3, so even though the value is not the same, this is outdated and
    // must be ignored.
    VehiclePropValue value2 = {
            .prop = 0,
            .areaId = 0,
            .value = {.int32Values = {1}},
            .timestamp = 2,
    };
    clients = getManager()->getSubscribedClients({value1});

    ASSERT_TRUE(clients.find(client1) == clients.end())
            << "Must filter out outdated property events if VUR is enabled";
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
