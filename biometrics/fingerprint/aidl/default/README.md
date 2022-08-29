# Virtual Fingerprint HAL

This is a virtual HAL implementation that is backed by system properties instead
of actual hardware. It's intended for testing and UI development on debuggable
builds to allow devices to masquerade as alternative device types and for
emulators.

## Supported Devices

This HAL can be used on emulators, like cuttlefish, or on real devices. Add the
following to your device's `.mk` file to include it:

```
PRODUCT_PACKAGES_DEBUG += android.hardware.biometrics.fingerprint-service.example
```

The virtual HAL will be ignored if a real HAL is also installed on the target
device. Set the `biometric_virtual_enabled` settings and reboot the device to
switch to the virtual HAL. Unset it and reboot again to switch back.

## Getting Started

First, set the type of sensor the device should use, enable the virtual
extensions in the framework, and reboot.

```shell
$ adb root
$ adb shell settings put secure biometric_virtual_enabled 1
$ adb shell setprop persist.vendor.fingerprint.virtual.type rear
$ adb reboot
```

### Enrollments

Next, setup enrollments on the device. This can either be done through the UI,
or via adb directly.

#### Direct Enrollment

To set enrollment directly without the UI:

```shell
$ adb root
$ adb shell locksettings set-pin 0000
$ adb shell setprop persist.vendor.fingerprint.virtual.enrollments 1
$ adb shell cmd fingerprint sync
```

#### UI Enrollment

1. Set pin
      ```shell
      $ adb shell locksettings set-pin 0000
      ```
2. Tee up the results of the enrollment before starting the process:

      ```shell
      $ adb shell setprop vendor.fingerprint.virtual.next_enrollment 1:100,100,100:true
      ```
3. Navigate to `Settings -> Security -> Fingerprint Unlock` and follow the
   prompts.
4. Verify the enrollments in the UI:

      ```shell
      $ adb shell getprop persist.vendor.fingerprint.virtual.enrollments
      ```

### Authenticate

To authenticate successfully set the enrolled id that should succeed. Unset it
or change the value to make authenticate operations fail:

````shell
$ adb shell setprop vendor.fingerprint.virtual.enrollment_hit 1
````

### View HAL State

To view all the properties of the HAL (see `fingerprint.sysprop` file for the API):

```shell
$ adb shell getprop | grep vendor.fingerprint.virtual
```
