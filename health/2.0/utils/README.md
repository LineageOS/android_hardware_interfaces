# libhealthhalutils

A convenience library for (hwbinder) clients of health HAL to choose between
the "default" instance (served by vendor service) or "backup" instance (served
by healthd). C++ clients of health HAL should use this library instead of
calling `IHealth::getService()` directly.

Its Java equivalent can be found in `BatteryService.HealthServiceWrapper`.
