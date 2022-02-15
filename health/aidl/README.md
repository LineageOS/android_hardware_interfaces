# Health AIDL HAL

## Determine whether the example service implementation is sufficient {#determine}

You need a custom implementation if any of the following is true:

* You are migrating from a custom
  [health 2.1 HIDL HAL implementation](../2.1/README.md).
* System properties `ro.charger.enable_suspend` and/or `ro.charger.no_ui`
  are set to a `true` value. See [below](#charger-sysprops).
* The device supports offline charging mode, and the `service`
  declaration with `class charger` in `init.rc` is different from the one
  provided by the example implementation. See [below](#charger-init-rc).

If the example HAL service is sufficient, [install it](#use-example). Otherwise,
[implement a custom HAL service](#use-custom).

### System properties for charger {#charger-sysprops}

The health AIDL HAL service also provides functionalities of `charger`. As a
result, the system charger at `/system/bin/charger` is deprecated.

However, the health AIDL HAL service is not allowed to read `ro.charger.*`
system properties. These properties include:
* `ro.charger.enable_suspend`. If set, you need a custom health AIDL HAL
  service. See [below](#charger-enable-suspend).
* `ro.charger.no_ui`. If set, you need a custom health AIDL HAL service.
  See [below](#charger-no-ui).
* `ro.charger.draw_split_screen`. The system property is deprecated.
* `ro.charger.draw_split_offset`. The system property is deprecated.
* `ro.charger.disable_init_blank`. The system property is deprecated.

If you need to set any of the deprecated system properties, contact
[OWNERS](OWNERS).

### Default `service` declaration for charger in `init.rc` {#charger-init-rc}

See
[android.hardware.health-service.example.rc](default/android.hardware.health-service.example.rc).

Check the `service` declaration in your device-specific `init.rc` file that
has `class charger`. Most likely, the declaration looks something like this
(Below is an excerpt from Pixel 3):

```text
service vendor.charger /system/bin/charger
    class charger
    seclabel u:r:charger:s0
    user system
    group system wakelock input
    capabilities SYS_BOOT
    file /dev/kmsg w
    file /sys/fs/pstore/console-ramoops-0 r
    file /sys/fs/pstore/console-ramoops r
    file /proc/last_kmsg r
```

Compare each line against the one provided by the example health AIDL HAL
service in
[android.hardware.health-service.example.rc](default/android.hardware.health-service.example.rc).
Specifically:

* For the `service` line, if the name of the service is **NOT**
  `vendor.charger`, and there are actions
  in the rc file triggered by `on property:init.svc.<name>=running` where
  `<name>` is the name of your charger service, then you need a custom health
  AIDL service.
* If your service belongs to additional classes beside `charger`, you need a
  custom health AIDL service.
* Modify the `seclabel` line. Replace `charger` with `charger_vendor`.
* If your service has a different `user` (not `system`), you need a custom
  health AIDL service.
* If your service belongs to additional `group`s beside
  `system wakelock input`, you need a custom health AIDL service.
* If your service requires additional capabilities beside `SYS_BOOT`,
  you need a custom health AIDL service.
* If your service requires additional `file`s to be opened prior to execution,
  you need a custom health AIDL service.

## Using the example health AIDL HAL service {#use-example}

If you [determined](#determine) that the example health AIDL HAL service works
for your device, install it with

```mk
PRODUCT_PACKAGES += \
    android.hardware.health-service.example \
    android.hardware.health-service.example_recovery \
```

Then, delete any existing `service` with `class charger` in your device-specific
`init.rc` files, because
[android.hardware.health-service.example.rc](default/android.hardware.health-service.example.rc)
already contains an entry for charger.

If your device supports charger mode and it has custom charger resources,
[move charger resources to `/vendor`](#charger-res)

## Implementing a custom health AIDL HAL service {#use-custom}

### Override the `Health` class {#health-impl}

See [`Health.h`](default/include/health-impl/Health.h) for its class
declaration. Inherit the class to customize for your device.

```c++
namespace aidl::android::hardware::health {
class HealthImpl : public Health {
    // ...
};
} // namespace aidl::android::hardware::health
int main(int, char**) {
    // ...
    auto binder = ndk::SharedRefBase::make<aidl::android::hardware::health::HealthImpl>(
            "default", std::move(config));
    // ...
}
```

* The logic to modify `healthd_config`, traditionally in `healthd_board_init()`
  should be called before passing the `healthd_config` struct to your
  `HealthImpl` class in [`main()`](#main).

* The following functions are similar to the ones in the health 2.1 HIDL HAL:

| AIDL implementation                 | HIDL implementation         |
|-------------------------------------|-----------------------------|
| `Health::getChargeCounterUah`       | `Health::getChargeCounter`  |
| `Health::getCurrentNowMicroamps`    | `Health::getCurrentNow`     |
| `Health::getCurrentAverageMicroamps`| `Health::getCurrentAverage` |
| `Health::getCapacity`               | `Health::getCapacity`       |
| `Health::getChargeStatus`           | `Health::getChargeStatus`   |
| `Health::getEnergyCounterNwh`       | `Health::getEnergyCounter`  |
| `Health::getDiskStats`              | `Health::getDiskStats`      |
| `Health::getStorageInfo`            | `Health::getStorageInfo`    |
| `Health::BinderEvent`               | `BinderHealth::BinderEvent` |
| `Health::dump`                      | `Health::debug`             |
| `Health::ShouldKeepScreenOn`        | `Health::shouldKeepScreenOn`|
| `Health::UpdateHealthInfo`          | `Health::UpdateHealthInfo`  |

### Implement `main()` {#main}

See the [`main.cpp`](default/main.cpp) for the example health AIDL service for
an example.

If you need to modify `healthd_config`, do it before passing it to the
constructor of `HealthImpl` (or `Health` if you did not implement a subclass
of it).

```c++
int main(int argc, char** argv) {
    auto config = std::make_unique<healthd_config>();
    ::android::hardware::health::InitHealthdConfig(config.get());
    healthd_board_init(config.get());
    auto binder = ndk::SharedRefBase::make<Health>("default", std::move(config));
    // ...
}
```

If your device does not support off-line charging mode, or does not have a UI
for charger (`ro.charger.no_ui=true`), skip the invocation of
`ChargerModeMain()` in `main()`.

### Build system changes

Install both the platform and recovery variant of the service. For example:

```mk
PRODUCT_PACKAGES += \
    android.hardware.health-service.cuttlefish \
    android.hardware.health-service.cuttlefish_recovery \
```

### SELinux rules

Add device specific permissions to the domain where the health HAL
process is executed, especially if a device-specific `libhealthd` is used
and/or device-specific storage related APIs are implemented.

Example (assuming that your health AIDL service runs in domain
`hal_health_tuna`:

```text
type hal_health_tuna, domain;
hal_server_domain(hal_health_tuna, hal_health)
type hal_health_tuna_exec, exec_type, vendor_file_type, file_type;

# allow hal_health_tuna ...;
```

If you did not define a separate domain, the domain is likely
`hal_health_default`. The device-specific rules for it is likely at
`device/<manufacturer>/<device>/sepolicy/vendor/hal_health_default.te`.
In this case, the aforementioned SELinux rules and types has already been
defined. You only need to add device-specific permissions.

```text
# allow hal_health_default ...;
```

### Implementing charger {#charger}

#### Move charger resources to `/vendor`

Ensure that charger resources are installed to `/vendor`, not `/product`.

`animation.txt` must be moved to the following location:

```text
/vendor/etc/res/values/charger/animation.txt
```

Charger resources in `/system` is not read by the health HAL service in
`/vendor`. Specifically, resources should be installed to the following
location:

```
/vendor/etc/res/images/charger/*.png
```

If resources are not found in these locations, the health HAL service falls
back to the following locations:

```
/vendor/etc/res/images/charger/default/*.png
```

You can use the default resources by installing the default module:

```makefile
PRODUCT_PACKAGES += charger_res_images_vendor
```

#### Modify `init.rc` for charger

It is recommended that you move the existing `service` entry with
`class charger` to the `init.rc` file in your custom health service.

If there are existing actions in the rc file triggered by
`on property:init.svc.<name>=running`, where `<name>` is the name of your
existing charger service (usually `vendor.charger`), then the name of the
service must be kept as-is. If you modify the name of the service, the actions
are not triggered properly.

Modify the entry to invoke the health service binary with `--charger` argument.
See
[android.hardware.health-service.example.rc](default/android.hardware.health-service.example.rc)
for an example:

```text
service vendor.charger /vendor/bin/hw/android.hardware.health-service-tuna --charger
    class charger
    seclabel u:r:charger_vendor:s0
    # ...
```

#### No charger mode {#no-charger}

If your device does not support off-line charging mode, skip the invocation of
`ChargerModeMain()` in `main()`.

```c++
int main(int, char**) {
    // ...
    // Skip checking if arguments contain "--charger"
    auto hal_health_loop = std::make_shared<HalHealthLoop>(binder, binder);
    return hal_health_loop->StartLoop();
}
```

You may optionally delete the `service` entry with `class charger` in the
`init.rc` file.

#### No charger UI {#charger-no-ui}

If your device does not have a UI for charger (`ro.charger.no_ui=true`), skip
the invocation of `ChargerModeMain()` in `main()`.

You may want to keep the `KernelLogger` so that charger still logs battery
information to the kernel logs.

```c++
int main(int argc, char** argv) {
    // ...
    if (argc >= 2 && argv[1] == "--charger"sv) {
        android::base::InitLogging(argv, &android::base::KernelLogger);
        // fallthrough to HalHealthLoop::StartLoop()
    }
    auto hal_health_loop = std::make_shared<HalHealthLoop>(binder, binder);
    return hal_health_loop->StartLoop();
}
```

#### Enable suspend {#charger-enable-suspend}

If your device has `ro.charger.enable_suspend=true`, implement a new class,
`ChargerCallbackImpl`, that inherits from
[`ChargerCallback`](default/include/health-impl/ChargerUtils.h). Then
override the `ChargerEnableSuspend` function to return `true`. Then pass an
instance of `ChargerCallbackImpl` to `ChargerModeMain()` instead.

```c++
namespace aidl::android::hardware::health {
class ChargerCallbackImpl : public ChargerCallback {
    bool ChargerEnableSuspend() override { return true; }
};
} // namespace aidl::android::hardware::health
int main(int argc, char** argv) {
    // ...
    if (argc >= 2 && argv[1] == "--charger"sv) {
        android::base::InitLogging(argv, &android::base::KernelLogger);
#if !CHARGER_FORCE_NO_UI
        return ChargerModeMain(binder,
                std::make_shared<aidl::android::hardware::health::ChargerCallbackImpl>(binder));
#endif
    }
    // ...
}
```

#### SELinux rules for charger

If your health AIDL service runs in a domain other than `hal_health_default`,
add `charger_type` to it so the health HAL service can have charger-specific
permissions. Example (assuming that your health AIDL service runs in domain
`hal_health_tuna`:

```text
domain_trans(init, hal_health_tuna_exec, charger_vendor)
```
