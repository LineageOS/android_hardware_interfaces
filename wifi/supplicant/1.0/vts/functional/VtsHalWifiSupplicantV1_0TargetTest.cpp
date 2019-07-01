/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "supplicant_hidl_test_utils.h"
#include "wifi_hidl_test_utils.h"

class WifiSupplicantHidlEnvironment_1_0 : public WifiSupplicantHidlEnvironment {
   public:
    // get the test environment singleton
    static WifiSupplicantHidlEnvironment_1_0* Instance() {
        static WifiSupplicantHidlEnvironment_1_0* instance =
            new WifiSupplicantHidlEnvironment_1_0;
        return instance;
    }
    virtual void registerTestServices() override {
        registerTestService<::android::hardware::wifi::V1_0::IWifi>();
        registerTestService<
            ::android::hardware::wifi::supplicant::V1_0::ISupplicant>();
    }

   private:
    WifiSupplicantHidlEnvironment_1_0() {}
};

WifiSupplicantHidlEnvironment* gEnv =
    WifiSupplicantHidlEnvironment_1_0::Instance();

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    gEnv->init(&argc, argv);
    int status = gEnv->initFromOptions(argc, argv);
    if (status == 0) {
        int status = RUN_ALL_TESTS();
        LOG(INFO) << "Test result = " << status;
    }
    return status;
}
