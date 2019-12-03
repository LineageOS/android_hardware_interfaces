# Implement the 2.1 HAL instead!

It is strongly recommended that you implement the 2.1 HAL directly. See
`hardware/interfaces/health/2.1/README.md` for more details.

# Implement Health 1.0 HAL

1. Install common binderized service. The binderized service `dlopen()`s
   passthrough implementations on the device, so there is no need to write
   your own.

    ```mk
    # Install default binderized implementation to vendor.
    PRODUCT_PACKAGES += android.hardware.health@1.0-service
    ```

1. Add proper VINTF manifest entry to your device manifest. Example:

    ```xml
    <hal format="hidl">
        <name>android.hardware.health</name>
        <transport>hwbinder</transport>
        <version>1.0</version>
        <interface>
            <name>IHealth</name>
            <instance>default</instance>
        </interface>
    </hal>
    ```

1. Install the proper passthrough implemetation.

    1. If you want to use the default implementation (with default `libhealthd`),
       add the following to `device.mk`:

        ```mk
        PRODUCT_PACKAGES += \
            android.hardware.health@1.0-impl
        ```

    1. Otherwise, if you have a customized `libhealthd.<board>`:

        1. Define your passthrough implementation. Example (replace `<device>`
           and `<board>` accordingly):

            ```bp
            cc_library_shared {
                name: "android.hardware.health@1.0-impl-<device>",
                vendor: true,
                relative_install_path: "hw",

                static_libs: [
                    "android.hardware.health@1.0-impl-helper",
                    "android.hardware.health@1.0-convert",
                    "libhealthd.<board>",
                ],
            }
            ```

        1. Add to `device.mk`.

            ```
            PRODUCT_PACKAGES += android.hardware.health@1.0-impl-<device>
            ```

        1. Define appropriate SELinux permissions.
