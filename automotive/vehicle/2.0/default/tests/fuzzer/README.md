# Fuzzer for android.hardware.automotive.vehicle@2.0-manager-lib

## Plugin Design Considerations
The fuzzer plugin for android.hardware.automotive.vehicle@2.0-manager-lib is
designed based on the understanding of the library and tries to achieve the following:

##### Maximize code coverage
The configuration parameters are not hardcoded, but instead selected based on
incoming data. This ensures more code paths are reached by the fuzzer.

Vehicle Manager supports the following parameters:
1. Vehicle Property (parameter name: `vehicleProp`)
2. Diagnostic Integer Sensor Index (parameter name: `diagnosticIntIndex`)
3. Diagnostic Float Sensor Index (parameter name: `diagnosticFloatIndex`)
4. Availability Message Type (parameter name: `availabilityMsgType`)
5. Subscription Message Type (parameter name: `subscriptionMsgType`)

| Parameter| Valid Values| Configured Value|
|------------- |-------------| ----- |
| `vehicleProp` | 0.`VehicleProperty::INVALID` 1.`VehicleProperty::HVAC_FAN_SPEED` 2.`VehicleProperty::INFO_MAKE` 3.`VehicleProperty::DISPLAY_BRIGHTNESS`  4.`VehicleProperty::INFO_FUEL_CAPACITY` 5.`VehicleProperty::HVAC_SEAT_TEMPERATURE`| Value obtained from FuzzedDataProvider |
| `diagnosticIntIndex`   | 0.`DiagnosticIntegerSensorIndex::FUEL_SYSTEM_STATUS` 1.`DiagnosticIntegerSensorIndex::MALFUNCTION_INDICATOR_LIGHT_ON` 2.`DiagnosticIntegerSensorIndex::NUM_OXYGEN_SENSORS_PRESENT` 3.`DiagnosticIntegerSensorIndex::FUEL_TYPE`  | Value obtained from FuzzedDataProvider |
| `diagnosticFloatIndex`   | 0.`DiagnosticFloatSensorIndex::CALCULATED_ENGINE_LOAD` 1.`DiagnosticFloatSensorIndex::SHORT_TERM_FUEL_TRIM_BANK1` 2.`DiagnosticFloatSensorIndex::LONG_TERM_FUEL_TRIM_BANK1` 3.`DiagnosticFloatSensorIndex::THROTTLE_POSITION`  | Value obtained from FuzzedDataProvider |
| `availabilityMsgType`   | 0.`VmsMessageType::AVAILABILITY_CHANGE` 1.`VmsMessageType::AVAILABILITY_RESPONSE` | Value obtained from FuzzedDataProvider |
| `subscriptionMsgType`   | 0.`VmsMessageType::SUBSCRIPTIONS_CHANGE` 1.`VmsMessageType::SUBSCRIPTIONS_RESPONSE` | Value obtained from FuzzedDataProvider |

This also ensures that the plugin is always deterministic for any given input.

## Build

This describes steps to build vehicleManager_fuzzer binary.

### Android

#### Steps to build
Build the fuzzer
```
  $ mm -j$(nproc) vehicleManager_fuzzer
```

#### Steps to run
Create a directory CORPUS_DIR
```
  $ adb shell mkdir /data/local/tmp/CORPUS_DIR
```

##### Some Additional steps needed to run the vehicleManager_fuzzer successfully on device

1. Push the following libraries from /vendor/lib/ and /vendor/lib64/ folders of your workspace to the device's /vendor/lib/ and /vendor/lib64/ :
```
1.1  android.hardware.automotive.vehicle@2.0.so
1.2  carwatchdog_aidl_interface-V2-ndk_platform.so
```
2. Now, reboot the device using command
```
  $ adb reboot
```

##### To run the fuzzer on device
```
  $ adb sync data
  $ adb shell LD_LIBRARY_PATH=/vendor/lib64 /data/fuzz/${TARGET_ARCH}/vehicleManager_fuzzer/vendor/vehicleManager_fuzzer /data/local/tmp/CORPUS_DIR
```

## References:
 * http://llvm.org/docs/LibFuzzer.html
 * https://github.com/google/oss-fuzz
