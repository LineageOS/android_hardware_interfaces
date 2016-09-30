#ifndef HIDL_GENERATED_android_hardware_light_V2_0_Light_H_
#define HIDL_GENERATED_android_hardware_light_V2_0_Light_H_

#include <android/hardware/light/2.0/ILight.h>
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <map>

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Light : public ILight {
    Light(std::map<Type, light_device_t*> &&lights);

    // Methods from ::android::hardware::light::V2_0::ILight follow.
    Return<Status> setLight(Type type, const LightState& state)  override;
    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb)  override;

private:
    std::map<Type, light_device_t*> mLights;
};

extern "C" ILight* HIDL_FETCH_ILight(const char* name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_light_V2_0_Light_H_
