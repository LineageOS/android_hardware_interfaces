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

#include <android/binder_process.h>

#include "radio_config_utils.h"
#include "radio_data_utils.h"
#include "radio_ims_utils.h"
#include "radio_imsmedia_utils.h"
#include "radio_messaging_utils.h"
#include "radio_modem_utils.h"
#include "radio_network_utils.h"
#include "radio_sap_utils.h"
#include "radio_sim_utils.h"
#include "radio_voice_utils.h"

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioConfigTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioConfigTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioConfig::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioDataTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioDataTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioData::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioMessagingTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioMessagingTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioMessaging::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioModemTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioModemTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioModem::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioNetworkTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioNetworkTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioNetwork::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SapTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, SapTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ISap::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioSimTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, RadioSimTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IRadioSim::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioVoiceTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioVoiceTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioVoice::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioImsTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, RadioImsTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IRadioIms::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RadioImsMediaTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, RadioImsMediaTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IImsMedia::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
