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
#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
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

const std::string kSlotMapFile = "/metadata/password_slots/slot_map";
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
    void FindFreeSlots();

    std::unique_ptr<WeaverAdapter> weaver_;
    WeaverConfig config_;
    uint32_t first_free_slot_;
    uint32_t last_free_slot_;
};

void WeaverTest::SetUp() {
    std::string api, instance_name;
    std::tie(api, instance_name) = GetParam();
    if (api == "hidl") {
        weaver_.reset(new WeaverHidlAdapter(instance_name));
    } else if (api == "aidl") {
        weaver_.reset(new WeaverAidlAdapter(instance_name));
    } else {
        FAIL() << "Bad test parameterization";
    }
    ASSERT_TRUE(weaver_->isReady());

    auto ret = weaver_->getConfig(&config_);
    ASSERT_TRUE(ret.isOk());
    ASSERT_GT(config_.slots, 0);
    GTEST_LOG_(INFO) << "WeaverConfig: slots=" << config_.slots << ", keySize=" << config_.keySize
                     << ", valueSize=" << config_.valueSize;

    FindFreeSlots();
    GTEST_LOG_(INFO) << "First free slot is " << first_free_slot_ << ", last free slot is "
                     << last_free_slot_;
}

void WeaverTest::FindFreeSlots() {
    // Determine which Weaver slots are in use by the system. These slots can't be used by the test.
    std::set<uint32_t> used_slots;
    if (access(kSlotMapFile.c_str(), F_OK) == 0) {
        std::string contents;
        ASSERT_TRUE(android::base::ReadFileToString(kSlotMapFile, &contents))
                << "Failed to read " << kSlotMapFile;
        for (const auto& line : android::base::Split(contents, "\n")) {
            auto trimmed_line = android::base::Trim(line);
            if (trimmed_line[0] == '#' || trimmed_line[0] == '\0') continue;
            auto slot_and_user = android::base::Split(trimmed_line, "=");
            uint32_t slot;
            ASSERT_TRUE(slot_and_user.size() == 2 &&
                        android::base::ParseUint(slot_and_user[0], &slot))
                    << "Error parsing " << kSlotMapFile << " at \"" << line << "\"";
            GTEST_LOG_(INFO) << "Slot " << slot << " is in use by " << slot_and_user[1];
            ASSERT_LT(slot, config_.slots);
            used_slots.insert(slot);
        }
    }
    // Starting in Android 14, the system will always use at least one Weaver slot if Weaver is
    // supported at all.  Make sure we saw at least one.
    // TODO: uncomment after Android 14 is merged into AOSP
    // ASSERT_FALSE(used_slots.empty())
    //<< "Could not determine which Weaver slots are in use by the system";

    // Find the first free slot.
    int found = 0;
    for (uint32_t i = 0; i < config_.slots; i++) {
        if (used_slots.find(i) == used_slots.end()) {
            first_free_slot_ = i;
            found++;
            break;
        }
    }
    // Find the last free slot.
    for (uint32_t i = config_.slots; i > 0; i--) {
        if (used_slots.find(i - 1) == used_slots.end()) {
            last_free_slot_ = i - 1;
            found++;
            break;
        }
    }
    ASSERT_EQ(found, 2) << "All Weaver slots are already in use by the system";
}

/*
 * Checks config values are suitably large
 */
TEST_P(WeaverTest, GetConfig) {
    EXPECT_GE(config_.slots, 16u);
    EXPECT_GE(config_.keySize, 16u);
    EXPECT_GE(config_.valueSize, 16u);
}

/*
 * Gets the config twice and checks they are the same
 */
TEST_P(WeaverTest, GettingConfigMultipleTimesGivesSameResult) {
    WeaverConfig config2;

    auto ret = weaver_->getConfig(&config2);
    ASSERT_TRUE(ret.isOk());

    EXPECT_EQ(config_, config2);
}

/*
 * Writes a key and value to the last free slot
 */
TEST_P(WeaverTest, WriteToLastSlot) {
    const auto writeRet = weaver_->write(last_free_slot_, KEY, VALUE);
    ASSERT_TRUE(writeRet.isOk());
}

/*
 * Writes a key and value to a slot
 * Reads the slot with the same key and receives the value that was previously written
 */
TEST_P(WeaverTest, WriteFollowedByReadGivesTheSameValue) {
    const uint32_t slotId = first_free_slot_;
    const auto ret = weaver_->write(slotId, KEY, VALUE);
    ASSERT_TRUE(ret.isOk());

    WeaverReadResponse response;
    const auto readRet = weaver_->read(slotId, KEY, &response);
    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(response.value, VALUE);
    EXPECT_EQ(response.timeout, 0u);
    EXPECT_EQ(response.status, WeaverReadStatus::OK);
}

/*
 * Writes a key and value to a slot
 * Overwrites the slot with a new key and value
 * Reads the slot with the new key and receives the new value
 */
TEST_P(WeaverTest, OverwritingSlotUpdatesTheValue) {
    const uint32_t slotId = first_free_slot_;
    const auto initialWriteRet = weaver_->write(slotId, WRONG_KEY, VALUE);
    ASSERT_TRUE(initialWriteRet.isOk());

    const auto overwriteRet = weaver_->write(slotId, KEY, OTHER_VALUE);
    ASSERT_TRUE(overwriteRet.isOk());

    WeaverReadResponse response;
    const auto readRet = weaver_->read(slotId, KEY, &response);
    ASSERT_TRUE(readRet.isOk());
    EXPECT_EQ(response.value, OTHER_VALUE);
    EXPECT_EQ(response.timeout, 0u);
    EXPECT_EQ(response.status, WeaverReadStatus::OK);
}

/*
 * Writes a key and value to a slot
 * Reads the slot with a different key so does not receive the value
 */
TEST_P(WeaverTest, WriteFollowedByReadWithWrongKeyDoesNotGiveTheValue) {
    const uint32_t slotId = first_free_slot_;
    const auto writeRet = weaver_->write(slotId, KEY, VALUE);
    ASSERT_TRUE(writeRet.isOk());

    WeaverReadResponse response;
    const auto readRet = weaver_->read(slotId, WRONG_KEY, &response);
    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(response.value.empty());
    EXPECT_EQ(response.status, WeaverReadStatus::INCORRECT_KEY);
}

/*
 * Writing to an invalid slot fails
 */
TEST_P(WeaverTest, WritingToInvalidSlotFails) {
    if (config_.slots == std::numeric_limits<uint32_t>::max()) {
        // If there are no invalid slots then pass
        return;
    }

    const auto writeRet = weaver_->write(config_.slots, KEY, VALUE);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Reading from an invalid slot fails rather than incorrect key
 */
TEST_P(WeaverTest, ReadingFromInvalidSlotFails) {
    if (config_.slots == std::numeric_limits<uint32_t>::max()) {
        // If there are no invalid slots then pass
        return;
    }

    WeaverReadResponse response;
    const auto readRet = weaver_->read(config_.slots, KEY, &response);
    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(response.value.empty());
    EXPECT_EQ(response.timeout, 0u);
    EXPECT_EQ(response.status, WeaverReadStatus::FAILED);
}

/*
 * Writing a key that is too large fails
 */
TEST_P(WeaverTest, WriteWithTooLargeKeyFails) {
    std::vector<uint8_t> bigKey(config_.keySize + 1);

    const auto writeRet = weaver_->write(first_free_slot_, bigKey, VALUE);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Writing a value that is too large fails
 */
TEST_P(WeaverTest, WriteWithTooLargeValueFails) {
    std::vector<uint8_t> bigValue(config_.valueSize + 1);

    const auto writeRet = weaver_->write(first_free_slot_, KEY, bigValue);
    ASSERT_FALSE(writeRet.isOk());
}

/*
 * Reading with a key that is too large fails
 */
TEST_P(WeaverTest, ReadWithTooLargeKeyFails) {
    std::vector<uint8_t> bigKey(config_.keySize + 1);

    WeaverReadResponse response;
    const auto readRet = weaver_->read(first_free_slot_, bigKey, &response);
    ASSERT_TRUE(readRet.isOk());
    EXPECT_TRUE(response.value.empty());
    EXPECT_EQ(response.timeout, 0u);
    EXPECT_EQ(response.status, WeaverReadStatus::FAILED);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WeaverTest);

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
