# AIDL VHAL libraries and reference implementation.
---

This directory stores the libraries useful for implementing vendor AIDL VHAL.
This directory also stores a reference fake implementation for AIDL VHAL.

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

Also defines a binary `android.hardware.automotive.vehicle@V3-default-service`
which is the reference VHAL implementation. It implements `IVehicle.aidl`
interface. It uses `DefaultVehicleHal`, along with `FakeVehicleHardware`
(in fake_impl). It simulates the vehicle bus interaction by using an
in-memory map. Meaning that all properties (except for some special ones) are
just written into a hash map and read from a hash map without relying on any
hardware. As a result, the reference implementation can run on emulator or
any host environment.

Vendor must not directly use the reference implementation for a real vehicle.