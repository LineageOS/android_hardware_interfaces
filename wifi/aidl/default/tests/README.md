# Vendor HAL gTest Suite

## Overview
Rather than testing an active instance of the service like the VTS tests,
this test suite will test individual files from the Vendor HAL.
This is especially useful for testing conversion methods (see `aidl_struct_util_unit_tests.cpp`),
but can also be used to test things like `wifi_chip`.

## Usage
Run the test script with a device connected:

```
./runtests.sh
```
