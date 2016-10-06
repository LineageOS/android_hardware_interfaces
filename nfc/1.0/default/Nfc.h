#ifndef HIDL_GENERATED_android_hardware_nfc_V1_0_Nfc_H_
#define HIDL_GENERATED_android_hardware_nfc_V1_0_Nfc_H_

#include <android/hardware/nfc/1.0/INfc.h>
#include <hidl/Status.h>
#include <hardware/hardware.h>
#include <hardware/nfc.h>
namespace android {
namespace hardware {
namespace nfc {
namespace V1_0 {
namespace implementation {

using ::android::hardware::nfc::V1_0::INfc;
using ::android::hardware::nfc::V1_0::INfcClientCallback;
using ::android::hardware::nfc::V1_0::nfc_data_t;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Nfc : public INfc {
  Nfc(nfc_nci_device_t* device);
  ::android::hardware::Return<int32_t> open(const sp<INfcClientCallback>& clientCallback)  override;
  ::android::hardware::Return<int32_t> write(const nfc_data_t& data)  override;
  ::android::hardware::Return<int32_t> core_initialized(const hidl_vec<uint8_t>& data)  override;
  ::android::hardware::Return<int32_t> pre_discover()  override;
  ::android::hardware::Return<int32_t> close()  override;
  ::android::hardware::Return<int32_t> control_granted()  override;
  ::android::hardware::Return<int32_t> power_cycle()  override;

  static void event_callback(uint8_t event, uint8_t status) {
      if (mCallback != nullptr) {
          mCallback->sendEvent(
                  (::android::hardware::nfc::V1_0::nfc_event_t) event,
                  (::android::hardware::nfc::V1_0::nfc_status_t) status);
      }
  }
  static void data_callback(uint16_t data_len, uint8_t* p_data) {
      nfc_data_t data;
      data.data.setToExternal(p_data, data_len);
      if (mCallback != nullptr) {
          mCallback->sendData(data);
      }
  }
  private:
    static sp<INfcClientCallback> mCallback;
    const nfc_nci_device_t*       mDevice;
};

extern "C" INfc* HIDL_FETCH_INfc(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace nfc
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_nfc_V1_0_Nfc_H_
