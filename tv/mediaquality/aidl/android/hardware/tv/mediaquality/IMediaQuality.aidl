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

import android.hardware.tv.mediaquality.AmbientBacklightSettings;
import android.hardware.tv.mediaquality.IMediaQualityCallback;
import android.hardware.tv.mediaquality.IPictureProfileAdjustmentListener;
import android.hardware.tv.mediaquality.IPictureProfileChangedListener;
import android.hardware.tv.mediaquality.ISoundProfileAdjustmentListener;
import android.hardware.tv.mediaquality.ISoundProfileChangedListener;
import android.hardware.tv.mediaquality.ParamCapability;
import android.hardware.tv.mediaquality.ParameterName;
import android.hardware.tv.mediaquality.PictureParameters;
import android.hardware.tv.mediaquality.SoundParameters;
import android.hardware.tv.mediaquality.VendorParamCapability;
import android.hardware.tv.mediaquality.VendorParameterIdentifier;

/**
 * Interface for the media quality service
 */
@VintfStability
interface IMediaQuality {
    /**
     * Sets a callback for events.
     *
     * @param callback Callback object to pass events.
     */
    void setCallback(in IMediaQualityCallback callback);

    /**
     * Sets the ambient backlight detector settings.
     *
     * @param settings Ambient backlight detector settings.
     */
    void setAmbientBacklightDetector(in AmbientBacklightSettings settings);

    /**
     * Sets the ambient backlight detection enabled or disabled. The ambient backlight is the
     * projection of light against the wall driven by the current content playing. Enable will
     * detects the Ambient backlight metadata and ambient control app can control the related
     * device as configured before.
     *
     * @param enabled True to enable the ambient backlight detection, false to disable.
     */
    void setAmbientBacklightDetectionEnabled(in boolean enabled);

    /**
     * Gets the ambient backlight detection enabled status. The ambient backlight is enabled by
     * calling setAmbientBacklightDetectionEnabled(in boolean enabled). True to enable the
     * ambient light detection and False to disable the ambient backlight detection.
     *
     * @return True if the ambient backlight detection is enabled, false otherwise.
     */
    boolean getAmbientBacklightDetectionEnabled();

    /**
     * Check if auto picture quality feature is supported on the current TV device.
     *
     * @return true when the device supports the auto picture quality, false when the device does
     * not supports the auto picture quality.
     */
    boolean isAutoPqSupported();

    /**
     * Get the current state of auto picture quality.
     *
     * @return true when auto picture quality is enabled, false when auto picture quality is
     * disabled.
     */
    boolean getAutoPqEnabled();

    /**
     * Set the auto picture quality enable/disable. Auto picture quality is to adjust the Picture
     * parameters depends on the current content playing.
     *
     * @param enable True to enable, false to disable.
     */
    void setAutoPqEnabled(boolean enable);

    /**
     * Check if auto super resolution feature is supported on the current TV device.
     *
     * @return true when the device supports the super resolution feature, false when the device
     * does not support super resolution.
     */
    boolean isAutoSrSupported();

    /**
     * Get the current state of auto super resolution.
     *
     * @return true when auto super resolution is enabled, false when auto super resolution is
     * disabled.
     */
    boolean getAutoSrEnabled();

    /**
     * Set the auto super resolution enable/disable. Auto super resolution is to analyze the
     * lower resolution image and invent the missing pixel to make the image looks sharper.
     *
     * @param enable True to enable, false to disable.
     */
    void setAutoSrEnabled(boolean enable);

    /**
     * Check if auto sound/audio quality feature is supported on the current TV device.
     *
     * @return true when the device supports the auto sound/audio quality, false when
     * the device does not supports the auto sound/audio quality.
     */
    boolean isAutoAqSupported();

    /**
     * Get the current state of auto sound/audio quality.
     *
     * @return true when auto sound/audio quality is enabled, false when auto sound/audio
     * quality is disabled.
     */
    boolean getAutoAqEnabled();

    /**
     * Set the auto sound/audio quality enable/disable. Auto sound/audio quality is to
     * adjust the sound parameters depends on the current content playing.
     *
     * @param enable True to enable, false to disable.
     */
    void setAutoAqEnabled(boolean enable);

    /**
     * Get picture profile changed listener.
     *
     * @return the IPictureProfileChangedListener.
     */
    IPictureProfileChangedListener getPictureProfileListener();

    /**
     * Sets the listener for picture adjustment from the HAL.
     *
     * When the same client registers this listener multiple times, only the most recent
     * registration will be active. The previous listener will be overwritten.
     *
     * When different client registers this listener, it will overwrite the previous registered
     * client. Only one listener can be active.
     *
     * @param IPictureProfileAdjustmentListener listener object to pass picture profile, profile
     *        id and hardware capability.
     */
    void setPictureProfileAdjustmentListener(IPictureProfileAdjustmentListener listener);

    /**
     * Send the default picture parameters to the vendor code or HAL to apply the picture
     * parameters.
     *
     * @param pictureParameters PictureParameters with pre-defined parameters and vendor defined
     * parameters.
     */
    void sendDefaultPictureParameters(in PictureParameters pictureParameters);

    /**
     * Get sound profile changed listener.
     *
     * @return the ISoundProfileChangedListener.
     */
    ISoundProfileChangedListener getSoundProfileListener();

    /**
     * Sets the listener for sound adjustment from the HAL.
     *
     * When the same client registers this listener multiple times, only the most recent
     * registration will be active. The previous listener will be overwritten.
     *
     * When different client registers this listener, it will overwrite the previous registered
     * client. Only one listener can be active.
     *
     * @param ISoundProfileAdjustmentListener listener object to pass sound profile, profile id
     *        and hardware capability.
     */
    void setSoundProfileAdjustmentListener(ISoundProfileAdjustmentListener listener);

    /**
     * Send the default sound parameters to the vendor code or HAL to apply the sound parameters.
     *
     * @param soundParameters SoundParameters with pre-defined parameters and vendor defined
     * parameters.
     */
    void sendDefaultSoundParameters(in SoundParameters soundParameters);

    /**
     * Gets capability information of the given parameters.
     */
    void getParamCaps(in ParameterName[] paramNames, out ParamCapability[] caps);

    /**
     * Gets vendor capability information of the given parameters.
     */
    void getVendorParamCaps(in VendorParameterIdentifier[] names, out VendorParamCapability[] caps);

    /**
     * When HAL request picture parameters by picture profile id, the framework will use this to
     * send the picture parameters associate with the profile id.
     *
     * @param pictureParameters pictureParameters that associate with the profile id HAL provided.
     */
    void sendPictureParameters(in PictureParameters pictureParameters);

    /**
     * When HAL request sound parameters by sound profile id, the framework will use this to
     * send the sound parameters associate with the profile id.
     *
     * @param soundParameters soundParameters that associate with the profile id HAL provided.
     */
    void sendSoundParameters(in SoundParameters soundParameters);
}
