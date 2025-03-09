# IHDCPAuthControl as a Trusted HAL service

IHDCPAuthControl is expected to be a service implemented in a TEE.
We provide a default reference implementation and its integration in Trusty
as an example.

The VTS test for a Trusted HAL service ought to run in the VM.
We provide an integration of the VTS test in a Trusty VM,
and later in a Microdroid VM (b/380632474).

This interface shall not be exposed to the host and thus shall be part of
the list of excluded interfaces from
[compatibility_matrices/exclude/fcm_exclude.cpp](../../../compatibility_matrices/exclude/fcm_exclude.cpp)

## 1. Mock Implementation

The mock implementation under default/src/lib.rs is expected to be integrated in a
TEE. For AOSP testing we offer two virtual device testing options:

- Cuttlefish AVD, where the reference implementation is integrated in an AVF VM, emulating a TEE.
- Trusty QEMU AVD, where the reference implementation is integrated in a Trusty TEE image (executed in secure world)

### 1.1. Cuttlefish: Integrate in an AVF HAL pVM (Trusty)

In Cuttlefish, we emulate a TEE with an AVF Trusty pVM.
The VM2TZ IPC is emulated with a vsock port forward utility (b/379582767).

Until vsock port forwarding is supported, the trusty_test_vm is used temporarily.
(VTS tests and HAL implementation will be in same pVM).

TODO: complete when trusty_hal_vm is created

In order to add the mock HdcpAuthControlService to the trusty_test_vm, make sure
that `hardware/interfaces/security/see/hdcp/default` is added to the
trusty_test_vm makefile, by adding it to
[trusty/device/x86/generic-x86_64/project/generic-x86_64-inc.mk](../../../../../trusty/device/x86/generic-x86_64/project/generic-x86_64-inc.mk)

### 1.2. Trusty QEMU AVD: Integrate as a TA in Trusty TEE

In order to add the mock HdcpAuthControlService to the Trusty TEE, make sure
that `hardware/interfaces/security/see/hdcp/default` is added to
[trusty/device/arm/generic-arm64/project/generic-arm-inc.mk](../../../../../trusty/device/arm/generic-arm64/project/generic-arm-inc.mk)


## 2. VTS Tests

IHdcpAuthControl service is expected to only be exposed to AVF pVM.

The VTS tests shall verify:

- IHdcpAuthControl cannot be accessed from the Android Host:

   see [aidl/vts/src/host_test.rs](aidl/vts/host_test.rs)

- IHdcpAuthControl can be accessed from an AVF pVM:

   see [aidl/vts/src/vm_test.rs](aidl/vts/src/vm_test.rs)
   see [aidl/vts/AndroidTest.xml](aidl/vts/AndroidTest.xml)


To integrate the VTS test in the trusty_test_vm:

1.
1. add the test to [hardware/interfaces/security/see/usertests-rust-inc.mk](../usertests-rust-inc.mk)

