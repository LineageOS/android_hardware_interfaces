# Face Virtual HAL (VHAL)

This is a virtual HAL implementation that is backed by system properties
instead of actual hardware. It's intended for testing and UI development
on debuggable builds to allow devices to masquerade as alternative device
types and for emulators.
Note: The virtual face HAL feature development will be done in phases. Refer to this doc often for
the latest supported features

## Supported Devices

The face virtual hal is automatically built in in all debug builds (userdebug and eng) for the latest pixel devices and CF.
The instructions in this doc applies  to all

## Enabling Face Virtual HAL

On pixel devicse (non-CF), by default (after manufacture reset), Face VHAL is not enabled. Therefore real Face HAL is used.
Face VHAL enabling is gated by the following two AND conditions:
1. The Face VHAL feature flag (as part of Trunk-development strategy) must be tured until the flags life-cycle ends.
2. The Face VHAL must be enabled via sysprop

##Getting Stared

A basic use case for a successful authentication via Face VHAL is given as an exmple below.

### Enabling VHAL
```shell
$ adb root
$ adb shell device_config put biometrics_framework com.android.server.biometrics.face_vhal_feature true
$ adb shell settings put secure biometric_virtual_enabled 1
$ adb shell setprop persist.vendor.face.virtual.strength strong
$ adb shell setprop persist.vendor.face.virtual.type RGB
$ adb reboot
```

### Direct Enrollment
```shell
$ adb shell locksettings set-pin 0000
$ adb shell setprop persist.vendor.face.virtual.enrollments 1
$ adb shell cmd face syncadb shell cmd face sync
```

## Authenticating
To authenticate successfully, the captured (hit) must match the enrollment id set above. To  trigger
authentication failure, set the hit id to a different value.
```shell
$ adb shell setprop vendor.face.virtual.operation_authenticate_duration 800
$ adb shell setprop vendor.face.virtual.enrollment_hit 1
```
Note: At the initial phase of the Face VHAL development, Face-on-camera simulation is not supported, hence
the authentication immediately occurrs  as soon as the authentication request arriving at VHAL.


## Enrollment via Setup

```shell
# authenticar_id,bucket_id:duration:(true|false)....
$ adb shell setprop vendor.face.virtual.next_enrollment 1,0:500:true,5:250:true,10:150:true,15:500:true
$ walk thru the manual enrollment process by following screen instructions

# If you would like to get rid of the enrollment, run the follwoing command
$ adb shell setprop persist.vendor.face.virtual.enrollments \"\"
```
