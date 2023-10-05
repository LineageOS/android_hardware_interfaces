# Virtual Face HAL

This is a virtual HAL implementation that is backed by system properties
instead of actual hardware. It's intended for testing and UI development
on debuggable builds to allow devices to masquerade as alternative device
types and for emulators.

## Device Selection

You can either run the FakeFaceEngine on a [real device](#actual-device) or a [virtual device/cuttlefish](#getting-started-on-a-virtual-device-cuttlefish). This document should
help you to get started on either one.

After setting up a device, go ahead and try out [enrolling](#enrolling) & [authenticating](#authenticating)

### Getting started on a Virtual Device (cuttlefish)


Note, I'm running this via a cloudtop virtual device.

1. Setup cuttlefish on cloudtop, See [this](https://g3doc.corp.google.com/company/teams/android/teampages/acloud/getting_started.md?cl=head) for more details.
2. acloud create --local-image
3. Enter in the shell command to disable hidl

```shell
$ adb root
$ adb shell settings put secure com.android.server.biometrics.AuthService.hidlDisabled 1
$ adb reboot
```
4. You should now be able to do fake enrollments and authentications (as seen down below)

### Actual Device

1. Modify your real devices make file (I.E. vendor/google/products/{YOUR_DEVICE}.mk)
2. Ensure that there is no other face HAL that is being included by the device
3. Add the following
```
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.biometrics.face.xml:$(TARGET_COPY_OUT_PRODUCT)/etc/permissions/android.hardware.biometrics.face.xml

PRODUCT_PACKAGES += \
    android.hardware.biometrics.face-service.example \

```
4. Now build and flash m -j120 && flash
5. Run the following commands

```shell
# This is a temporary workaround
$ adb root
$ adb shell setprop persist.vendor.face.virtual.type RGB
$ adb shell setprop persist.vendor.face.virtual.strength strong
$ adb shell locksettings set-pin 0000
$ adb reboot
```

## Enrolling

```shell
# authenticar_id,bucket_id:duration:(true|false)....
$ adb shell setprop vendor.face.virtual.next_enrollment 1,0:500:true,5:250:true,10:150:true,15:500:true
$ adb shell am start -n com.android.settings/.biometrics.face.FaceEnrollIntroduction
# If you would like to get rid of the enrollment, run the follwoing command
$ adb shell setprop persist.vendor.face.virtual.enrollments \"\"
```

## Authenticating

```shell
# If enrollment hasn't been setup
$ adb shell setprop persist.vendor.face.virtual.enrollments 1
$ adb shell cmd face sync
# After enrollment has been setup
$ adb shell setprop vendor.face.virtual.operation_authenticate_duration 800
$ adb shell setprop vendor.face.virtual.enrollment_hit 1
# Power button press to simulate auth
$ adb shell input keyevent 26
```
