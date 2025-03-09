/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;
@VintfStability
interface IMediaQuality {
  void setCallback(in android.hardware.tv.mediaquality.IMediaQualityCallback callback);
  void setAmbientBacklightDetector(in android.hardware.tv.mediaquality.AmbientBacklightSettings settings);
  void setAmbientBacklightDetectionEnabled(in boolean enabled);
  boolean getAmbientBacklightDetectionEnabled();
  boolean isAutoPqSupported();
  boolean getAutoPqEnabled();
  void setAutoPqEnabled(boolean enable);
  boolean isAutoSrSupported();
  boolean getAutoSrEnabled();
  void setAutoSrEnabled(boolean enable);
  boolean isAutoAqSupported();
  boolean getAutoAqEnabled();
  void setAutoAqEnabled(boolean enable);
  android.hardware.tv.mediaquality.IPictureProfileChangedListener getPictureProfileListener();
  void setPictureProfileAdjustmentListener(android.hardware.tv.mediaquality.IPictureProfileAdjustmentListener listener);
  void sendDefaultPictureParameters(in android.hardware.tv.mediaquality.PictureParameters pictureParameters);
  android.hardware.tv.mediaquality.ISoundProfileChangedListener getSoundProfileListener();
  void setSoundProfileAdjustmentListener(android.hardware.tv.mediaquality.ISoundProfileAdjustmentListener listener);
  void sendDefaultSoundParameters(in android.hardware.tv.mediaquality.SoundParameters soundParameters);
  void getParamCaps(in android.hardware.tv.mediaquality.ParameterName[] paramNames, out android.hardware.tv.mediaquality.ParamCapability[] caps);
  void getVendorParamCaps(in android.hardware.tv.mediaquality.VendorParameterIdentifier[] names, out android.hardware.tv.mediaquality.VendorParamCapability[] caps);
  void sendPictureParameters(in android.hardware.tv.mediaquality.PictureParameters pictureParameters);
  void sendSoundParameters(in android.hardware.tv.mediaquality.SoundParameters soundParameters);
}
