# Virtual Fingerprint HAL

This is a virtual HAL implementation that is backed by system properties instead
of actual hardware. It's intended for testing and UI development on debuggable
builds to allow devices to masquerade as alternative device types and for
emulators.

## Supported Devices

This HAL can be used on emulators, like cuttlefish, or on real devices. Add the
following to your device's `.mk` file to include it:

```
PRODUCT_PACKAGES_DEBUG += com.android.hardware.biometrics.fingerprint.virtual
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

## Authenticate

To authenticate successfully set the enrolled id that should succeed. Unset it
or change the value to make authenticate operations fail:

````shell
$ adb shell setprop vendor.fingerprint.virtual.enrollment_hit 1
````

## Acquired Info Insertion

Fingerprint image acquisition states at HAL are reported to framework via onAcquired() callback. The valid acquired state info for AIDL HAL include

{UNKNOWN(0), GOOD(1), PARTIAL(2), INSUFFICIENT(3), SENSOR_DIRTY(4), TOO_SLOW(5), TOO_FAST(6), VENDOR(7), START(8), TOO_DARK(9), TOO_BRIGHT(10), IMMOBILE(11), RETRYING_CAPTURE(12)}

Refer to [AcquiredInfo.aidl](../android/hardware/biometrics/fingerprint/AcquiredInfo.aidl) for details


The states can be specified in sequence for the HAL operations involving fingerprint image captures, namely authenticate, enrollment and detectInteraction

```shell
$ adb shell setprop vendor.fingerprint.virtual.operation_authenticate_acquired 6,9,1
$ adb shell setprop vendor.fingerprint.virtual.operation_detect_interaction_acquired 6,1
$ adb shell setprop vendor.fingerprint.virtual.next_enrollment 2:1000-[5,1],500:true

#next_enrollment format example:
.---------------------- enrollment id (2)
|   .------------------ the image capture 1 duration (1000ms)
|   |   .--------------   acquired info first (TOO_SLOW)
|   |   | .------------   acquired info second (GOOD)
|   |   | |   .-------- the image capture 2 duration (500ms)
|   |   | |   |   .---- enrollment end status (success)
|   |   | |   |   |
|   |   | |   |   |
|   |   | |   |   |
2:1000-[5,1],500:true
```
For vendor specific acquired info, acquiredInfo = 1000 + vendorAcquiredInfo

## Error Insertion
The valid error codes for AIDL HAL include

{UNKNOWN(0), HW_UNAVAILABLE(1), UNABLE_TO_PROCESS(2), TIMEOUT(3), NO_SPACE(4), CANCELED(5), UNABLE_TO_REMOVE(6), VENDOR(7), BAD_CALIBRATION(8)}

Refer to [Error.aidl](../android/hardware/biometrics/fingerprint/Error.aidl) for details


There are many HAL operations which can result in errors, refer to [here](fingerprint.sysprop) file for details.

```shell
$ adb shell setprop vendor.fingerprint.virtual.operation_authenticate_error 8
```
For vendor specific error, errorCode = 1000 + vendorErrorCode

## Latency Insertion
Three HAL operations (authenticate, enrollment and detect interaction) latency can be optionally specified in multiple ways
1. default latency is fixed at 400 ms if not specified via sysprop
2. specify authenticate operation latency to 900 ms
      ```shell adb shell setprop vendor.fingerprint.virtual.operation_authenticate_latency 900```
3. specify authenticate operation latency between 600 to 1200 ms in unifrom distribution
      ```shelladb shell setprop vendor.fingerprint.virtual.operation_authenticate_latency 600,1200```

## Lockout
To force the device into lockout state
```shell
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout true
```
To test permanent lockout based on the failed authentication attempts (e.g. 7)
```shell
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout_permanent_threshold 7
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout_enable true
```
To test timed lockout based on the failed authentication attempts (e.g. 8 seconds on 5 attempts)
```shell
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout_timed_duration 8000
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout_timed_threshold 5
$ adb shell setprop persist.vendor.fingerprint.virtual.lockout_enable true
```

## Reset all configurations to default
The following command will reset virtual configurations (related system properties) to default value.
```shell
$ adb shell cmd android.hardware.biometrics.fingerprint.IFingerprint/virtual resetconfig
$ adb reboot
```

## View HAL State

To view all the properties of the HAL (see `fingerprint.sysprop` file for the API):

```shell
$ adb shell getprop | grep vendor.fingerprint.virtual
```
To dump virtual HAL internal data
```shell
adb shell dumpsys android.hardware.biometrics.fingerprint.IFingerprint/virtual
```
