/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleHalTypes_H_
#define android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleHalTypes_H_

#include <aidl/android/hardware/automotive/vehicle/AutomaticEmergencyBrakingState.h>
#include <aidl/android/hardware/automotive/vehicle/BlindSpotWarningState.h>
#include <aidl/android/hardware/automotive/vehicle/CrossTrafficMonitoringWarningState.h>
#include <aidl/android/hardware/automotive/vehicle/CruiseControlCommand.h>
#include <aidl/android/hardware/automotive/vehicle/CruiseControlState.h>
#include <aidl/android/hardware/automotive/vehicle/CruiseControlType.h>
#include <aidl/android/hardware/automotive/vehicle/DiagnosticFloatSensorIndex.h>
#include <aidl/android/hardware/automotive/vehicle/DiagnosticIntegerSensorIndex.h>
#include <aidl/android/hardware/automotive/vehicle/DriverDistractionState.h>
#include <aidl/android/hardware/automotive/vehicle/DriverDistractionWarning.h>
#include <aidl/android/hardware/automotive/vehicle/DriverDrowsinessAttentionState.h>
#include <aidl/android/hardware/automotive/vehicle/DriverDrowsinessAttentionWarning.h>
#include <aidl/android/hardware/automotive/vehicle/ElectronicStabilityControlState.h>
#include <aidl/android/hardware/automotive/vehicle/EmergencyLaneKeepAssistState.h>
#include <aidl/android/hardware/automotive/vehicle/ErrorState.h>
#include <aidl/android/hardware/automotive/vehicle/EvConnectorType.h>
#include <aidl/android/hardware/automotive/vehicle/EvStoppingMode.h>
#include <aidl/android/hardware/automotive/vehicle/EvsServiceState.h>
#include <aidl/android/hardware/automotive/vehicle/EvsServiceType.h>
#include <aidl/android/hardware/automotive/vehicle/ForwardCollisionWarningState.h>
#include <aidl/android/hardware/automotive/vehicle/FuelType.h>
#include <aidl/android/hardware/automotive/vehicle/GetValueRequest.h>
#include <aidl/android/hardware/automotive/vehicle/GetValueResult.h>
#include <aidl/android/hardware/automotive/vehicle/GetValueResults.h>
#include <aidl/android/hardware/automotive/vehicle/GsrComplianceRequirementType.h>
#include <aidl/android/hardware/automotive/vehicle/HandsOnDetectionDriverState.h>
#include <aidl/android/hardware/automotive/vehicle/HandsOnDetectionWarning.h>
#include <aidl/android/hardware/automotive/vehicle/ImpactSensorLocation.h>
#include <aidl/android/hardware/automotive/vehicle/LaneCenteringAssistCommand.h>
#include <aidl/android/hardware/automotive/vehicle/LaneCenteringAssistState.h>
#include <aidl/android/hardware/automotive/vehicle/LaneDepartureWarningState.h>
#include <aidl/android/hardware/automotive/vehicle/LaneKeepAssistState.h>
#include <aidl/android/hardware/automotive/vehicle/LocationCharacterization.h>
#include <aidl/android/hardware/automotive/vehicle/LowSpeedCollisionWarningState.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2CommonIgnitionMonitors.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2FuelSystemStatus.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2FuelType.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2IgnitionMonitorKind.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2SecondaryAirStatus.h>
#include <aidl/android/hardware/automotive/vehicle/Obd2SparkIgnitionMonitors.h>
#include <aidl/android/hardware/automotive/vehicle/PortLocationType.h>
#include <aidl/android/hardware/automotive/vehicle/SetValueRequest.h>
#include <aidl/android/hardware/automotive/vehicle/SetValueResult.h>
#include <aidl/android/hardware/automotive/vehicle/SetValueResults.h>
#include <aidl/android/hardware/automotive/vehicle/StatusCode.h>
#include <aidl/android/hardware/automotive/vehicle/SubscribeOptions.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAirbagLocation.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleApPowerStateReport.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleApPowerStateReq.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleArea.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAreaDoor.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAreaMirror.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAreaSeat.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAreaWheel.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAreaWindow.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleAutonomousState.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleGear.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleHvacFanDirection.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleIgnitionState.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleLightState.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleLightSwitch.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleOilLevel.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropConfig.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropConfigs.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropError.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropValue.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleProperty.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyAccess.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyChangeMode.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyGroup.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyStatus.h>
#include <aidl/android/hardware/automotive/vehicle/VehiclePropertyType.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleSeatOccupancyState.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleTurnSignal.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleUnit.h>
#include <aidl/android/hardware/automotive/vehicle/VehicleVendorPermission.h>
#include <aidl/android/hardware/automotive/vehicle/WindshieldWipersState.h>
#include <aidl/android/hardware/automotive/vehicle/WindshieldWipersSwitch.h>

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_VehicleHalTypes_H_
