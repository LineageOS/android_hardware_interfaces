#define LOG_TAG "android.hardware.boot@1.0-service"
#include <utils/Log.h>

#include <iostream>
#include <unistd.h>

#include <android/hardware/boot/1.0/IBootControl.h>

#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

using android::sp;

using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

using ::android::hardware::boot::V1_0::IBootControl;

int main (int /* argc */, char * /* argv */ []) {
    ALOGI("Service is starting.");
    const char instance[] = "bootctrl";
    ALOGI("Retrieving default implementation of instance %s.",
          instance);

    sp<IBootControl> service = IBootControl::getService(instance, true /* getStub */);

    if (service.get() == nullptr) {
        ALOGE("IBootControl::getService returned NULL, exiting");
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
