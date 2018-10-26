# VINTF Device Manifest

In Android Pie, an `<fqname>` tag was introduced to be able to express multiple
different versions of the same HAL in VINTF manifests (for DRM)
in device manifest. For devices launching with previous versions of Android and
upgrading to Android Pie, the device manifest must not use `<fqname>` to
satisfy requirements for non-optional HALs, because older version of `libvintf`
do not recognize it, causing errors during OTA update.

Assuming that the HAL provides `@1.0::I*/default`,
`@1.1::I*/clearkey` and `@1.1::I*/foo` instances:

## Devices upgrading to Android Pie

### `target-level=1` or `target-level=2`

FCM (framework compatibility matrix) version 2 (released in Android Oreo MR1)
requires DRM 1.0. If the new device manifest has Target FCM Version (i.e.
`target-level`) 1 or 2, it should use the following snippet:

```xml
<hal format="hidl">
    <name>android.hardware.drm</name>
    <transport>hwbinder</transport>
    <version>1.0</version>
    <interface>
        <name>ICryptoFactory</name>
        <instance>default</instance>
    </interface>
    <interface>
        <name>IDrmFactory</name>
        <instance>default</instance>
    </interface>
    <fqname>@1.1::ICryptoFactory/clearkey</fqname>
    <fqname>@1.1::IDrmFactory/clearkey</fqname>
    <fqname>@1.1::ICryptoFactory/foo</fqname>
    <fqname>@1.1::IDrmFactory/foo</fqname>
</hal>
```

### `target-level=3`

FCM (framework compatibility matrix) version 3 (released in Android Pie)
requires DRM 1.1. If the new device manifest has Target FCM Version (i.e.
`target-level`) 3, it should use the following snippet:


```xml
<hal format="hidl">
    <name>android.hardware.drm</name>
    <transport>hwbinder</transport>
    <version>1.1</version>
    <interface>
        <name>ICryptoFactory</name>
        <instance>clearkey</instance>
        <instance>foo</instance>
    </interface>
    <interface>
        <name>IDrmFactory</name>
        <instance>clearkey</instance>
        <instance>foo</instance>
    </interface>
    <fqname>@1.0::ICryptoFactory/default</fqname>
    <fqname>@1.0::IDrmFactory/default</fqname>
</hal>
```

## Devices launching with Android Pie
If you have a new device launched with Android Pie (no OTA), both of the
aforementioned snippets can be used. Besides, it is recommended to use the
new, clearer format:

```xml
<hal format="hidl">
    <name>android.hardware.drm</name>
    <transport>hwbinder</transport>
    <fqname>@1.0::ICryptoFactory/default</fqname>
    <fqname>@1.0::IDrmFactory/default</fqname>
    <fqname>@1.1::ICryptoFactory/clearkey</fqname>
    <fqname>@1.1::IDrmFactory/clearkey</fqname>
    <fqname>@1.1::ICryptoFactory/foo</fqname>
    <fqname>@1.1::IDrmFactory/foo</fqname>
</hal>
```
