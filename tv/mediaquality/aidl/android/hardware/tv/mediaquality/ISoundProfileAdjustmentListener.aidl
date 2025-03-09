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

package android.hardware.tv.mediaquality;

import android.hardware.tv.mediaquality.ParamCapability;
import android.hardware.tv.mediaquality.SoundProfile;
import android.hardware.tv.mediaquality.VendorParamCapability;

@VintfStability
oneway interface ISoundProfileAdjustmentListener {
    /**
     * Notifies Media Quality Manager when the sound profile changed.
     *
     * @param soundProfile Sound profile.
     */
    void onSoundProfileAdjusted(in SoundProfile soundProfile);

    /**
     * Notifies Media Quality Manager when parameter capabilities changed.
     *
     * @param soundProfileId the ID of the profile used by the media content. -1 if there
     *                         is no associated profile.
     * @param caps the updated capabilities.
     */
    void onParamCapabilityChanged(long soundProfileId, in ParamCapability[] caps);

    /**
     * Notifies Media Quality Manager when vendor parameter capabilities changed.
     *
     * <p>This should be also called when the listener is registered to let the client know
     * what vendor parameters are supported.
     *
     * @param soundProfileId the ID of the profile used by the media content. -1 if there
     *                         is no associated profile.
     * @param caps the updated vendor capabilities.
     */
    void onVendorParamCapabilityChanged(long soundProfileId, in VendorParamCapability[] caps);

    /**
     * Request the sound parameters by sound profile id. Check SoundParameters for its detail.
     * This is called from the HAL to media quality framework.
     *
     * The requested sound parameters will get from IMediaQuality::sendSoundParameters called
     * by the framework.
     *
     * @param SoundProfileId The SoundProfile id that associate with the SoundProfile.
     */
    void onRequestSoundParameters(long SoundProfileId);
}
