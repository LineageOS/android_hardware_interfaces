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

#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <vhal_v2_0/ConcurrentQueue.h>
#include <vhal_v2_0/DefaultVehicleConnector.h>
#include <vhal_v2_0/DefaultVehicleHal.h>
#include <vhal_v2_0/PropertyUtils.h>
#include <vhal_v2_0/VehicleObjectPool.h>
#include <vhal_v2_0/VehiclePropertyStore.h>

namespace {

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::automotive::vehicle::V2_0::FuelType;
using ::android::hardware::automotive::vehicle::V2_0::recyclable_ptr;
using ::android::hardware::automotive::vehicle::V2_0::StatusCode;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropConfig;
using ::android::hardware::automotive::vehicle::V2_0::VehicleProperty;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyStatus;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropertyStore;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValue;
using ::android::hardware::automotive::vehicle::V2_0::VehiclePropValuePool;
using ::android::hardware::automotive::vehicle::V2_0::impl::DefaultVehicleConnector;
using ::android::hardware::automotive::vehicle::V2_0::impl::DefaultVehicleHal;

using VehiclePropValuePtr = recyclable_ptr<VehiclePropValue>;

class DefaultVhalImplTest : public ::testing::Test {
  public:
    ~DefaultVhalImplTest() { mEventQueue.deactivate(); }

  protected:
    void SetUp() override {
        mPropStore.reset(new VehiclePropertyStore);
        mConnector.reset(new DefaultVehicleConnector);
        mHal.reset(new DefaultVehicleHal(mPropStore.get(), mConnector.get()));
        mConnector->setValuePool(&mValueObjectPool);
        mHal->init(&mValueObjectPool,
                   std::bind(&DefaultVhalImplTest::onHalEvent, this, std::placeholders::_1),
                   std::bind(&DefaultVhalImplTest::onHalPropertySetError, this,
                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

  private:
    void onHalEvent(VehiclePropValuePtr v) { mEventQueue.push(std::move(v)); }

    void onHalPropertySetError(StatusCode /*errorCode*/, int32_t /*property*/, int32_t /*areaId*/) {
    }

  protected:
    std::unique_ptr<DefaultVehicleHal> mHal;
    std::unique_ptr<DefaultVehicleConnector> mConnector;
    std::unique_ptr<VehiclePropertyStore> mPropStore;
    VehiclePropValuePool mValueObjectPool;
    android::ConcurrentQueue<VehiclePropValuePtr> mEventQueue;
};

TEST_F(DefaultVhalImplTest, testListProperties) {
    std::vector<VehiclePropConfig> configs = mHal->listProperties();

    EXPECT_EQ((size_t)122, configs.size());
}

TEST_F(DefaultVhalImplTest, testGetDefaultPropertyFloat) {
    VehiclePropValue value;
    StatusCode status;
    value.prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY);

    auto gotValue = mHal->get(value, &status);

    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.floatValues.size());
    EXPECT_EQ(15000.0f, gotValue->value.floatValues[0]);
}

TEST_F(DefaultVhalImplTest, testGetDefaultPropertyEnum) {
    VehiclePropValue value;
    StatusCode status;
    value.prop = toInt(VehicleProperty::INFO_FUEL_TYPE);

    auto gotValue = mHal->get(value, &status);

    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.int32Values.size());
    EXPECT_EQ((int)FuelType::FUEL_TYPE_UNLEADED, gotValue->value.int32Values[0]);
}

TEST_F(DefaultVhalImplTest, testGetDefaultPropertyInt) {
    VehiclePropValue value;
    StatusCode status;
    value.prop = toInt(VehicleProperty::INFO_MODEL_YEAR);

    auto gotValue = mHal->get(value, &status);

    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.int32Values.size());
    EXPECT_EQ(2020, gotValue->value.int32Values[0]);
}

TEST_F(DefaultVhalImplTest, testGetDefaultPropertyString) {
    VehiclePropValue value;
    StatusCode status;
    value.prop = toInt(VehicleProperty::INFO_MAKE);

    auto gotValue = mHal->get(value, &status);

    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ("Toy Vehicle", gotValue->value.stringValue);
}

TEST_F(DefaultVhalImplTest, testGetUnknownProperty) {
    VehiclePropValue value;
    StatusCode status;
    value.prop = 0;

    auto gotValue = mHal->get(value, &status);

    EXPECT_EQ(StatusCode::INVALID_ARG, status);
}

TEST_F(DefaultVhalImplTest, testSetFloat) {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY);
    value.value.floatValues.resize(1);
    value.value.floatValues[0] = 1.0f;

    StatusCode status = mHal->set(value);
    EXPECT_EQ(StatusCode::OK, status);

    auto gotValue = mHal->get(value, &status);
    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.floatValues.size());
    EXPECT_EQ(1.0f, gotValue->value.floatValues[0]);
}

TEST_F(DefaultVhalImplTest, testSetEnum) {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INFO_FUEL_TYPE);
    value.value.int32Values.resize(1);
    value.value.int32Values[0] = (int)FuelType::FUEL_TYPE_LEADED;

    StatusCode status = mHal->set(value);
    EXPECT_EQ(StatusCode::OK, status);

    auto gotValue = mHal->get(value, &status);
    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.int32Values.size());
    EXPECT_EQ((int)FuelType::FUEL_TYPE_LEADED, gotValue->value.int32Values[0]);
}

TEST_F(DefaultVhalImplTest, testSetInt) {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INFO_MODEL_YEAR);
    value.value.int32Values.resize(1);
    value.value.int32Values[0] = 2021;

    StatusCode status = mHal->set(value);
    EXPECT_EQ(StatusCode::OK, status);

    auto gotValue = mHal->get(value, &status);
    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ((unsigned int)1, gotValue->value.int32Values.size());
    EXPECT_EQ(2021, gotValue->value.int32Values[0]);
}

TEST_F(DefaultVhalImplTest, testSetString) {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INFO_MAKE);
    value.value.stringValue = "My Vehicle";

    StatusCode status = mHal->set(value);
    EXPECT_EQ(StatusCode::OK, status);

    auto gotValue = mHal->get(value, &status);
    EXPECT_EQ(StatusCode::OK, status);
    EXPECT_EQ("My Vehicle", gotValue->value.stringValue);
}

TEST_F(DefaultVhalImplTest, testSetUnknownProperty) {
    VehiclePropValue value;
    value.prop = 0;

    EXPECT_EQ(StatusCode::INVALID_ARG, mHal->set(value));
}

TEST_F(DefaultVhalImplTest, testSetStatusNotAllowed) {
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY);
    value.status = VehiclePropertyStatus::UNAVAILABLE;
    value.value.floatValues.resize(1);
    value.value.floatValues[0] = 1.0f;

    StatusCode status = mHal->set(value);

    EXPECT_EQ(StatusCode::INVALID_ARG, status);
}

TEST_F(DefaultVhalImplTest, testSubscribe) {
    // Clear existing events.
    mEventQueue.flush();

    auto status = mHal->subscribe(toInt(VehicleProperty::PERF_VEHICLE_SPEED), 10);

    EXPECT_EQ(StatusCode::OK, status);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Modify the speed after 0.5 seconds.
    VehiclePropValue value;
    value.prop = toInt(VehicleProperty::PERF_VEHICLE_SPEED);
    value.value.floatValues.resize(1);
    value.value.floatValues[0] = 1.0f;
    EXPECT_EQ(StatusCode::OK, mHal->set(value));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto events = mEventQueue.flush();
    EXPECT_LE((size_t)10, events.size());

    // The first event should be the default value.
    EXPECT_EQ((size_t)1, events[0]->value.floatValues.size());
    EXPECT_EQ(0.0f, events[0]->value.floatValues[0]);
    // The last event should be the value after update.
    EXPECT_EQ((size_t)1, events[events.size() - 1]->value.floatValues.size());
    EXPECT_EQ(1.0f, events[events.size() - 1]->value.floatValues[0]);
}

TEST_F(DefaultVhalImplTest, testSubscribeInvalidProp) {
    EXPECT_EQ(StatusCode::INVALID_ARG, mHal->subscribe(toInt(VehicleProperty::INFO_MAKE), 10));
}

TEST_F(DefaultVhalImplTest, testUnsubscribe) {
    auto status = mHal->subscribe(toInt(VehicleProperty::PERF_VEHICLE_SPEED), 10);
    EXPECT_EQ(StatusCode::OK, status);

    // Wait for 0.5 seconds to generate some events.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    status = mHal->unsubscribe(toInt(VehicleProperty::PERF_VEHICLE_SPEED));
    EXPECT_EQ(StatusCode::OK, status);

    // Clear all the events.
    mEventQueue.flush();

    // Wait for 0.5 seconds.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // There should be no new events generated.
    auto events = mEventQueue.flush();
    EXPECT_EQ((size_t)0, events.size());
}

TEST_F(DefaultVhalImplTest, testUnsubscribeInvalidProp) {
    EXPECT_EQ(StatusCode::INVALID_ARG, mHal->unsubscribe(toInt(VehicleProperty::INFO_MAKE)));
}

TEST_F(DefaultVhalImplTest, testDump) {
    hidl_vec<hidl_string> options;
    hidl_handle fd = {};
    native_handle_t* handle = native_handle_create(/*numFds=*/1, /*numInts=*/0);
    int memfd = memfd_create("memfile", 0);
    handle->data[0] = dup(memfd);
    fd.setTo(handle, /*shouldOwn=*/true);

    EXPECT_TRUE(mHal->dump(fd, options));

    lseek(memfd, 0, SEEK_SET);
    char buf[10240] = {};
    read(memfd, buf, sizeof(buf));
    close(memfd);

    // Read one property and check that it is in the dumped info.
    VehiclePropValue value;
    StatusCode status;
    value.prop = toInt(VehicleProperty::INFO_MAKE);
    auto gotValue = mHal->get(value, &status);
    EXPECT_EQ(StatusCode::OK, status);
    // Server side prop store does not have timestamp.
    gotValue->timestamp = 0;

    std::string infoMake = toString(*gotValue);
    EXPECT_NE(std::string::npos, std::string(buf).find(infoMake));
}

}  // namespace
