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

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using namespace android::hardware::automotive::vehicle::V2_0;
using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

constexpr auto kTimeout = std::chrono::milliseconds(500);
constexpr auto kInvalidProp = 0x31600207;

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

// Test getAllPropConfig() returns at least 4 property configs.
TEST_P(VehicleHalHidlTest, getAllPropConfigs) {
    ALOGD("VehicleHalHidlTest::getAllPropConfigs");
    bool isCalled = false;
    hidl_vec<VehiclePropConfig> propConfigs;
    mVehicle->getAllPropConfigs([&isCalled, &propConfigs](const hidl_vec<VehiclePropConfig>& cfgs) {
        propConfigs = cfgs;
        isCalled = true;
    });
    ASSERT_TRUE(isCalled);
    ASSERT_GE(propConfigs.size(), 4);
}

// Test getPropConfig() can query all properties listed in CDD.
TEST_P(VehicleHalHidlTest, getPropConfigs) {
    ALOGD("VehicleHalHidlTest::getPropConfigs");
    // Check the properties listed in CDD
    hidl_vec<int32_t> properties = {
            (int)VehicleProperty::GEAR_SELECTION, (int)VehicleProperty::NIGHT_MODE,
            (int)VehicleProperty::PARKING_BRAKE_ON, (int)VehicleProperty::PERF_VEHICLE_SPEED};
    bool isCalled = false;
    mVehicle->getPropConfigs(
            properties, [&isCalled](StatusCode status, const hidl_vec<VehiclePropConfig>& cfgs) {
                ASSERT_EQ(StatusCode::OK, status);
                ASSERT_EQ(4u, cfgs.size());
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

// Test get() return current value for properties.
TEST_P(VehicleHalHidlTest, get) {
    ALOGD("VehicleHalHidlTest::get");
    invokeGet((int)VehicleProperty::PERF_VEHICLE_SPEED, 0);
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
            int setValue = mActualValue.value.int32Values[0] == 1 ? 0 : 1;
            VehiclePropValue propToSet = mActualValue;
            propToSet.value.int32Values[0] = setValue;
            ASSERT_EQ(StatusCode::OK, mVehicle->set(propToSet))
                    << "Invalid status code for setting property: " << cfg.prop;
            // check set success
            invokeGet(cfg.prop, 0);
            ASSERT_EQ(StatusCode::OK, mActualStatusCode);
            ASSERT_EQ(setValue, mActualValue.value.int32Values[0])
                    << "Failed to set value for property: " << cfg.prop;
        }
    }
}

// Test set() on an read_only property.
TEST_P(VehicleHalHidlTest, setNotWritableProp) {
    ALOGD("VehicleHalHidlTest::setNotWritableProp");
    invokeGet(static_cast<int>(VehicleProperty::PERF_VEHICLE_SPEED), 0);
    ASSERT_EQ(StatusCode::OK, mActualStatusCode);
    VehiclePropValue vehicleSpeed = mActualValue;

    ASSERT_EQ(StatusCode::ACCESS_DENIED, mVehicle->set(vehicleSpeed));
}

// Test subscribe() and unsubscribe().
TEST_P(VehicleHalHidlTest, subscribeAndUnsubscribe) {
    ALOGD("VehicleHalHidlTest::subscribeAndUnsubscribe");
    const auto prop = static_cast<int>(VehicleProperty::PERF_VEHICLE_SPEED);
    sp<VtsVehicleCallback> cb = new VtsVehicleCallback();

    hidl_vec<SubscribeOptions> options = {
            SubscribeOptions{.propId = prop, 100.0, .flags = SubscribeFlags::EVENTS_FROM_CAR}};

    ASSERT_EQ(StatusCode::OK, mVehicle->subscribe(cb, options));
    ASSERT_TRUE(cb->waitForExpectedEvents(10));

    ASSERT_EQ(StatusCode::OK, mVehicle->unsubscribe(cb, prop));
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
