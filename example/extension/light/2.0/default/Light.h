#ifndef HIDL_GENERATED_android_hardware_example_extension_light_V2_0_Light_H_
#define HIDL_GENERATED_android_hardware_example_extension_light_V2_0_Light_H_

#include <android/hardware/example/extension/light/2.0/IExtLight.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace example {
namespace extension {
namespace light {
namespace V2_0 {
namespace implementation {

using ::android::hardware::example::extension::light::V2_0::ExtLightState;
using ::android::hardware::example::extension::light::V2_0::IExtLight;
using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Light : public IExtLight {
    // Methods from ::android::hardware::light::V2_0::ILight follow.
    Return<Status> setLight(Type type, const LightState& state)  override;
    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb)  override;

    // Methods from ::android::hardware::example::extension::light::V2_0::ILight follow.
    Return<Status> setExtLight(Type type, const ExtLightState& state)  override;

};

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace extension
}  // namespace example
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_example_extension_light_V2_0_Light_H_
