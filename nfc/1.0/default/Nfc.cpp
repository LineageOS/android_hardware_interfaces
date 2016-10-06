#define LOG_TAG "android.hardware.nfc@1.0-impl"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include "Nfc.h"

namespace android {
namespace hardware {
namespace nfc {
namespace V1_0 {
namespace implementation {

sp<INfcClientCallback> Nfc::mCallback = NULL;

Nfc::Nfc(nfc_nci_device_t* device) : mDevice(device) {
}

// Methods from ::android::hardware::nfc::V1_0::INfc follow.
::android::hardware::Return<int32_t> Nfc::open(const sp<INfcClientCallback>& clientCallback)  {
    mCallback = clientCallback;
    return mDevice->open(mDevice, event_callback, data_callback);
}

::android::hardware::Return<int32_t> Nfc::write(const nfc_data_t& data)  {
    return mDevice->write(mDevice, data.data.size(), &data.data[0]);
}

::android::hardware::Return<int32_t> Nfc::core_initialized(const hidl_vec<uint8_t>& data)  {
    hidl_vec<uint8_t> copy = data;
    return mDevice->core_initialized(mDevice, &copy[0]);
}

::android::hardware::Return<int32_t> Nfc::pre_discover()  {
    return mDevice->pre_discover(mDevice);
}

::android::hardware::Return<int32_t> Nfc::close()  {
    return mDevice->close(mDevice);
}

::android::hardware::Return<int32_t> Nfc::control_granted()  {
    return mDevice->control_granted(mDevice);
}

::android::hardware::Return<int32_t> Nfc::power_cycle()  {
    return mDevice->power_cycle(mDevice);
}


INfc* HIDL_FETCH_INfc(const char *hal) {
    nfc_nci_device_t* nfc_device;
    int ret = 0;
    const hw_module_t* hw_module = NULL;

    ret = hw_get_module (hal, &hw_module);
    if (ret == 0)
    {
        ret = nfc_nci_open (hw_module, &nfc_device);
        if (ret != 0) {
            ALOGE ("nfc_nci_open %s failed: %d", hal, ret);
        }
    }
    else
        ALOGE ("hw_get_module %s failed: %d", hal, ret);

    if (ret == 0) {
        return new Nfc(nfc_device);
    } else {
        ALOGE("Passthrough failed to load legacy HAL.");
        return nullptr;
    }
}

} // namespace implementation
}  // namespace V1_0
}  // namespace nfc
}  // namespace hardware
}  // namespace android
