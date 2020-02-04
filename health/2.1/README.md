# Implementing Health 2.1 HAL

1. Install common binderized service. The binderized service `dlopen()`s
   passthrough implementations on the device, so there is no need to write
   your own.

    ```mk
    # Install default binderized implementation to vendor.
    PRODUCT_PACKAGES += android.hardware.health@2.1-service
    ```

1. Delete existing VINTF manifest entry. Search for `android.hardware.health` in
   your device manifest, and delete the whole `<hal>` entry for older versions
   of the HAL. Instead, when `android.hardware.health@2.1-service` is installed,
   a VINTF manifest fragment is installed to `/vendor/etc/vintf`, so there is
   no need to manually specify it in your device manifest. See
   [Manifest fragments](https://source.android.com/devices/architecture/vintf/objects#manifest-fragments)
   for details.

1. Install the proper passthrough implemetation.

    1. If you want to use default implementation:

        ```mk
        # Install default passthrough implementation to vendor.
        PRODUCT_PACKAGES += android.hardware.health@2.1-impl

        # For non-A/B devices, install default passthrough implementation to recovery.
        PRODUCT_PACKAGES += android.hardware.health@2.1-impl.recovery
        ```

        You are done. Otherwise, go to the next step.

    1. If you want to write your own implementation,

        1. Copy skeleton implementation from the [appendix](#impl).

        1. Modify the implementation to suit your needs.

            * If you have a board or device specific `libhealthd`, see
              [Upgrading with  a customized libhealthd](#update-from-1-0).
            * If you are upgrading from 1.0 health HAL, see
              [Upgrading from Health HAL 1.0](#update-from-1-0).
            * If you are upgrading from a customized 2.0 health HAL
              implementation, See
              [Upgrading from Health HAL 2.0](#update-from-2-0).

        1. [Install the implementation](#install).

        1. [Update necessary SELinux permissions](#selinux).

        1. [Fix `/charger` symlink](#charger-symlink).

# Upgrading with a customized libhealthd or from Health HAL 1.0 {#update-from-1-0}

`libhealthd` contains two functions: `healthd_board_init()` and
`healthd_board_battery_update()`. Similarly, Health HAL 1.0 contains `init()`
and `update()`, with an additional `energyCounter()` function.

* `healthd_board_init()` / `@1.0::IHealth.init()` should be called before
  passing the `healthd_config` struct to your `HealthImpl` class. See
  `HIDL_FETCH_IHealth` in [`HealthImpl.cpp`](#health_impl_cpp).

* `healthd_board_battery_update()` / `@1.0::IHealth.update()` should be called
  in `HealthImpl::UpdateHealthInfo()`. Example:

  ```c++
  void HealthImpl::UpdateHealthInfo(HealthInfo* health_info) {
      struct BatteryProperties props;
      convertFromHealthInfo(health_info->legacy.legacy, &props);
      healthd_board_battery_update(&props);
      convertToHealthInfo(&props, health_info->legacy.legacy);
  }
  ```
  For efficiency, you should move code in `healthd_board_battery_update` to
  `HealthImpl::UpdateHealthInfo` and modify `health_info` directly to avoid
  conversion to `BatteryProperties`.

* Code for `@1.0::IHealth.energyCounter()` should be moved to
  `HealthImpl::getEnergyCounter()`. Example:

  ```c++
  Return<void> Health::getEnergyCounter(getEnergyCounter_cb _hidl_cb) {
      int64_t energy = /* ... */;
      _hidl_cb(Result::SUCCESS, energy);
      return Void();
  }
  ```

# Upgrading from Health HAL 2.0 {#update-from-2-0}

* If you have implemented `healthd_board_init()` and/or
  `healthd_board_battery_update()` (instead of using `libhealthd.default`),
  see [the section above](#update-from-1-0)
  for instructions to convert them.

* If you have implemented `get_storage_info()` and/or `get_disk_stats()`
  (instead of using libhealthstoragedefault), implement `HealthImpl::getDiskStats`
  and/or `HealthImpl::getStorageInfo` directly. There is no need to override
  `HealthImpl::getHealthInfo` or `HealthImpl::getHealthInfo_2_1` because they call
  `getDiskStats` and `getStorageInfo` to retrieve storage information.

# Install the implementation {#install}

In `device.mk`:

```mk
# Install the passthrough implementation to vendor.
PRODUCT_PACKAGES += android.hardware.health@2.1-impl-<device>

# For non-A/B devices, also install the passthrough implementation to recovery.
PRODUCT_PACKAGES += android.hardware.health@2.1-impl-<device>.recovery
```

# Update necessary SELinux permissions {#selinux}

For example (replace `<device>` with the device name):
```
# device/<manufacturer>/<device>/sepolicy/vendor/hal_health_default.te
# Add device specific permissions to hal_health_default domain, especially
# if a device-specific libhealthd is used and/or device-specific storage related
# APIs are implemented.
```

# Fix `/charger` symlink {#charger-symlink}
If you are using `/charger` in your `init.rc` scripts, it is recommended
(required for devices running in Android R) that the path is changed to
`/system/bin/charger` instead.

Search for `service charger` in your device configuration directory to see if
this change applies to your device. Below is an example of how the script should
look like:

```
service charger /system/bin/charger
    class charger
    user system
    group system wakelock input
    capabilities SYS_BOOT
    file /dev/kmsg w
    file /sys/fs/pstore/console-ramoops-0 r
    file /sys/fs/pstore/console-ramoops r
    file /proc/last_kmsg r
```

# Appendix: sample code for the implementation {#impl}

## `device/<manufacturer>/<device>/health/Android.bp` {#android_bp}

```bp
cc_library_shared {
    name: "android.hardware.health@2.1-impl-<device>",
    stem: "android.hardware.health@2.0-impl-2.1-<device>",

    // Install to vendor and recovery.
    proprietary: true,
    recovery_available: true,

    relative_install_path: "hw",

    shared_libs: [
        "libbase",
        "libcutils",
        "libhidlbase",
        "liblog",
        "libutils",
        "android.hardware.health@2.1",
        "android.hardware.health@2.0",
    ],

    static_libs: [
        "android.hardware.health@1.0-convert",
        "libbatterymonitor",
        "libhealthloop",
        "libhealth2impl",
        // "libhealthd.<device>"
    ],

    srcs: [
        "HealthImpl.cpp",
    ],

    // No vintf_fragments because both -impl and -service should have been
    // installed.
}
```

## `device/<manufacturer>/<device>/health/HealthImpl.cpp` {#health_impl_cpp}

```c++
#include <memory>
#include <string_view>

#include <health/utils.h>
#include <health2impl/Health.h>
#include <hidl/Status.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::health::InitHealthdConfig;
using ::android::hardware::health::V2_1::IHealth;
using ::android::hidl::base::V1_0::IBase;

using namespace std::literals;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// android::hardware::health::V2_1::implementation::Health implements most
// defaults. Uncomment functions that you need to override.
class HealthImpl : public Health {
  public:
    HealthImpl(std::unique_ptr<healthd_config>&& config)
        : Health(std::move(config)) {}

    // A subclass can override this if these information should be retrieved
    // differently.
    // Return<void> getChargeCounter(getChargeCounter_cb _hidl_cb) override;
    // Return<void> getCurrentNow(getCurrentNow_cb _hidl_cb) override;
    // Return<void> getCurrentAverage(getCurrentAverage_cb _hidl_cb) override;
    // Return<void> getCapacity(getCapacity_cb _hidl_cb) override;
    // Return<void> getEnergyCounter(getEnergyCounter_cb _hidl_cb) override;
    // Return<void> getChargeStatus(getChargeStatus_cb _hidl_cb) override;
    // Return<void> getStorageInfo(getStorageInfo_cb _hidl_cb) override;
    // Return<void> getDiskStats(getDiskStats_cb _hidl_cb) override;
    // Return<void> getHealthInfo(getHealthInfo_cb _hidl_cb) override;

    // Functions introduced in Health HAL 2.1.
    // Return<void> getHealthConfig(getHealthConfig_cb _hidl_cb) override;
    // Return<void> getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) override;
    // Return<void> shouldKeepScreenOn(shouldKeepScreenOn_cb _hidl_cb) override;

  protected:
    // A subclass can override this to modify any health info object before
    // returning to clients. This is similar to healthd_board_battery_update().
    // By default, it does nothing.
    // void UpdateHealthInfo(HealthInfo* health_info) override;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    using ::android::hardware::health::V2_1::implementation::HealthImpl;
    if (instance != "default"sv) {
        return nullptr;
    }
    auto config = std::make_unique<healthd_config>();
    InitHealthdConfig(config.get());

    // healthd_board_init(config.get());

    return new HealthImpl(std::move(config));
}
```
