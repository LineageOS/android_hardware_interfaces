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

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include "radio_config_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioConfigTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_config = IRadioConfig::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_config.get());

    radioRsp_config = ndk::SharedRefBase::make<RadioConfigResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_config.get());

    count_ = 0;

    radioInd_config = ndk::SharedRefBase::make<RadioConfigIndication>(*this);
    ASSERT_NE(nullptr, radioInd_config.get());

    radio_config->setResponseFunctions(radioRsp_config, radioInd_config);
}

/*
 * Test IRadioConfig.getHalDeviceCapabilities() for the response returned.
 */
TEST_P(RadioConfigTest, getHalDeviceCapabilities) {
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_config->getHalDeviceCapabilities(serial);
    ASSERT_OK(res);
    ALOGI("getHalDeviceCapabilities, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());
}
