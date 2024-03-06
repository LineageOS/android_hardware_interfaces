# Face Virtual HAL (VHAL)

This is a virtual HAL implementation that is backed by system properties instead
of actual hardware. It's intended for testing and UI development on debuggable
builds to allow devices to masquerade as alternative device types and for
emulators. Note: The virtual face HAL feature development will be done in
phases. Refer to this doc often for the latest supported features

## Supported Devices

The face virtual hal is automatically built in in all debug builds (userdebug<br/>
and eng) for the latest pixel devices and CF. The instructions in this doc<br/>
applies to all

## Enabling Face Virtual HAL

On pixel devicse (non-CF), by default (after manufacture reset), Face VHAL is <br/>
not enabled. Therefore real Face HAL is used. Face VHAL enabling is gated by the<br/>
following two AND conditions:<br/>
1. The Face VHAL feature flag (as part ofTrunk-development strategy) must be<br/>
   turned on until the flags life-cycle ends.
2. The Face VHAL must be enabled via sysprop.

See the adb commands below

## Getting Stared

A basic use case for a successful authentication via Face VHAL is given as an
exmple below.

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

To authenticate successfully, the captured (hit) must match the enrollment id<br/>
set above. To trigger authentication failure, set the hit id to a different value.
`shell
$ adb shell setprop vendor.face.virtual.operation_authenticate_duration 800
$ adb shell setprop vendor.face.virtual.enrollment_hit 1`

### AcquiredInfo

AcquiredInfo codes can be sent during authentication by specifying the sysprop.<br/>
The codes is sent in sequence and in the interval of operation_authentication_duration/numberOfAcquiredInfoCode
`shell
$ adb shell setprop vendor.face.virtual.operation_authenticate_acquired 6,9,1013`
Refer to [AcquiredInfo.aidl](https://source.corp.google.com/h/googleplex-android/platform/superproject/main/+/main:hardware/interfaces/biometrics/face/aidl/android/hardware/biometrics/face/AcquiredInfo.aidl) for full face acquiredInfo codes.
Note: For vendor specific acquired info, acquiredInfo = 1000 + vendorCode.

### Error Insertion

Error can be inserted during authentction by specifying the authenticate_error
sysprop. `shell $ adb shell setprop
vendor.face.virtual.operation_authenticate_error 4` Refer to
[Error.aidl](https://source.corp.google.com/h/googleplex-android/platform/superproject/main/+/main:hardware/interfaces/biometrics/face/aidl/android/hardware/biometrics/face/Error.aidl)
for full face error codes

## Enrollment via Settings

Enrollment process is specified by sysprop `next_enrollment` in the following
format

```shell
Format: <id>:<progress_ms-[acquiredInfo,...],...:<success>
        ----:-----------------------------------:---------
        |           |                               |--->sucess (true/false)
        |           |--> progress_step(s)
        |
        |-->enrollment_id

E.g.
$ adb shell setprop vendor.face.virtual.next_enrollment 1:6000-[21,8,1,1108,1,10,1113,1,1118,1124]:true
```

If next_enrollment prop is not set, the following default value is used:<br/>
&nbsp;&nbsp;defaultNextEnrollment="1:1000-[21,7,1,1103],1500-[1108,1],2000-[1113,1],2500-[1118,1]:true"<br/>
Note: Enrollment data and configuration can be supported upon request in case of needs

## Lockout

Device lockout is based on the number of consecutive failed authentication attempts. There are a few
flavors of lockout mechanisms that are supported by virtula HAL <br/>

### Permanent Lockout

There are two sysprop to control permanent lockout <br/>
1. general lockout feature enable <br/>
2. threshold of failed attempts <br/>
`shell
$ adb shell setprop persist.vendor.face.virtual.lockout_enable true
$ adb shell setprop persist.vendor.face.virtual.lockout_permanent_threshold 3`

### Temporary Lockout

There are a few parameters to control temporary lockout (aka timed lockout): <br/>
1. enable lockout (general lockout feature enable, and timed lcokout enable) <br/>
2. threshold of failed attempts <br/>
3. timeout in ms <br/>
`shell
$ adb shell setprop persist.vendor.face.virtual.lockout_enable true
$ adb shell setprop persist.vendor.face.virtual.lockout_timed_enable true
$ adb shell setprop persist.vendor.face.virtual.lockout_timed_threshold 5
$ adb shell setprop persist.vendor.face.virtual.lockout_timed_duration 10000`

### Forced Lockout

A permanent lockout can be inserted on next authentication attempt independent of the failed <br/>
attempt count. This is a feature purely for test purpose.
`shell
$ adb shell setprop persist.vendor.face.virtual.lockout true`
