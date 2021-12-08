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

#include <health-shim/shim.h>

#include <android/hardware/health/translate-ndk.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using HidlHealth = android::hardware::health::V2_0::IHealth;
using HidlHealthInfoCallback = android::hardware::health::V2_0::IHealthInfoCallback;
using HidlStorageInfo = android::hardware::health::V2_0::StorageInfo;
using HidlDiskStats = android::hardware::health::V2_0::DiskStats;
using HidlHealthInfo = android::hardware::health::V2_0::HealthInfo;
using HidlBatteryStatus = android::hardware::health::V1_0::BatteryStatus;
using android::sp;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::health::V2_0::Result;
using ndk::SharedRefBase;
using testing::Invoke;
using testing::NiceMock;

namespace aidl::android::hardware::health {
MATCHER(IsOk, "") {
    *result_listener << "status is " << arg.getDescription();
    return arg.isOk();
}

MATCHER_P(ExceptionIs, exception_code, "") {
    *result_listener << "status is " << arg.getDescription();
    return arg.getExceptionCode() == exception_code;
}

class MockHidlHealth : public HidlHealth {
  public:
    MOCK_METHOD(Return<Result>, registerCallback, (const sp<HidlHealthInfoCallback>& callback),
                (override));
    MOCK_METHOD(Return<Result>, unregisterCallback, (const sp<HidlHealthInfoCallback>& callback),
                (override));
    MOCK_METHOD(Return<Result>, update, (), (override));
    MOCK_METHOD(Return<void>, getChargeCounter, (getChargeCounter_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getCurrentNow, (getCurrentNow_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getCurrentAverage, (getCurrentAverage_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getCapacity, (getCapacity_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getEnergyCounter, (getEnergyCounter_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getChargeStatus, (getChargeStatus_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getStorageInfo, (getStorageInfo_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getDiskStats, (getDiskStats_cb _hidl_cb), (override));
    MOCK_METHOD(Return<void>, getHealthInfo, (getHealthInfo_cb _hidl_cb), (override));
};

class HealthShimTest : public ::testing::Test {
  public:
    void SetUp() override {
        hidl = new NiceMock<MockHidlHealth>();
        shim = SharedRefBase::make<HealthShim>(hidl);
    }
    sp<MockHidlHealth> hidl;
    std::shared_ptr<IHealth> shim;
};

#define ADD_TEST(name, aidl_name, AidlValueType, hidl_value, not_supported_hidl_value) \
    TEST_F(HealthShimTest, name) {                                                     \
        ON_CALL(*hidl, name).WillByDefault(Invoke([](auto cb) {                        \
            cb(Result::SUCCESS, hidl_value);                                           \
            return Void();                                                             \
        }));                                                                           \
        AidlValueType value;                                                           \
        ASSERT_THAT(shim->aidl_name(&value), IsOk());                                  \
        ASSERT_EQ(value, static_cast<AidlValueType>(hidl_value));                      \
    }                                                                                  \
                                                                                       \
    TEST_F(HealthShimTest, name##Unsupported) {                                        \
        ON_CALL(*hidl, name).WillByDefault(Invoke([](auto cb) {                        \
            cb(Result::NOT_SUPPORTED, not_supported_hidl_value);                       \
            return Void();                                                             \
        }));                                                                           \
        AidlValueType value;                                                           \
        ASSERT_THAT(shim->aidl_name(&value), ExceptionIs(EX_UNSUPPORTED_OPERATION));   \
    }

ADD_TEST(getChargeCounter, getChargeCounterUah, int32_t, 0xFEEDBEEF, 0)
ADD_TEST(getCurrentNow, getCurrentNowMicroamps, int32_t, 0xC0FFEE, 0)
ADD_TEST(getCurrentAverage, getCurrentAverageMicroamps, int32_t, 0xA2D401D, 0)
ADD_TEST(getCapacity, getCapacity, int32_t, 77, 0)
ADD_TEST(getEnergyCounter, getEnergyCounterNwh, int64_t, 0x1234567887654321, 0)
ADD_TEST(getChargeStatus, getChargeStatus, BatteryStatus, HidlBatteryStatus::CHARGING,
         HidlBatteryStatus::UNKNOWN)

#undef ADD_TEST

template <typename AidlValueType, typename HidlValueType>
bool Translate(const HidlValueType& hidl_value, AidlValueType* aidl_value) {
    return ::android::h2a::translate(hidl_value, aidl_value);
}

template <typename AidlValueType, typename HidlValueType>
bool Translate(const std::vector<HidlValueType>& hidl_vec, std::vector<AidlValueType>* aidl_vec) {
    aidl_vec->clear();
    aidl_vec->reserve(hidl_vec.size());
    for (const auto& hidl_value : hidl_vec) {
        auto& aidl_value = aidl_vec->emplace_back();
        if (!Translate(hidl_value, &aidl_value)) return false;
    }
    return true;
}

#define ADD_INFO_TEST(name, AidlValueType, hidl_value)                               \
    TEST_F(HealthShimTest, name) {                                                   \
        AidlValueType expected_aidl_value;                                           \
        ASSERT_TRUE(Translate(hidl_value, &expected_aidl_value));                    \
        ON_CALL(*hidl, name).WillByDefault(Invoke([&](auto cb) {                     \
            cb(Result::SUCCESS, hidl_value);                                         \
            return Void();                                                           \
        }));                                                                         \
        AidlValueType aidl_value;                                                    \
        ASSERT_THAT(shim->name(&aidl_value), IsOk());                                \
        ASSERT_EQ(aidl_value, expected_aidl_value);                                  \
    }                                                                                \
                                                                                     \
    TEST_F(HealthShimTest, name##Unsupported) {                                      \
        ON_CALL(*hidl, name).WillByDefault(Invoke([](auto cb) {                      \
            cb(Result::NOT_SUPPORTED, {});                                           \
            return Void();                                                           \
        }));                                                                         \
        AidlValueType aidl_value;                                                    \
        ASSERT_THAT(shim->name(&aidl_value), ExceptionIs(EX_UNSUPPORTED_OPERATION)); \
    }

ADD_INFO_TEST(getStorageInfo, std::vector<StorageInfo>,
              (std::vector<HidlStorageInfo>{{
                      .lifetimeA = 15,
                      .lifetimeB = 18,
              }}))

ADD_INFO_TEST(getDiskStats, std::vector<DiskStats>,
              (std::vector<HidlDiskStats>{{
                      .reads = 100,
                      .writes = 200,
              }}))

ADD_INFO_TEST(getHealthInfo, HealthInfo,
              (HidlHealthInfo{
                      .batteryCurrentAverage = 999,
              }))

#undef ADD_INFO_TEST

}  // namespace aidl::android::hardware::health
