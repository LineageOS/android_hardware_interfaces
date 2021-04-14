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

#include <limits>

using ::aidl::android::hardware::weaver::IWeaver;
using ::aidl::android::hardware::weaver::WeaverConfig;
using ::aidl::android::hardware::weaver::WeaverReadResponse;

using ::ndk::SpAIBinder;

const std::vector<uint8_t> KEY{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const std::vector<uint8_t> WRONG_KEY{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const std::vector<uint8_t> VALUE{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
const std::vector<uint8_t> OTHER_VALUE{0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 255, 255};

struct WeaverAidlTest : public ::testing::TestWithParam<std::string> {
    virtual void SetUp() override {
        weaver = IWeaver::fromBinder(
            SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(weaver, nullptr);
    }

    virtual void TearDown() override {}

    std::shared_ptr<IWeaver> weaver;
};

/*
 * Checks config values are suitably large
 */
TEST_P(WeaverAidlTest, GetConfig) {
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
TEST_P(WeaverAidlTest, GettingConfigMultipleTimesGivesSameResult) {
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
TEST_P(WeaverAidlTest, WriteToLastSlot) {
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
TEST_P(WeaverAidlTest, WriteFollowedByReadGivesTheSameValue) {
    constexpr uint32_t slotId = 0;
    const auto ret = weaver->write(slotId, KEY, VALUE);
    ASSERT_TRUE(ret.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    const auto readRet = weaver->read(slotId, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(readValue, VALUE);
    EXPECT_EQ(timeout, 0u);
}

/*
 * Writes a key and value to a slot
 * Overwrites the slot with a new key and value
 * Reads the slot with the new key and receives the new value
 */
TEST_P(WeaverAidlTest, OverwritingSlotUpdatesTheValue) {
    constexpr uint32_t slotId = 0;
    const auto initialWriteRet = weaver->write(slotId, WRONG_KEY, VALUE);
    ASSERT_TRUE(initialWriteRet.isOk());

    const auto overwriteRet = weaver->write(slotId, KEY, OTHER_VALUE);
    ASSERT_TRUE(overwriteRet.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    const auto readRet = weaver->read(slotId, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;

    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(readValue, OTHER_VALUE);
    EXPECT_EQ(timeout, 0u);
}

/*
 * Writes a key and value to a slot
 * Reads the slot with a different key so does not receive the value
 */
TEST_P(WeaverAidlTest, WriteFollowedByReadWithWrongKeyDoesNotGiveTheValue) {
    constexpr uint32_t slotId = 0;
    const auto ret = weaver->write(slotId, KEY, VALUE);
    ASSERT_TRUE(ret.isOk());

    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    const auto readRet =
        weaver->read(slotId, WRONG_KEY, &response);

    readValue = response.value;

    ASSERT_FALSE(readRet.isOk());
    ASSERT_EQ(EX_SERVICE_SPECIFIC, readRet.getExceptionCode());
    ASSERT_EQ(IWeaver::STATUS_INCORRECT_KEY, readRet.getServiceSpecificError());
    EXPECT_TRUE(readValue.empty());
}

/*
 * Writing to an invalid slot fails
 */
TEST_P(WeaverAidlTest, WritingToInvalidSlotFails) {
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
TEST_P(WeaverAidlTest, ReadingFromInvalidSlotFails) {
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
    const auto readRet =
        weaver->read(config.slots, KEY, &response);

    readValue = response.value;
    timeout = response.timeout;

    ASSERT_FALSE(readRet.isOk());
    ASSERT_EQ(EX_SERVICE_SPECIFIC, readRet.getExceptionCode());
    ASSERT_EQ(IWeaver::STATUS_FAILED, readRet.getServiceSpecificError());
    EXPECT_TRUE(readValue.empty());
    EXPECT_EQ(timeout, 0u);
}

/*
 * Writing a key that is too large fails
 */
TEST_P(WeaverAidlTest, WriteWithTooLargeKeyFails) {
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
TEST_P(WeaverAidlTest, WriteWithTooLargeValueFails) {
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
TEST_P(WeaverAidlTest, ReadWithTooLargeKeyFails) {
    WeaverConfig config;
    const auto configRet = weaver->getConfig(&config);
    ASSERT_TRUE(configRet.isOk());

    std::vector<uint8_t> bigKey(config.keySize + 1);

    constexpr uint32_t slotId = 0;
    WeaverReadResponse response;
    std::vector<uint8_t> readValue;
    uint32_t timeout;
    const auto readRet =
        weaver->read(slotId, bigKey, &response);

    readValue = response.value;
    timeout = response.timeout;

    ASSERT_FALSE(readRet.isOk());
    ASSERT_EQ(EX_SERVICE_SPECIFIC, readRet.getExceptionCode());
    ASSERT_EQ(IWeaver::STATUS_FAILED, readRet.getServiceSpecificError());
    EXPECT_TRUE(readValue.empty());
    EXPECT_EQ(timeout, 0u);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WeaverAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, WeaverAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IWeaver::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
