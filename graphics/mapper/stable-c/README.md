# IMapper "stable-c" HAL

Starting with gralloc version 5, IMapper is now exposed as a C API instead of through HIDL or AIDL.
This is due to HIDL being deprecated, and AIDL not wanting to support a pass-through mode & pointers
for just a couple of clients such as IMapper. So instead a stable C API is used to fill this gap.

## Implementing

To provide an implementation a library implementing the AIMapper API interface should be provided
in `/vendor/lib[64]/hw/mapper.<imapper_suffix>.so`. The `<imapper_suffix>` should be specified
as the `<instance>` in the VINTF manifest `<interface>` section. For example:
```xml
<manifest version="1.0" type="device">
    <hal format="native">
        <name>mapper</name>
        <version>5.0</version>
        <interface>
            <instance>minigbm</instance>
        </interface>
    </hal>
</manifest>
```
defines that the IMapper 5.0 library is provided by `/vendor/lib[64]/hw/mapper.minigbm.so`.

This library must export the following `extern "C"` symbols:

### `ANDROID_HAL_STABLEC_VERSION`

This is a uint32_t that should simply be set to the exported AIMapper version. For example:
```c++
extern "C" uint32_t ANDROID_HAL_STABLEC_VERSION = AIMAPPER_VERSION_5;
```

### `AIMapper_loadIMapper`

This is what should actually load the HAL interface. The full type signature is
```c++
extern "C" AIMapper_Error AIMapper_loadIMapper(AIMapper* _Nullable* _Nonnull outImplementation)
```

See `include/android/hardware/graphics/mapper/IMapper.h` for complete documentation on what
this function must return.

To make it easier to implement this C API, a header-only helper library is provided called
`libimapper_providerutils`. This library handles mapping from the C API struct to a C++ class
as well as provides helpers for encoding & decoding metadata, largely replacing the role that
`libgralloctypes` filled with IMapper 4.

To use this library, create a class that extends from `IMapperV5Impl` and use `IMapperProvider` to
implement `AIMapper_loadIMapper`:

```c++
// The IMapper interface itself
#include <android/hardware/graphics/mapper/IMapper.h>
// Helpers for reading & writing metadata
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>
// Helper for providing the implementation interface
#include <android/hardware/graphics/mapper/utils/IMapperProvider.h>

// Define an IMapperV5 implementation
class CrosGrallocMapperV5 final : public vendor::mapper::IMapperV5Impl {
    // Override all the methods of IMapperV5Impl
      AIMapper_Error importBuffer(const native_handle_t* _Nonnull handle,
                              buffer_handle_t _Nullable* _Nonnull outBufferHandle) override;
      [etc...]
};

// Expose the required C symbols

extern "C" uint32_t ANDROID_HAL_STABLEC_VERSION = AIMAPPER_VERSION_5;

extern "C" AIMapper_Error AIMapper_loadIMapper(AIMapper* _Nullable* _Nonnull outImplementation) {
    // Define an IMapperProvider for our V5 implementation
    static vendor::mapper::IMapperProvider<CrosGrallocMapperV5> provider;
    return provider.load(outImplementation);
}
```

A complete example, including using IMapperMetadataTypes, can be found in the cuttlefish
implementation in `//external/minigbm/cros_gralloc/mapper_stablec`

### Testing

As with HIDL & AIDL HALs, a VTS test is provided to validate the implementation. It is found in the
`vts` folder and may be run using `$ atest VtsHalGraphicsMapperStableC_TargetTest`

## Using

It is strongly recommended that clients use either the `AHardwareBuffer` (preferred) or
`GraphicBufferMapper` (from libui) APIs to use the mapper HAL rather than attempting to use
`AIMapper` directly.

## Version changes

### Version 5

* Initial introduction of this HAL interface
* Largely feature-equivalent to IMapper4
* Requires allocator-V2
* Removes `BufferDescriptorInfo`;
* IsSupported has moved to IAllocator
* Removes `validateBufferSize`, validation is instead handled by clients using metadata queries
* Getting the following StandardMetadataType is now mandatory:
  * STRIDE
* Setting the following StandardMetadataTypes is now mandatory:
  * DATASPACE
  * SMPTE2086
  * CTA861_3
  * BLEND_MODE
