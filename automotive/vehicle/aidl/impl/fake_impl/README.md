# Fake reference AIDL VHAL implementation libraries
---

This directory stores libraries for implementing a fake reference AIDL VHAL.

WARNING: All the libraries here are for TEST ONLY.

## GeneratorHub

Defines a library `FakeVehicleHalValueGenerators` that could generate fake
vehicle property values for testing.

## hardware

Defines a fake implementation for device-specifc interface `IVehicleHardware`:
`FakeVehicleHardware`. This implementation uses a in-memory map for storing
property values and does not communicate with or depending on any specific
vehicle bus.

## obd2frame

Defines a library `FakeObd2Frame` that generates fake OBD2 frame for OBD2
properties.

## userhal

Defines a library `FakeUserHal` that emulates a real User HAL behavior by
parsing debug commands.
