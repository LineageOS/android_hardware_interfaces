/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <radio_hidl_hal_utils_v1_3.h>

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioHidlTest_v1_3);
INSTANTIATE_TEST_SUITE_P(PerInstance, RadioHidlTest_v1_3,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 ::android::hardware::radio::V1_3::IRadio::descriptor)),
                         android::hardware::PrintInstanceNameToString);