#define LOG_TAG "android.hardware.nfc@1.0-service"
#include <utils/Log.h>

#include <iostream>
#include <unistd.h>

#include <android/hardware/nfc/1.0/INfc.h>

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
using android::hardware::nfc::V1_0::INfc;

int main() {
    ALOGI("Service is starting.");
    const char instance[] = "nfc_nci";
    ALOGI("Retrieving default implementation of instance %s.",
          instance);
    android::sp<INfc> service = INfc::getService(instance, true /* getStub */);
    if (service.get() == nullptr) {
        ALOGE("INfc::getService returned NULL, exiting");
        return -1;
    }
    ALOGI("Default implementation using %s is %s",
          instance, (service->isRemote() ? "REMOTE" : "LOCAL"));
    LOG_FATAL_IF(service->isRemote(), "Implementation is REMOTE!");
    ALOGI("Registering instance %s.", instance);
    service->registerAsService(instance);
    ALOGI("Ready.");

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
