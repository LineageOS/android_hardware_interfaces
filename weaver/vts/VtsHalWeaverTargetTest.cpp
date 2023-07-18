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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/weaver/IWeaver.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/weaver/1.0/IWeaver.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <limits>

using ::aidl::android::hardware::weaver::IWeaver;
using ::aidl::android::hardware::weaver::WeaverConfig;
using ::aidl::android::hardware::weaver::WeaverReadResponse;
using ::aidl::android::hardware::weaver::WeaverReadStatus;

using HidlIWeaver = ::android::hardware::weaver::V1_0::IWeaver;
using HidlWeaverConfig = ::android::hardware::weaver::V1_0::WeaverConfig;
using HidlWeaverReadStatus = ::android::hardware::weaver::V1_0::WeaverReadStatus;
using HidlWeaverReadResponse = ::android::hardware::weaver::V1_0::WeaverReadResponse;
using HidlWeaverStatus = ::android::hardware::weaver::V1_0::WeaverStatus;

const std::vector<uint8_t> KEY{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const std::vector<uint8_t> WRONG_KEY{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<uint8_t> VALUE{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
const std::vector<uint8_t> OTHER_VALUE{0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 255, 255};

class WeaverAdapter {
  public:
    virtual ~WeaverAdapter() {}
    virtual bool isReady() = 0;
    virtual ::ndk::ScopedAStatus getConfig(WeaverConfig* _aidl_return) = 0;
    virtual ::ndk::ScopedAStatus read(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                                      WeaverReadResponse* _aidl_return) = 0;
    virtual ::ndk::ScopedAStatus write(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                                       const std::vector<uint8_t>& in_value) = 0;
};

class WeaverAidlAdapter : public WeaverAdapter {
  public:
    WeaverAidlAdapter(const std::string& param)
        : aidl_weaver_(IWeaver::fromBinder(
                  ::ndk::SpAIBinder(AServiceManager_waitForService(param.c_str())))) {}
    ~WeaverAidlAdapter() {}

    bool isReady() { return aidl_weaver_ != nullptr; }

    ::ndk::ScopedAStatus getConfig(WeaverConfig* _aidl_return) {
        return aidl_weaver_->getConfig(_aidl_return);
    }

    ::ndk::ScopedAStatus read(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                              WeaverReadResponse* _aidl_return) {
        return aidl_weaver_->read(in_slotId, in_key, _aidl_return);
    }

    ::ndk::ScopedAStatus write(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                               const std::vector<uint8_t>& in_value) {
        return aidl_weaver_->write(in_slotId, in_key, in_value);
    }

  private:
    std::shared_ptr<IWeaver> aidl_weaver_;
};

class WeaverHidlAdapter : public WeaverAdapter {
  public:
    WeaverHidlAdapter(const std::string& param) : hidl_weaver_(HidlIWeaver::getService(param)) {}
    ~WeaverHidlAdapter() {}

    bool isReady() { return hidl_weaver_ != nullptr; }

    ::ndk::ScopedAStatus getConfig(WeaverConfig* _aidl_return) {
        bool callbackCalled = false;
        HidlWeaverStatus status;
        HidlWeaverConfig config;
        auto ret = hidl_weaver_->getConfig([&](HidlWeaverStatus s, HidlWeaverConfig c) {
            callbackCalled = true;
            status = s;
            config = c;
        });
        if (!ret.isOk() || !callbackCalled || status != HidlWeaverStatus::OK) {
            return ::ndk::ScopedAStatus::fromStatus(STATUS_FAILED_TRANSACTION);
        }
        _aidl_return->slots = config.slots;
        _aidl_return->keySize = config.keySize;
        _aidl_return->valueSize = config.valueSize;
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus read(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                              WeaverReadResponse* _aidl_return) {
        bool callbackCalled = false;
        HidlWeaverReadStatus status;
        std::vector<uint8_t> value;
        uint32_t timeout;
        auto ret = hidl_weaver_->read(in_slotId, in_key,
                                      [&](HidlWeaverReadStatus s, HidlWeaverReadResponse r) {
                                          callbackCalled = true;
                                          status = s;
                                          value = r.value;
                                          timeout = r.timeout;
                                      });
        if (!ret.isOk() || !callbackCalled) {
            return ::ndk::ScopedAStatus::fromStatus(STATUS_FAILED_TRANSACTION);
        }
        switch (status) {
            case HidlWeaverReadStatus::OK:
                _aidl_return->status = WeaverReadStatus::OK;
                break;
            case HidlWeaverReadStatus::FAILED:
                _aidl_return->status = WeaverReadStatus::FAILED;
                break;
            case HidlWeaverReadStatus::INCORRECT_KEY:
                _aidl_return->status = WeaverReadStatus::INCORRECT_KEY;
                break;
            case HidlWeaverReadStatus::THROTTLE:
                _aidl_return->status = WeaverReadStatus::THROTTLE;
                break;
            default:
                ADD_FAILURE() << "Unknown HIDL read status: " << static_cast<uint32_t>(status);
                _aidl_return->status = WeaverReadStatus::FAILED;
                break;
        }
        _aidl_return->value = value;
        _aidl_return->timeout = timeout;
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus write(int32_t in_slotId, const std::vector<uint8_t>& in_key,
                               const std::vector<uint8_t>& in_value) {
        auto status = hidl_weaver_->write(in_slotId, in_key, in_value);
        switch (status) {
            case HidlWeaverStatus::OK:
                return ::ndk::ScopedAStatus::ok();
            case HidlWeaverStatus::FAILED:
                return ::ndk::ScopedAStatus::fromStatus(STATUS_FAILED_TRANSACTION);
            default:
                ADD_FAILURE() << "Unknown HIDL write status: " << status.description();
                return ::ndk::ScopedAStatus::fromStatus(STATUS_FAILED_TRANSACTION);
        }
    }

  private:
    android::sp<HidlIWeaver> hidl_weaver_;
};

class WeaverTest : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
  protected:
    void SetUp() override;
    void TearDown() override {}

    std::unique_ptr<WeaverAdapter> weaver;
};

void WeaverTest::SetUp() {
    std::string api, instance_name;
    std::tie(api, instance_name) = GetParam();
    if (api == "hidl") {
        weaver.reset(new WeaverHidlAdapter(instance_name));
    } else if (api == "aidl") {
        weaver.reset(new WeaverAidlAdapter(instance_name));
    } else {
        FAIL() << "Bad test parameterization";
    }
    ASSERT_TRUE(weaver->isReady());
}

/*
 * Checks config values are suitably large
 */
TEST_P(WeaverTest, GetConfig) {
    WeaverConfig config;

    auto ret = weaver->getConfig(&config);

    ASSERT_TRUE(ret.isOk());

    EXPECT_GE(config.slots, 16u);
    EXPECT_GE(config.keySize, 16u);
    EXPECT_GE(config.valueSize, 16u);
}

/*
 * Gets the config twice and checks they are the same
 */
TEST_P(WeaverTest, GettingConfigMultipleTimesGivesSameResult) {
    WeaverConfig config1;
    WeaverConfig config2;

    auto ret = weaver->getConfig(&config1);
    ASSERT_TRUE(ret.isOk());

    ret = weaver->getConfig(&config2);
    ASSERT_TRUE(ret.isOk());

    EXPECT_EQ(config1, config2);
}

/*
 * Gets the number of slots from the config and writes a key and value to the last one
 */
TEST_P(WeaverTest, WriteToLastSlot) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);

    ASSERT_TRUE(configRet.isOk());

    const uint32_t lastSlot = config.slots - 1;
    const auto writeRet = weaver->write(lastSlot, KEY, VALUE);
    ASSERT_TRUE(writeRet.isOk());
}

/*
 * Writes a key and value to a slot
 * Reads the slot with the same key and receives the value that was previously written
 */
TEST_P(WeaverTest, WriteFollowedByReadGivesTheSameValue) {
    constexpr uint32_t slotId = 0;
    const auto ret = weaver->write(slotId, KEY, VALUE);
    ASSERT_TRUE(ret.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    WeaverReadStatus status;
    const auto readRet = weaver->read(slotId, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;
    status = response.status;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(readValue, VALUE);
    EXPECT_EQ(timeout, 0u);
    EXPECT_EQ(status, WeaverReadStatus::OK);
}

/*
 * Writes a key and value to a slot
 * Overwrites the slot with a new key and value
 * Reads the slot with the new key and receives the new value
 */
TEST_P(WeaverTest, OverwritingSlotUpdatesTheValue) {
    constexpr uint32_t slotId = 0;
    const auto initialWriteRet = weaver->write(slotId, WRONG_KEY, VALUE);
    ASSERT_TRUE(initialWriteRet.isOk());

    const auto overwriteRet = weaver->write(slotId, KEY, OTHER_VALUE);
    ASSERT_TRUE(overwriteRet.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    WeaverReadStatus status;
    const auto readRet = weaver->read(slotId, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;
    status = response.status;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(readValue, OTHER_VALUE);
    EXPECT_EQ(timeout, 0u);
    EXPECT_EQ(status, WeaverReadStatus::OK);
}

/*
 * Writes a key and value to a slot
 * Reads the slot with a different key so does not receive the value
 */
TEST_P(WeaverTest, WriteFollowedByReadWithWrongKeyDoesNotGiveTheValue) {
    constexpr uint32_t slotId = 0;
    const auto ret = weaver->write(slotId, KEY, VALUE);
    ASSERT_TRUE(ret.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    WeaverReadStatus status;
    const auto readRet =
        weaver->read(slotId, WRONG_KEY, &response);

    readValue = response.value;
    status = response.status;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(readValue.empty());
    EXPECT_EQ(status, WeaverReadStatus::INCORRECT_KEY);
}

/*
 * Writing to an invalid slot fails
 */
TEST_P(WeaverTest, WritingToInvalidSlotFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    if (config.slots == std::numeric_limits<uint32_t>::max()) {
        // If there are no invalid slots then pass
        return;
    }

    const auto writeRet = weaver->write(config.slots, KEY, VALUE);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Reading from an invalid slot fails rather than incorrect key
 */
TEST_P(WeaverTest, ReadingFromInvalidSlotFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    if (config.slots == std::numeric_limits<uint32_t>::max()) {
        // If there are no invalid slots then pass
        return;
    }

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    WeaverReadStatus status;
    const auto readRet =
        weaver->read(config.slots, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;
    status = response.status;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(readValue.empty());
    EXPECT_EQ(timeout, 0u);
    EXPECT_EQ(status, WeaverReadStatus::FAILED);
}

/*
 * Writing a key that is too large fails
 */
TEST_P(WeaverTest, WriteWithTooLargeKeyFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    std::vector<uint8_t> bigKey(config.keySize + 1);

    constexpr uint32_t slotId = 0;
    const auto writeRet = weaver->write(slotId, bigKey, VALUE);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Writing a value that is too large fails
 */
TEST_P(WeaverTest, WriteWithTooLargeValueFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    std::vector<uint8_t> bigValue(config.valueSize + 1);

    constexpr uint32_t slotId = 0;
    const auto writeRet = weaver->write(slotId, KEY, bigValue);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Reading with a key that is loo large fails
 */
TEST_P(WeaverTest, ReadWithTooLargeKeyFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    std::vector<uint8_t> bigKey(config.keySize + 1);

    constexpr uint32_t slotId = 0;
    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    WeaverReadStatus status;
    const auto readRet =
        weaver->read(slotId, bigKey, &response);

    readValue = response.value;
    timeout = response.timeout;
    status = response.status;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(readValue.empty());
    EXPECT_EQ(timeout, 0u);
    EXPECT_EQ(status, WeaverReadStatus::FAILED);
}

// Instantiate the test for each HIDL Weaver service.
INSTANTIATE_TEST_SUITE_P(
        PerHidlInstance, WeaverTest,
        testing::Combine(testing::Values("hidl"),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 HidlIWeaver::descriptor))),
        [](const testing::TestParamInfo<std::tuple<std::string, std::string>>& info) {
            return android::hardware::PrintInstanceNameToString(
                    testing::TestParamInfo<std::string>{std::get<1>(info.param), info.index});
        });

// Instantiate the test for each AIDL Weaver service.
INSTANTIATE_TEST_SUITE_P(
        PerAidlInstance, WeaverTest,
        testing::Combine(testing::Values("aidl"),
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWeaver::descriptor))),
        [](const testing::TestParamInfo<std::tuple<std::string, std::string>>& info) {
            // This name_generator makes the instance name be included in the test case names, e.g.
            // "PerAidlInstance/WeaverTest#GetConfig/0_android_hardware_weaver_IWeaver_default"
            // instead of "PerAidlInstance/WeaverTest#GetConfig/0".
            return android::PrintInstanceNameToString(
                    testing::TestParamInfo<std::string>{std::get<1>(info.param), info.index});
        });

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
