/*
 * Copyright 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.identity-service"

#include <aidl/android/hardware/security/keymint/IRemotelyProvisionedComponent.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "IdentityCredentialStore.h"

#include "FakeSecureHardwareProxy.h"

using ::android::sp;
using ::android::base::InitLogging;
using ::android::base::LogdLogger;
using ::android::base::LogId;
using ::android::base::LogSeverity;
using ::android::base::StderrLogger;

using ::aidl::android::hardware::identity::IdentityCredentialStore;
using ::aidl::android::hardware::security::keymint::IRemotelyProvisionedComponent;
using ::android::hardware::identity::FakeSecureHardwareProxyFactory;
using ::android::hardware::identity::SecureHardwareProxyFactory;

void ComboLogger(LogId id, LogSeverity severity, const char* tag, const char* file,
                 unsigned int line, const char* message) {
    StderrLogger(id, severity, tag, file, line, message);

    static LogdLogger logdLogger;
    logdLogger(id, severity, tag, file, line, message);
}

int main(int /*argc*/, char* argv[]) {
    InitLogging(argv, ComboLogger);

    sp<SecureHardwareProxyFactory> hwProxyFactory = new FakeSecureHardwareProxyFactory();
    const std::string remotelyProvisionedComponentName =
            std::string(IRemotelyProvisionedComponent::descriptor) + "/default";

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<IdentityCredentialStore> store =
            ndk::SharedRefBase::make<IdentityCredentialStore>(hwProxyFactory,
                                                              remotelyProvisionedComponentName);

    const std::string instance = std::string() + IdentityCredentialStore::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(store->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
