# Upgrading from Health 1.0 HAL

1. Remove `android.hardware.health@1.0*` from `PRODUCT_PACKAGES`
   in `device/<manufacturer>/<device>/device.mk`

1. If the device does not have a vendor-specific `libhealthd` AND does not
   implement storage-related APIs, just do the following:

   ```mk
   PRODUCT_PACKAGES += android.hardware.health@2.0-service
   ```

   Otherwise, continue to the next step.

1. Create directory
   `device/<manufacturer>/<device>/health`

1. Create `device/<manufacturer>/<device>/health/Android.bp`
   (or equivalent `device/<manufacturer>/<device>/health/Android.mk`)

    ```bp
    cc_binary {
        name: "android.hardware.health@2.0-service.<device>",
        init_rc: ["android.hardware.health@2.0-service.<device>.rc"],
        proprietary: true,
        relative_install_path: "hw",
        srcs: [
            "HealthService.cpp",
        ],

        cflags: [
            "-Wall",
            "-Werror",
        ],

        static_libs: [
            "android.hardware.health@2.0-impl",
            "android.hardware.health@1.0-convert",
            "libhealthservice",
            "libbatterymonitor",
        ],

        shared_libs: [
            "libbase",
            "libcutils",
            "libhidlbase",
            "libhidltransport",
            "libutils",
            "android.hardware.health@2.0",
        ],

        header_libs: ["libhealthd_headers"],

        overrides: [
            "healthd",
        ],
    }
    ```

    1. (recommended) To remove `healthd` from the build, keep "overrides" section.
    1. To keep `healthd` in the build, remove "overrides" section.

1. Create `device/<manufacturer>/<device>/health/android.hardware.health@2.0-service.<device>.rc`

    ```rc
    service vendor.health-hal-2-0 /vendor/bin/hw/android.hardware.health@2.0-service.<device>
        class hal
        user system
        group system
        file /dev/kmsg w
    ```

1. Create `device/<manufacturer>/<device>/health/HealthService.cpp`:

    ```c++
    #include <health2/service.h>
    int main() { return health_service_main(); }
    ```

1. `libhealthd` dependency:

    1. If the device has a vendor-specific `libhealthd.<soc>`, add it to static_libs.

    1. If the device does not have a vendor-specific `libhealthd`, add the following
        lines to `HealthService.cpp`:

        ```c++
        #include <healthd/healthd.h>
        void healthd_board_init(struct healthd_config*) {}

        int healthd_board_battery_update(struct android::BatteryProperties*) {
            // return 0 to log periodic polled battery status to kernel log
            return 0;
        }
        ```

1. Storage related APIs:

    1. If the device does not implement `IHealth.getDiskStats` and
        `IHealth.getStorageInfo`, add `libhealthstoragedefault` to `static_libs`.

    1. If the device implements one of these two APIs, add and implement the
        following functions in `HealthService.cpp`:

        ```c++
        void get_storage_info(std::vector<struct StorageInfo>& info) {
            // ...
        }
        void get_disk_stats(std::vector<struct DiskStats>& stats) {
            // ...
        }
        ```

1. Update necessary SELinux permissions. For example,

    ```
    # device/<manufacturer>/<device>/sepolicy/vendor/file_contexts
    /vendor/bin/hw/android\.hardware\.health@2\.0-service\.<device> u:object_r:hal_health_default_exec:s0

    # device/<manufacturer>/<device>/sepolicy/vendor/hal_health_default.te
    # Add device specific permissions to hal_health_default domain, especially
    # if a device-specific libhealthd is used and/or device-specific storage related
    # APIs are implemented.
    ```

1. Implementing health HAL in recovery. The health HAL is used for battery
status checks during OTA for non-A/B devices. If the health HAL is not
implemented in recovery, `is_battery_ok()` will always return `true`.

    1. If the device does not have a vendor-specific `libhealthd`, nothing needs to
    be done. A "backup" implementation is provided in
    `android.hardware.health@2.0-impl-default`, which is always installed to recovery
    image by default.

    1. If the device does have a vendor-specific `libhealthd`, implement the following
    module and include it in `PRODUCT_PACKAGES` (replace `<device>` with appropriate
    strings):

    ```bp
    // Android.bp
    cc_library_shared {
        name: "android.hardware.health@2.0-impl-<device>",
        recovery_available: true,
        relative_install_path: "hw",
        static_libs: [
            "android.hardware.health@2.0-impl",
            "libhealthd.<device>"
            // Include the following or implement device-specific storage APIs
            "libhealthstoragedefault",
        ],
        srcs: [
            "HealthImpl.cpp",
        ],
        overrides: [
            "android.hardware.health@2.0-impl-default",
        ],
    }
    ```

    ```c++
    // HealthImpl.cpp
    #include <health2/Health.h>
    #include <healthd/healthd.h>
    using android::hardware::health::V2_0::IHealth;
    using android::hardware::health::V2_0::implementation::Health;
    extern "C" IHealth* HIDL_FETCH_IHealth(const char* name) {
        const static std::string providedInstance{"default"};
        if (providedInstance != name) return nullptr;
        return Health::initInstance(&gHealthdConfig).get();
    }
    ```

    ```mk
    # device.mk
    PRODUCT_PACKAGES += android.hardware.health@2.0-impl-<device>
    ```
