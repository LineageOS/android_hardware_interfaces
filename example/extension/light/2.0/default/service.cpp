#define LOG_TAG "android.hardware.light@2.0-service"
#include <utils/Log.h>

// #include <iostream>
// #include <unistd.h>

// #include <hidl/IServiceManager.h>
// #include <hwbinder/IPCThreadState.h>
// #include <hwbinder/ProcessState.h>
// #include <utils/Errors.h>
// #include <utils/StrongPointer.h>

#include "Light.h"

using android::sp;

// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// Generated HIDL files
using android::hardware::light::V2_0::ILight;

int main() {
    const char instance[] = "light";

    android::sp<ILight> service = new Light();

    service->registerAsService(instance);

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
