/*
 * Copyright (C) 2020 The Android Open Source Project
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

 package android.hardware.automotive.audiocontrol;

 /**
  * The current ducking information for a single audio zone.
  *
  * <p>This includes devices to duck, as well as unduck based on the contents of a previous
  * {@link DuckingInfo}. Additionally, the current usages holding focus in the specified zone are
  * included, which were used to determine which addresses to duck.
  */
 @VintfStability
 parcelable DuckingInfo {
     /**
      * ID of the associated audio zone
      */
     int zoneId;

     /**
      * List of addresses for audio output devices that should be ducked.
      *
      * <p>The provided address strings are defined in audio_policy_configuration.xml.
      */
     String[] deviceAddressesToDuck;

     /**
      * List of addresses for audio output devices that were previously be ducked and should now be
      * unducked.
      *
      * <p>The provided address strings are defined in audio_policy_configuration.xml.
      */
     String[] deviceAddressesToUnduck;

     /**
      * List of usages currently holding focus for this audio zone.
      *
      * <p> See {@code audioUsage} in audio_policy_configuration.xsd for the list of allowed values.
      */
     String[] usagesHoldingFocus;
 }