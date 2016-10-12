#define LOG_TAG "android.hardware.light@2.0-service"
#include <utils/Log.h>

#include <iostream>
#include <unistd.h>

#include <android/hardware/light/2.0/ILight.h>

#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

using android::sp;

// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// Generated HIDL files
using android::hardware::light::V2_0::ILight;

int main() {
    ALOGI("Service is starting.");
    const char instance[] = "light";
    ALOGI("Retrieving default implementation of instance %s.",
          instance);

    android::sp<ILight> service = ILight::getService(instance, true);

    if (service.get() == nullptr) {
        ALOGE("ILight::getService returned NULL, exiting");
        return -1;
    }

    LOG_FATAL_IF(service->isRemote(), "Implementation is REMOTE!");

    ALOGI("Registering instance %s.", instance);
    service->registerAsService(instance);
    ALOGI("Ready.");

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
