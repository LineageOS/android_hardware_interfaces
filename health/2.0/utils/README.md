# libhealthhalutils

A convenience library for (hwbinder) clients of health HAL to choose between
the "default" instance (served by vendor service) or "backup" instance (served
by healthd). C++ clients of health HAL should use this library instead of
calling `IHealth::getService()` directly.

Its Java equivalent can be found in `BatteryService.HealthServiceWrapper`.

# libhealthservice

Common code for all (hwbinder) services of the health HAL, including healthd and
vendor health service `android.hardware.health@2.0-service(.<vendor>)`. `main()` in
those binaries calls `health_service_main()` directly.

# libhealthstoragedefault

Default implementation for storage related APIs for (hwbinder) services of the
health HAL. If an implementation of the health HAL do not wish to provide any
storage info, include this library. Otherwise, it should implement the following
two functions:

```c++
void get_storage_info(std::vector<struct StorageInfo>& info) {
    // ...
}
void get_disk_stats(std::vector<struct DiskStats>& stats) {
    // ...
}
```
