#define LOG_TAG "android.hardware.boot@1.0-service"

#include <android/hardware/boot/1.0/IBootControl.h>
#include <hidl/LegacySupport.h>

using ::android::hardware::boot::V1_0::IBootControl;
using android::hardware::defaultPassthroughServiceImplementation;

int main (int /* argc */, char * /* argv */ []) {
    return defaultPassthroughServiceImplementation<IBootControl>("bootctrl");
}
