# Property Configuration Files

Each JSON file in this folder is a property configuration file for reference
Vehicle HAL. They contain VehiclePropConfig information along with initial
value information.

## JSON schema

Each JSON file must be in a schema like the following example:
(The comment starting with "//" is for documentation only and must be removed
from the actual JSON file. The "comment" field is used for comment in the
actual JSON file and will be ignored by the parser)

```
{
    // (number) The version for the JSON schema.
    "apiVersion": 2,
    // (non-empty array of objects) The property configuration list.
    //
    // Each object is a configuration for one property.
    "properties": [
        {
            // (number/string) The ID for the property.
            // This value is defined in a string value
            // which represents a constant value, see the "JSON Number-type
            // Field Values" section for detail.
            "property": "VehicleProperty::INFO_FUEL_CAPACITY",
            // (optional, number/string) The access mode for the property.
            // If specified, this overwrite the default access mode specified in
            // VehicleProperty.aidl. Must be specified for vendor properties.
            "access": "VehiclePropertyAccess::READ",
            // (optional, number/string) The change mode for the property.
            // If specified, this overwrite the default change mode specified in
            // VehicleProperty.aidl. Must be specified for vendor properties.
            "changeMode": "VehiclePropertyChangeMode::STATIC",
            // (optional, string) The config string.
            "configString": "blahblah",
            // (optional, array of number/string) The config array.
            "configArray": [1, 2, "Constants::HVAC_ALL"],
            // (optional, object) The default value for the property.
            // If not specified, the property will be shown as unavailable
            // until its value is set.
            "defaultValue": {
                // (optional, array of int number/string) Int values.
                "int32Values": [1, 2, "Constants::HVAC_ALL"],
                // (optional, array of int number/string) Long values.
                "int64Values": [1, 2],
                // (optional, array of float number/string) Float values.
                "floatValues": [1.1, 2.2],
                // (optional, string) String value.
                "stringValue": "test"
            },
            // (optional, number/string) The minimum sample rate in HZ.
            // Only work for VehiclePropertyChangeMode::CONTINUOUS property.
            // Must be specified for continuous property.
            "minSampleRate": 1,
            // (optional, number/string) The maximum sample rate in HZ.
            // Only work for VehiclePropertyChangeMode::CONTINUOUS property.
            // Must be specified for continuous property.
            "maxSampleRate": 10,
            // (optional, array of objects) The area configs.
            "areas:" [
                {
                    // (number/string) The area ID.
                    "areaId": "Constants::DOOR_1_LEFT",
                    // (optional number/string) The minimum int value.
                    "minInt32Value": 1,
                    // (optional number/string) The maximum int value.
                    "maxInt32Value": 10,
                    // (optional number/string) The minimum long value.
                    "minInt64Value": 1,
                    // (optional number/string) The maximum long value.
                    "maxInt64Value": 10,
                    // (optional number/string) The minimum float value.
                    "minFloatValue": 1,
                    // (optional number/string) The maximum float value.
                    "maxFloatValue": 10,
                    // (optional boolean since Android 15) Whether variable
                    // update rate is supported for continuous property.
                    "supportVariableUpdateRate": true,
                    // (optional since Android 15) If specified, then this
                    // [propId, areaId] supports the getSupportedValuesLists
                    // and getMinMaxSupportedValue API.
                    "hasSupportedValueInfo": {
                        // (optional boolean) Whether this [propId, areaId]
                        // specifies supported values list. Default to false.
                        "hasSupportedValuesList": true,
                        // (optional boolean) Whether this [propId, areaId]
                        // specifies min supported value. Default to false.
                        "hasMinSupportedValue": true,
                        // (optional boolean) Whether this [propId, areaId]
                        // specifies max supported value. Default to false.
                        "hasMaxSupportedValue": true
                    },
                    // (optional object) The default value for this area.
                    // Uses the same format as the "defaultValue" field for
                    // property object. If specified, this overwrite the global
                    // defaultValue.
                    "defaultValue": {
                        "int32Values": [1, 2, "Constants::HVAC_ALL"],
                        "int64Values": [1, 2],
                        "floatValues": [1.1, 2.2],
                        "stringValue": "test"
                    }
                }
            ]
        }
     ]
}
```

## API Version

The JSON file contains an `apiVersion` field which specified the schema version.
This field is useful for the JSON parser to decide how to parse the file. A
new-version parser is always capable of parsing an older version config file,
but not the other way around. For example, the latest VHAL config version
supported by the config parser used in the reference VHAL is V2, but a V1 config
file is still accepted.

New fields may be supported by each Android release (see the comment in the
schema). The parser used in the reference VHAL ignores all unknown fields, so
specifying unsupported fields will not cause error, but will be ignored.

A new api version usually introduces a backward-incompatible schema change.
Supporting a new field typically does not require a new api version.

### Version 1

This is the base config version introduced at Android 13.

### Version 2

This is introduced at Android 16.

#### Default areaId config at property level

In this version, we allow per-areaId config to be specified at the per-property
level to be the default config for all areaIds. For example, for the following
config:

```
{
    "apiVersion": 2,
    "properties": [
        {
            "property": "VehicleProperty::SEAT_BELT_BUCKLED",
            "defaultValue": {
                "int32Values": [
                    0
                ]
            },
            "areas": [
                {
                    "areaId": "Constants::SEAT_1_LEFT",
                    "defaultValue": {
                        "int32Values": [
                            1
                        ]
                    }
                },
                {
                    "areaId": "Constants::SEAT_1_RIGHT"
                },
                {
                    "areaId": "Constants::SEAT_2_LEFT"
                },
                {
                    "areaId": "Constants::SEAT_2_RIGHT"
                },
                {
                    "areaId": "Constants::SEAT_2_CENTER"
                }
            ]
        }
     ]
}
```

This means that all areaIds for SEAT_BELT_BUCKLED has the default value 0,
except for SEAT_1_LEFT, which has the default value 1.


## JSON Number-type Field Values

For number type field values, they can either be defined as a numeric number,
e.g., `{"minInt32Value": 1}` or be defined as a string which represents a
defined constant value, e.g.,
`{"property": "VehicleProperty::INFO_FUEL_CAPACITY"}`.

For constant values, they must be a string in the format of `XXX::XXX`, where
the field before `::` is the constant type, and the field after `::` is the
variable name.

We support the following constant types:

* VehiclePropertyAccess

* VehiclePropertyChangeMode

* VehicleGear

* VehicleAreaWindow

* VehicleOilLevel

* VehicleUnit

* VehicleSeatOccupancyState

* VehicleHvacFanDirection

* VehicleApPowerStateReport

* VehicleTurnSignal

* VehicleVendorPermission

* EvsServiceType

* EvsServiceState

* EvConnectorType

* VehicleProperty

* GsrComplianceRequirementType

* VehicleIgnitionState

* FuelType

* AutomaticEmergencyBrakingState

* ForwardCollisionWarningState

* BlindSpotWarningState

* LaneDepartureWarningState

* LaneKeepAssistState

* LaneCenteringAssistCommand

* LaneCenteringAssistState

* ErrorState

* WindshieldWipersState

* WindshieldWipersSwitch

* Constants

Every constant type except "Constants" corresponds to a enum defined in Vehicle
HAL interfac. E.g. "VehicleProperty" corresponds to the enums defined in
"VehicleProperty.aidl".

"Constants" type refers to the constant variables defined in the paresr.
Specifically, the "CONSTANTS_BY_NAME" map defined in "JsonConfigLoader.cpp".
