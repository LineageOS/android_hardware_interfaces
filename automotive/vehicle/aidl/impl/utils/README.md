# Utility classes for VHAL implementation
---

This directory stores utility classes for VHAL implementation. Vendor
implementation could use utility classes from `common` folder in their
VHAL implementation.

## common

Defines common utility libraries.

### ConcurrentQueue

Provides a thread-safe concurrent queue object. Useful for adding object to
a queue in one thread (usually binder thread) and handle the objects in a
separate handler thread.

### ParcelableUtils

Provides functions to convert between a regular parcelable and a
`LargeParcelabe`.

A `LargeParcelable` is a parcelable that marshals the payload
into a shared memory file if the payload is too large to pass across binder.
It is used to pass large data across binder. Before sending the data, VHAL
impl should convert a regular parcelabe to a `LargeParcelable`. After receving
data, VHAL impl should convert a `LargeParcelable` back to regular parcelabe.

### PendingRequestPool

Defines A class for managing pending requests and automatically call timeout
callback if the request timed-out.

### PropertyUtils

Defines some useful constants.

### RecurrentTimer

Defines a thread-safe recurrent timer that can call a function periodically.

### VehicleHalTypes

Provides a header file that includes many commonly used header files. Useful
when you are using multiple types defined in VHAL interface.

### VehicleObjectPool

Defines a reusable in-memory pool for `VehiclePropValue`.

### VehiclePropertyStore

Defines an in-memory map for storing vehicle properties. Allows easier insert,
delete and lookup.

### VehicleUtils

Defines many useful utility functions.

## test_vendor_properties

Contains vendor properties used for testing purpose in reference VHAL.
