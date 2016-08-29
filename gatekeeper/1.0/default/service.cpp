#define LOG_TAG "android.hardware.gatekeeper@1.0-service"

#include <android/hardware/gatekeeper/1.0/IGatekeeper.h>

#include <hidl/LegacySupport.h>

// Generated HIDL files
using android::hardware::gatekeeper::V1_0::IGatekeeper;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    return defaultPassthroughServiceImplementation<IGatekeeper>("gatekeeper");
}
