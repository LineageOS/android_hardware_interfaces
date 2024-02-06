/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "VtsHalAutomotiveVehicle"

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <utils/Log.h>
#include <unordered_set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <utils/SystemClock.h>

#include <chrono>
#include <thread>

using namespace android::hardware::automotive::vehicle::V2_0;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

constexpr auto kTimeout = std::chrono::milliseconds(500);
constexpr auto kInvalidProp = 0x31600207;
static constexpr auto kPropSetDelayMillis = std::chrono::milliseconds(10000);

class VtsVehicleCallback : public IVehicleCallback {
  private:
    using MutexGuard = std::lock_guard<std::mutex>;
    using HidlVecOfValues = hidl_vec<VehiclePropValue>;
    std::mutex mLock;
    std::condition_variable mEventCond;
    std::vector<HidlVecOfValues> mReceivedEvents;

  public:
    Return<void> onPropertyEvent(const hidl_vec<VehiclePropValue>& values) override {
        {
            MutexGuard guard(mLock);
            mReceivedEvents.push_back(values);
        }
        mEventCond.notify_one();
        return Return<void>();
    }

    Return<void> onPropertySet(const VehiclePropValue& /* value */) override {
        return Return<void>();
    }
    Return<void> onPropertySetError(StatusCode /* errorCode */, int32_t /* propId */,
                                    int32_t /* areaId */) override {
        return Return<void>();
    }

    bool waitForExpectedEvents(size_t expectedEvents) {
        std::unique_lock<std::mutex> g(mLock);

        if (expectedEvents == 0 && mReceivedEvents.size() == 0) {
            return mEventCond.wait_for(g, kTimeout) == std::cv_status::timeout;
        }

        while (expectedEvents != mReceivedEvents.size()) {
            if (mEventCond.wait_for(g, kTimeout) == std::cv_status::timeout) {
                return false;
            }
        }
        return true;
    }

    void reset() { mReceivedEvents.clear(); }
};

class VehicleHalHidlTest : public testing::TestWithParam<std::string> {
  protected:
    bool checkIsSupported(int32_t propertyId);

  public:
    virtual void SetUp() override {
        mVehicle = IVehicle::getService(GetParam());
        ASSERT_NE(mVehicle.get(), nullptr);
    }
    virtual void TearDown() override {}

    sp<IVehicle> mVehicle;

    bool isBooleanGlobalProp(int32_t property) {
        return (property & (int)VehiclePropertyType::MASK) == (int)VehiclePropertyType::BOOLEAN &&
               (property & (int)VehicleArea::MASK) == (int)VehicleArea::GLOBAL;
    }

    void invokeGet(int32_t property, int32_t areaId) {
        VehiclePropValue requestedValue{};
        requestedValue.prop = property;
        requestedValue.areaId = areaId;

        invokeGet(requestedValue);
    }

    void invokeGet(const VehiclePropValue& requestedPropValue) {
        mActualValue = VehiclePropValue{};  // reset previous values

        StatusCode refStatus;
        VehiclePropValue refValue;
        bool isCalled = false;
        mVehicle->get(requestedPropValue,
                      [&refStatus, &refValue, &isCalled](StatusCode status,
                                                         const VehiclePropValue& value) {
                          refStatus = status;
                          refValue = value;
                          isCalled = true;
                      });
        ASSERT_TRUE(isCalled) << "callback wasn't called for property: " << requestedPropValue.prop;

        mActualValue = refValue;
        mActualStatusCode = refStatus;
    }

    VehiclePropValue mActualValue;
    StatusCode mActualStatusCode;
};

// Test getAllPropConfigs() returns at least 1 property configs.
TEST_P(VehicleHalHidlTest, getAllPropConfigs) {
    ALOGD("VehicleHalHidlTest::getAllPropConfigs");
    bool isCalled = false;
    hidl_vec<VehiclePropConfig> propConfigs;
    mVehicle->getAllPropConfigs([&isCalled, &propConfigs](const hidl_vec<VehiclePropConfig>& cfgs) {
        propConfigs = cfgs;
        isCalled = true;
    });
    ASSERT_TRUE(isCalled);
    ASSERT_GE(propConfigs.size(), 1);
}

// Test getPropConfigs() can query properties returned by getAllPropConfigs.
TEST_P(VehicleHalHidlTest, getPropConfigsWithValidProps) {
    ALOGD("VehicleHalHidlTest::getPropConfigs");
    std::vector<int32_t> properties;
    mVehicle->getAllPropConfigs([&properties](const hidl_vec<VehiclePropConfig>& cfgs) {
        for (int i = 0; i < cfgs.size(); i++) {
            properties.push_back(cfgs[i].prop);
        }
    });
    bool isCalled = false;
    mVehicle->getPropConfigs(
            properties,
            [&properties, &isCalled](StatusCode status, const hidl_vec<VehiclePropConfig>& cfgs) {
                ASSERT_EQ(StatusCode::OK, status);
                ASSERT_EQ(properties.size(), cfgs.size());
                isCalled = true;
            });
    ASSERT_TRUE(isCalled);
}

// Test getPropConfig() with an invalid propertyId returns an error code.
TEST_P(VehicleHalHidlTest, getPropConfigsWithInvalidProp) {
    ALOGD("VehicleHalHidlTest::getPropConfigsWithInvalidProp");
    hidl_vec<int32_t> properties = {kInvalidProp};
    bool isCalled = false;
    mVehicle->getPropConfigs(
            properties, [&isCalled](StatusCode status, const hidl_vec<VehiclePropConfig>& cfgs) {
                ASSERT_NE(StatusCode::OK, status);
                ASSERT_EQ(0, cfgs.size());
                isCalled = true;
            });
    ASSERT_TRUE(isCalled);
}

bool VehicleHalHidlTest::checkIsSupported(int32_t propertyId) {
    hidl_vec<int32_t> properties = {propertyId};
    bool result = false;
    mVehicle->getPropConfigs(properties, [&result](
            StatusCode status, [[maybe_unused]] const hidl_vec<VehiclePropConfig>& cfgs) {
        result = (status == StatusCode::OK);
    });
    return result;
}

// Test get() return current value for properties.
TEST_P(VehicleHalHidlTest, get) {
    ALOGD("VehicleHalHidlTest::get");
    int32_t propertyId = static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED);
    if (!checkIsSupported(propertyId)) {
        GTEST_SKIP() << "Property: " << propertyId << " is not supported, skip the test";
    }
    invokeGet(propertyId, 0);
    ASSERT_EQ(StatusCode::OK, mActualStatusCode);
}

// Test get() with an invalid propertyId return an error codes.
TEST_P(VehicleHalHidlTest, getInvalidProp) {
    ALOGD("VehicleHalHidlTest::getInvalidProp");

    invokeGet(kInvalidProp, 0);
    ASSERT_NE(StatusCode::OK, mActualStatusCode);
}

// Test set() on read_write properties.
TEST_P(VehicleHalHidlTest, setProp) {
    ALOGD("VehicleHalHidlTest::setProp");
    hidl_vec<VehiclePropConfig> propConfigs;
    // skip hvac related properties
    std::unordered_set<int32_t> hvacProps = {(int)VehicleProperty::HVAC_DEFROSTER,
                                             (int)VehicleProperty::HVAC_AC_ON,
                                             (int)VehicleProperty::HVAC_MAX_AC_ON,
                                             (int)VehicleProperty::HVAC_MAX_DEFROST_ON,
                                             (int)VehicleProperty::HVAC_RECIRC_ON,
                                             (int)VehicleProperty::HVAC_DUAL_ON,
                                             (int)VehicleProperty::HVAC_AUTO_ON,
                                             (int)VehicleProperty::HVAC_POWER_ON,
                                             (int)VehicleProperty::HVAC_AUTO_RECIRC_ON,
                                             (int)VehicleProperty::HVAC_ELECTRIC_DEFROSTER_ON};
    mVehicle->getAllPropConfigs(
            [&propConfigs](const hidl_vec<VehiclePropConfig>& cfgs) { propConfigs = cfgs; });
    for (const VehiclePropConfig& cfg : propConfigs) {
        // test on boolean and writable property
        if (cfg.access == VehiclePropertyAccess::READ_WRITE && isBooleanGlobalProp(cfg.prop) &&
            !hvacProps.count(cfg.prop)) {
            invokeGet(cfg.prop, 0);

            if (mActualStatusCode == StatusCode::NOT_AVAILABLE ||
                mActualValue.status == VehiclePropertyStatus::UNAVAILABLE) {
                ALOGD("Property %i isn't available", cfg.prop);
                continue;
            }
            int setValue = mActualValue.value.int32Values[0] == 1 ? 0 : 1;
            VehiclePropValue propToSet = mActualValue;
            propToSet.value.int32Values[0] = setValue;

            StatusCode setResult = mVehicle->set(propToSet);
            ASSERT_THAT(setResult, testing::AnyOf(StatusCode::OK, StatusCode::NOT_AVAILABLE))
                << "Invalid status code for setting unavailable property: " << cfg.prop;

            // check set success
            invokeGet(cfg.prop, 0);
            // Retry getting the value until we pass the timeout. getValue might not return
            // the expected value immediately since setValue is async.
            auto propSetTimeoutMillis = ::android::uptimeMillis() + kPropSetDelayMillis.count();
            while (true) {
                invokeGet(cfg.prop, 0);
                if (mActualStatusCode == StatusCode::OK &&
                    mActualValue.status == VehiclePropertyStatus::AVAILABLE &&
                    mActualValue.value.int32Values[0] == setValue) {
                    break;
                }
                if (::android::uptimeMillis() >= propSetTimeoutMillis) {
                    // Reach timeout, the following assert should fail.
                    break;
                }
                // Sleep for 100ms between each invokeGet retry.
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            ASSERT_EQ(StatusCode::OK, mActualStatusCode);
            // If the property isn't available, it doesn't make sense to check
            // the returned value.
            if (mActualValue.status == VehiclePropertyStatus::AVAILABLE) {
                ASSERT_EQ(setValue, mActualValue.value.int32Values[0])
                        << "Failed to set value for property: " << cfg.prop;
            }
        }
    }
}

// Test set() on an read_only property.
TEST_P(VehicleHalHidlTest, setNotWritableProp) {
    ALOGD("VehicleHalHidlTest::setNotWritableProp");
    int32_t propertyId = static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED);
    if (!checkIsSupported(propertyId)) {
        GTEST_SKIP() << "Property: " << propertyId << " is not supported, skip the test";
    }
    invokeGet(propertyId, 0);
    ASSERT_EQ(StatusCode::OK, mActualStatusCode);
    VehiclePropValue vehicleSpeed = mActualValue;

    ASSERT_EQ(StatusCode::ACCESS_DENIED, mVehicle->set(vehicleSpeed));
}

// Test subscribe() and unsubscribe().
TEST_P(VehicleHalHidlTest, subscribeAndUnsubscribe) {
    ALOGD("VehicleHalHidlTest::subscribeAndUnsubscribe");
    int32_t propertyId = static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED);
    if (!checkIsSupported(propertyId)) {
        GTEST_SKIP() << "Property: " << propertyId << " is not supported, skip the test";
    }
    sp<VtsVehicleCallback> cb = new VtsVehicleCallback();

    hidl_vec<SubscribeOptions> options = {SubscribeOptions{
            .propId = propertyId, 100.0, .flags = SubscribeFlags::EVENTS_FROM_CAR}};

    ASSERT_EQ(StatusCode::OK, mVehicle->subscribe(cb, options));
    ASSERT_TRUE(cb->waitForExpectedEvents(10));

    ASSERT_EQ(StatusCode::OK, mVehicle->unsubscribe(cb, propertyId));
    cb->reset();
    ASSERT_FALSE(cb->waitForExpectedEvents(10));
}

// Test subscribe() with an invalid property.
TEST_P(VehicleHalHidlTest, subscribeInvalidProp) {
    ALOGD("VehicleHalHidlTest::subscribeInvalidProp");

    sp<VtsVehicleCallback> cb = new VtsVehicleCallback();

    hidl_vec<SubscribeOptions> options = {SubscribeOptions{
            .propId = kInvalidProp, 10.0, .flags = SubscribeFlags::EVENTS_FROM_CAR}};

    ASSERT_NE(StatusCode::OK, mVehicle->subscribe(cb, options));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VehicleHalHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, VehicleHalHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IVehicle::descriptor)),
        android::hardware::PrintInstanceNameToString);
