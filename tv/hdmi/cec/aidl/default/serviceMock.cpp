/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.hdmi.cec-service-shim"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Log.h>
#include "HdmiCecMock.h"

using android::hardware::tv::hdmi::cec::implementation::HdmiCecMock;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    std::shared_ptr<HdmiCecMock> hdmiCecAidl = ndk::SharedRefBase::make<HdmiCecMock>();
    const std::string instance = std::string() + HdmiCecMock::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(hdmiCecAidl->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return 0;
}
