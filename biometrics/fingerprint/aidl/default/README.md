# Virtual Fingerprint HAL

This is a virtual HAL implementation that is backed by system properties
instead of actual hardware. It's intended for testing and UI development
on debuggable builds to allow devices to masquerade as alternative device
types and for emulators.

## Getting Started

First, set the type of sensor the device should use, enable the virtual
extensions in the framework, and reboot.

This doesn't work with HIDL and you typically need to have a PIN or password
set for things to work correctly, so this is a good time to set those too.

```shell
$ adb root
$ adb shell settings put secure biometric_virtual_enabled 1
$ adb shell setprop persist.vendor.fingerprint.virtual.type rear
$ adb shell locksettings set-pin 0000
$ adb shell settings put secure com.android.server.biometrics.AuthService.hidlDisabled 1
$ adb reboot
```

### Enrollments

Next, setup enrollments on the device. This can either be done through
the UI, or via adb. 

#### UI Enrollment

  1. Tee up the results of the enrollment before starting the process:

        ```shell
        $ adb shell setprop vendor.fingerprint.virtual.next_enrollment 1:100,100,100:true
        ```
  2. Navigate to `Settings -> Security -> Fingerprint Unlock` and follow the prompts.
  3. Verify the enrollments in the UI:

        ```shell
        $ adb shell getprop persist.vendor.fingerprint.virtual.enrollments
        ```

#### Direct Enrollment

To set enrollment directly without the UI:

```shell
$ adb root
$ adb shell setprop persist.vendor.fingerprint.virtual.enrollments 1
$ adb shell cmd fingerprint sync
```

Note: You may need to do this twice. The templates are checked
as part of some lazy operations, like user switching and startup, which can 
cause the framework to delete the enrollments before the sync operation runs.
Until this is fixed, just run the commands twice as a workaround.

### Authenticate

To authenticate successfully set the enrolled id that should succeed. Unset it
or change the value to make authenticate operations fail:

````shell
$ adb shell setprop vendor.fingerprint.virtual.enrollment_hit 1
````

### View HAL State

To view all the properties of the HAL (see `fingerprint.sysprop` for the API):

```shell
$ adb shell getprop | grep vendor.fingerprint.virtual
```
