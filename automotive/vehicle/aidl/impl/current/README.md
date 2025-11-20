# AIDL VHAL V4 libraries and reference implementation.
---

This directory stores the libraries useful for implementing vendor AIDL VHAL V4.
This directory also stores a reference fake implementation for AIDL VHAL V4.

## default_config

Stores the default vehicle property configurations for reference vehicle HAL.
Vendor implementation could copy this library but must update the configuration
to meet their own requirements, e.g. enable or disable certain properties or
update the initial value for certain properties.

##	fake_impl

Contains libraries used specifically for the fake reference VHAL implementation.
These libraries are for test only and must not be directly used for vendor
VHAL implementation.

These libraries contain test-spcific logic and must not run directly on a real
vehicle.

## grpc

Stores code for GRPC based VHAL implementation.

The GRPC based VHAL implementation delegates most of the VHAL logic to a VHAL
proxy server running remotely. For example, the server may run in a separate
Android (maybe not AAOS) VM or a non-Android machine running in the same
ethernet.

This is used in AAOS cuttlefish, where the VHAL proxy server is running on the
host machine with AAOS running in a VM.

The GRPC VHAL uses `DefaultVehicleHal`, but with a special
`IVehicleHardware` implementation: `GRPCVehicleHardware`. The
`GRPCVehicleHardware` is a thin proxy layer that forwards all requests to a
GRPC server (`GRPCVehicleProxyServer`).

The supported communication channels between GRPC VHAL and the GRPC Vehicle
proxy server are Ethernet or Vsock.

Note that VHAL must be ready early on during the boot so the communciation
channel setup step must happen very early before VHAL registers itself.

## hardware

Defines an interface `IVehicleHardware.h` which vendor must implement for
vehicle-specific logic if they want to follow our reference VHAL design.

## proto

Stores Some protobuf files translated from AIDL VHAL interface types. These
files are used in GRPC VHAL implementation.

## utils

Defines a library `VehicleHalUtils` which provides useful utility functions for
VHAL implementation. Vendor VHAL could use this library.

## vhal

Defines a library `DefaultVehicleHal` which provides generic logic for all VHAL
implementations (including reference VHAL). Vendor VHAL implementation could
use this library, along with their own implementation for `IVehicleHardware`
interface.

Also defines a binary `android.hardware.automotive.vehicle@V4-default-service`
which is the reference VHAL implementation. It implements `IVehicle.aidl`
interface. It uses `DefaultVehicleHal`, along with `FakeVehicleHardware`
(in fake_impl). It simulates the vehicle bus interaction by using an
in-memory map. Meaning that all properties (except for some special ones) are
just written into a hash map and read from a hash map without relying on any
hardware. As a result, the reference implementation can run on emulator or
any host environment.

Vendor must not directly use the reference implementation for a real vehicle.