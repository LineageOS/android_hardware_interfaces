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

package android.hardware.automotive.audiocontrol;

import android.hardware.automotive.audiocontrol.VolumeActivationConfigurationEntry;

/**
 * Use to configure audio activiations, only used at boot up time.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable VolumeActivationConfiguration {
    /**
     * Configuration name used for debugging purposes to identify config used.
     *
     * <p>Is present, it must be non-empty and unique for all volume acvitations, otherwise
     * car audio service will construct one based on audio zone, configuration and volume group
     * info.
     */
    @nullable String name;

    /**
     * List of activation configurations.
     *
     * <P>Car audio service currently only  uses the first activation config on the list.
     */
    List<VolumeActivationConfigurationEntry> volumeActivationEntries;
}
