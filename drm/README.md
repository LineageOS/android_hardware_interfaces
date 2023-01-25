# DRM HAL

This is the underlying HAL implementation for `MediaDrm`/`MediaCrypto` (and
their NDK counterparts).

## Plugin-vendor-specific VTS modules

The interface `DrmHalVTSVendorModule_V1` is compatible with all versions of the
DRM HAL (hidl 1.0-1.4, aidl).

Please see `./1.0/vts/doc/Drm_Vendor_Modules_v1.pdf`.

TODO(b/266091099): convert `Drm_Vendor_Modules_v1.pdf` to Markdown.