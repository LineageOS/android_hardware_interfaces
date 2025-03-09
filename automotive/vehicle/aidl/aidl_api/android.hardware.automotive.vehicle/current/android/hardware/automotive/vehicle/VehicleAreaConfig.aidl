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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.automotive.vehicle;
@JavaDerive(equals=true, toString=true) @RustDerive(Clone=true) @VintfStability
parcelable VehicleAreaConfig {
  int areaId;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code INT32} type property. Ignored for other types. The optional minimum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the min supported value ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minInt32Value} and {@code maxInt32Value} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  int minInt32Value;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code INT32} type property. Ignored for other types. The optional maximum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the max supported value ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minInt32Value} and {@code maxInt32Value} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  int maxInt32Value;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code INT64} type property. Ignored for other types. The optional minimum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the min supported value ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minInt64Value} and {@code maxInt64Value} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  long minInt64Value;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code INT64} type property. Ignored for other types. The optional maximum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the max supported value ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minInt64Value} and {@code maxInt64Value} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  long maxInt64Value;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code FLOAT} type property. Ignored for other types. The optional minimum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the min supported value ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minFloatValue} and {@code maxFloatValue} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  float minFloatValue;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for {@code FLOAT} type property. Ignored for other types. The optional maximum value at boot time. For backward compatibility, if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true}, and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true}, this must be equal to the max supported value ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time. If no minimum or maximum value is available at boot time, both {@code minFloatValue} and {@code maxFloatValue} must be set to 0. If either one is not 0, then we assume min and max both take effect.
   */
  float maxFloatValue;
  /**
   * @deprecated client should use {@code getMinMaxSupportedValue} instead. Only applicable for property with {@code @data_enum} annotation. Ignored for other properties. Optional supported subset of supported values at boot time. If the property has a @data_enum and supportedEnumValues is {@code null}, then it is assumed all @data_enum values are supported unless specified through another mechanism. For backward compatibility, if {@code HasSupportedValueInfo.hasSupportedValuesList} is {@code true} and this property has {@code data_enum} annotation, this must be set to the same as {@code SupportedValuesListResult.supportedValuesList} at boot time.
   */
  @nullable long[] supportedEnumValues;
  android.hardware.automotive.vehicle.VehiclePropertyAccess access = android.hardware.automotive.vehicle.VehiclePropertyAccess.NONE;
  boolean supportVariableUpdateRate;
  @nullable android.hardware.automotive.vehicle.HasSupportedValueInfo hasSupportedValueInfo;
}
