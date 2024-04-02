/*
* Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "FMQ_UnitTests"

#include <TestAidlMsgQ.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/tests/msgq/1.0/ITestMsgQ.h>

using aidl::android::fmq::test::TestAidlMsgQ;

#include <hidl/LegacySupport.h>

using android::hardware::tests::msgq::V1_0::ITestMsgQ;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    android::hardware::details::setTrebleTestingOverride(true);
    // Register AIDL service
    ABinderProcess_startThreadPool();
    std::shared_ptr<TestAidlMsgQ> store = ndk::SharedRefBase::make<TestAidlMsgQ>();

    const std::string instance = std::string() + TestAidlMsgQ::descriptor + "/default";
    LOG(INFO) << "instance: " << instance;
    CHECK(AServiceManager_addService(store->asBinder().get(), instance.c_str()) == STATUS_OK);

    // Register HIDL service
    CHECK(defaultPassthroughServiceImplementation<ITestMsgQ>() == android::OK);
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE;  // should not reach
}
