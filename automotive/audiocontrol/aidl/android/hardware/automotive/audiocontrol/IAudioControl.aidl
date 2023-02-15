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
 * Important note on Metadata:
 * Metadata qualifies a playback track for an output stream.
 * This is highly closed to {@link android.media.AudioAttributes}.
 * It allows to identify the audio stream rendered / requesting / abandonning the focus.
 *
 * AudioControl 1.0 was limited to identification through {@code AttributeUsage} listed as
 * {@code audioUsage} in audio_policy_configuration.xsd.
 *
 * Any new OEM needs would not be possible without extension.
 *
 * Relying on {@link android.hardware.automotive.audiocontrol.PlaybackTrackMetadata} allows
 * to use a combination of {@code AttributeUsage}, {@code AttributeContentType} and
 * {@code AttributeTags} to identify the use case / routing thanks to
 * {@link android.media.audiopolicy.AudioProductStrategy}.
 * The belonging to a strategy is deduced by an AOSP logic (in sync at native and java layer).
 *
 * IMPORTANT NOTE ON TAGS:
 * To limit the possibilies and prevent from confusion, we expect the String to follow
 * a given formalism that will be enforced.
 *
 * 1 / By convention, tags shall be a "key=value" pair.
 * Vendor must namespace their tag's key (for example com.google.strategy=VR) to avoid conflicts.
 * vendor specific applications and must be prefixed by "VX_". Vendor must
 *
 * 2 / Tags reported here shall be the same as the tags used to define a given
 * {@link android.media.audiopolicy.AudioProductStrategy} and so in
 * audio_policy_engine_configuration.xml file.
 */
import android.hardware.audio.common.PlaybackTrackMetadata;
import android.hardware.automotive.audiocontrol.AudioFocusChange;
import android.hardware.automotive.audiocontrol.AudioGainConfigInfo;
import android.hardware.automotive.audiocontrol.DuckingInfo;
import android.hardware.automotive.audiocontrol.IAudioGainCallback;
import android.hardware.automotive.audiocontrol.IFocusListener;
import android.hardware.automotive.audiocontrol.IModuleChangeCallback;
import android.hardware.automotive.audiocontrol.MutingInfo;
import android.hardware.automotive.audiocontrol.Reasons;

/**
 * Interacts with the car's audio subsystem to manage audio sources and volumes
 */
@VintfStability
interface IAudioControl {
    /**
     * Notifies HAL of changes in audio focus status for focuses requested or abandoned by the HAL.
     *
     * This will be called in response to IFocusListener's requestAudioFocus and
     * abandonAudioFocus, as well as part of any change in focus being held by the HAL due focus
     * request from other activities or services.
     *
     * The HAL is not required to wait for an callback of AUDIOFOCUS_GAIN before playing audio, nor
     * is it required to stop playing audio in the event of a AUDIOFOCUS_LOSS callback is received.
     *
     * This method was deprecated in version 2 to allow getting rid of usages limitation.
     * Use {@link IAudioControl#onAudioFocusChangeWithMetaData} instead.
     *
     * @param usage The audio usage associated with the focus change {@code AttributeUsage}. See
     * {@code audioUsage} in audio_policy_configuration.xsd for the list of allowed values.
     * @deprecated use {@link android.hardware.audio.common.PlaybackTrackMetadata} instead.
     * @param zoneId The identifier for the audio zone that the HAL is playing the stream in
     * @param focusChange the AudioFocusChange that has occurred.
     */
    oneway void onAudioFocusChange(in String usage, in int zoneId, in AudioFocusChange focusChange);

    /**
     * Notifies HAL of changes in output devices that the HAL should apply ducking to.
     *
     * This will be called in response to changes in audio focus, and will include a
     * {@link DuckingInfo} object per audio zone that experienced a change in audo focus.
     *
     * @param duckingInfos an array of {@link DuckingInfo} objects for the audio zones where audio
     * focus has changed.
     */
    oneway void onDevicesToDuckChange(in DuckingInfo[] duckingInfos);

    /**
     * Notifies HAL of changes in output devices that the HAL should apply muting to.
     *
     * This will be called in response to changes in audio mute state for each volume group
     * and will include a {@link MutingInfo} object per audio zone that experienced a mute state
     * event.
     *
     * @param mutingInfos an array of {@link MutingInfo} objects for the audio zones where audio
     * mute state has changed.
     */
    oneway void onDevicesToMuteChange(in MutingInfo[] mutingInfos);

    /**
     * Registers focus listener to be used by HAL for requesting and abandoning audio focus.
     *
     * It is expected that there will only ever be a single focus listener registered. If the
     * observer dies, the HAL implementation must unregister observer automatically. If called when
     * a listener is already registered, the existing one should be unregistered and replaced with
     * the new listener.
     *
     * @param listener the listener interface.
     */
    oneway void registerFocusListener(in IFocusListener listener);

    /**
     * Control the right/left balance setting of the car speakers.
     *
     * This is intended to shift the speaker volume toward the right (+) or left (-) side of
     * the car. 0.0 means "centered". +1.0 means fully right. -1.0 means fully left.
     *
     * A value outside the range -1 to 1 must be clamped by the implementation to the -1 to 1
     * range.
     */
    oneway void setBalanceTowardRight(in float value);

    /**
     * Control the fore/aft fade setting of the car speakers.
     *
     * This is intended to shift the speaker volume toward the front (+) or back (-) of the car.
     * 0.0 means "centered". +1.0 means fully forward. -1.0 means fully rearward.
     *
     * A value outside the range -1 to 1 must be clamped by the implementation to the -1 to 1
     * range.
     */
    oneway void setFadeTowardFront(in float value);

    /**
     * Notifies HAL of changes in audio focus status for focuses requested or abandoned by the HAL.
     *
     * This will be called in response to IFocusListener's requestAudioFocus and
     * abandonAudioFocus, as well as part of any change in focus being held by the HAL due focus
     * request from other activities or services.
     *
     * The HAL is not required to wait for an callback of AUDIOFOCUS_GAIN before playing audio, nor
     * is it required to stop playing audio in the event of a AUDIOFOCUS_LOSS callback is received.
     *
     * @param playbackMetaData The output stream metadata associated with the focus request
     * @param zoneId The identifier for the audio zone that the HAL is playing the stream in
     * @param focusChange the AudioFocusChange that has occurred.
     */
    oneway void onAudioFocusChangeWithMetaData(in PlaybackTrackMetadata playbackMetaData,
            in int zoneId, in AudioFocusChange focusChange);

    /**
     * Notifies HAL of changes in output devices that the HAL should apply gain change to
     * and the reason(s) why
     *
     * This may be called in response to changes in audio focus, and will include a list of
     * {@link android.hardware.automotive.audiocontrol.AudioGainConfigInfo} objects per audio zone
     * that experienced a change in audo focus.
     *
     * @param reasons List of reasons that triggered the given gains changed.
     *                This must be one or more of the
     *                {@link android.hardware.automotive.audiocontrol.Reasons} constants.
     *
     * @param gains List of gains the change is intended to.
     */
    oneway void setAudioDeviceGainsChanged(in Reasons[] reasons, in AudioGainConfigInfo[] gains);

    /**
     * Registers callback to be used by HAL for reporting unexpected gain(s) changed and the
     * reason(s) why.
     *
     * It is expected that there will only ever be a single callback registered. If the
     * observer dies, the HAL implementation must unregister observer automatically. If called when
     * a listener is already registered, the existing one should be unregistered and replaced with
     * the new callback.
     *
     * @param callback The {@link android.hardware.automotive.audiocontrol.IAudioGainCallback}
     *                 interface.
     */
    oneway void registerGainCallback(in IAudioGainCallback callback);

    /**
     * Sets callback with HAL for notifying changes to hardware module (that is:
     * {@link android.hardware.audio.core.IModule}) configurations.
     *
     * @param callback The {@link android.hardware.automotive.audiocontrol.IModuleChangeCallback}
     *                 interface to use use when new updates are available for
     *                 {@link android.hardware.audio.core.IModule} configs
     * @throws EX_UNSUPPORTED_OPERATION if dynamic audio configs are not supported.
     * @throws EX_ILLEGAL_STATE if a callback already exists
     * @throws EX_ILLEGAL_ARGUMENT if the passed callback is (@code null}
     */
    void setModuleChangeCallback(in IModuleChangeCallback callback);

    /**
     * Clears module change callback
     *
     * If no callback is registered previously, then this call should be a no-op.
     *
     * @throws EX_UNSUPPORTED_OPERATION if dynamic audio configs are not supported.
     */
    void clearModuleChangeCallback();
}
