# Remote Provisioning Changelog

This document provides an exact description of which changes have occurred in the
`IRemotelyProvisionedComponent` HAL interface in each Android release.

## Releases
* **Android S (12):** IRemotelyProvisionedComponent v1
* **Android T (13):** IRemotelyProvisionedComponent v2

## IRemotelyProvisionedComponent 1 -> 2
* DeviceInfo
 * Most entries are no longer optional.
 * `att_id_state` is now `fused`. `fused` is used to indicate if SecureBoot is enabled.
 * `version` is now `2`.
 * `board` has been removed.
 * `device` has been added.
* RpcHardwareInfo
 * `uniqueId` String added as a field in order to differentiate IRPC instances on device.