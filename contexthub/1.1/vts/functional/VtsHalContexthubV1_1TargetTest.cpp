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

#define LOG_TAG "contexthub_hidl_hal_test"

#include "ContexthubCallbackBase.h"
#include "ContexthubHidlTestBase.h"
#include "VtsHalContexthubUtils.h"

#include <android-base/logging.h>
#include <android/hardware/contexthub/1.0/IContexthub.h>
#include <android/hardware/contexthub/1.1/IContexthub.h>
#include <android/log.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

#include <cinttypes>

using ::android::hardware::contexthub::V1_1::IContexthub;
using ::android::hardware::contexthub::V1_1::Setting;
using ::android::hardware::contexthub::V1_1::SettingValue;
using ::android::hardware::contexthub::vts_utils::ContexthubCallbackBase;
using ::android::hardware::contexthub::vts_utils::ContexthubHidlTestBase;
using ::android::hardware::contexthub::vts_utils::getHalAndHubIdList;

namespace {

const std::vector<std::tuple<std::string, std::string>> kTestParameters =
        getHalAndHubIdList<IContexthub>();

class ContexthubHidlTest : public ContexthubHidlTestBase<IContexthub> {};

TEST_P(ContexthubHidlTest, TestOnSettingChanged) {
    // In VTS, we only test that sending the values doesn't cause things to blow up - other test
    // suites verify the expected E2E behavior in CHRE
    ASSERT_OK(registerCallback(new ContexthubCallbackBase()));
    hubApi->onSettingChanged(Setting::LOCATION, SettingValue::DISABLED);
    hubApi->onSettingChanged(Setting::LOCATION, SettingValue::ENABLED);
    ASSERT_OK(registerCallback(nullptr));
}

INSTANTIATE_TEST_SUITE_P(HubIdSpecificTests, ContexthubHidlTest, testing::ValuesIn(kTestParameters),
                         android::hardware::PrintInstanceTupleNameToString<>);

}  // anonymous namespace
