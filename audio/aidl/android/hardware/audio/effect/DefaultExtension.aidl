/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.audio.effect;

/**
 * The default extension used for Parameter.Specific.vendorEffect ParcelableHolder.
 *
 * The audio framework attach this default extension to the ParcelableHolder in VendorExtension,
 * and pass though all parameters it received from the client to audio HAL.
 *
 * For now it's not possible for vendor to define their own vendor extensions without changing the
 * audio framework. More specificly, in order to add a customized effect parameter AIDL parcelable,
 * vendors need to add the logic for conversion between AIDL and effect_param_t for the effect AIDL
 * in: frameworks/av/media/libaudiohal/impl/effectAidlConversion.
 *
 * There is no VTS test cases for the vendor extension effect implementation, however all effect
 * implementations must support the common parameters defined in Parameter.aidl, so vendor
 * extension effect implementation still need to support setting and getting of these common
 * parameters, which is enforced by VTS.
 */
@VintfStability
parcelable DefaultExtension {
    byte[] bytes;
}
