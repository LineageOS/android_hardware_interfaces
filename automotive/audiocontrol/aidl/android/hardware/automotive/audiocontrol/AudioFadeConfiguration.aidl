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

import android.hardware.automotive.audiocontrol.FadeConfiguration;
import android.hardware.automotive.audiocontrol.FadeState;
import android.media.audio.common.AudioAttributes;
import android.media.audio.common.AudioContentType;
import android.media.audio.common.AudioUsage;

/**
 * Encapsulates the audio fade configuration info
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioFadeConfiguration {
    /**
     * Default fade in duration
     */
    const long DEFAULT_FADE_IN_DURATION_MS = 1000;

    /**
     * Default fade out duration
     */
    const long DEFAULT_FADE_OUT_DURATION_MS = 2000;

    /**
     * Default delay for fade in offenders
     */
    const long DEFAULT_DELAY_FADE_IN_OFFENDERS_MS = 2000;

    /**
     * Audio configuration name, use for debugging purposes
     */
    String name;

    /**
     * Audio configuration state
     */
    FadeState fadeState;

    /**
     * Fade in duration in milliseconds
     *
     * <p>Use to construct the default fade in configuration. This can be overwritten for different
     * attributes/usages by passing a list of fade-in configuration,
     * see {@code #fadeInConfigurations}
     */
    long fadeInDurationMs = DEFAULT_FADE_IN_DURATION_MS;

    /**
     * Fade out duration in milliseconds
     *
     * <p>Use to construct the default fade out configuration. This can be overwritten for different
     * attributes/usages by passing a list of fade-out configuration,
     * see {@code #fadeOutConfigurations}
     */
    long fadeOutDurationMs = DEFAULT_FADE_OUT_DURATION_MS;

    /**
     * Fade in delayed duration for audio focus offender in milliseconds
     *
     * <p>Fade offender are defined as audio players that do not stop playback after audio focus
     * lost. This timeout serves to continue to fadeout the offender until audio is stopped or the
     * timeout expires.
     */
    long fadeInDelayedForOffendersMs = DEFAULT_DELAY_FADE_IN_OFFENDERS_MS;

    /**
     * List of audio attribute usage that should be faded using the parameters in
     * this configuration.
     *
     *<p>If the list is empty car audio service will overwrite the list for the confgiruation with
     * default usages, e.g. {AudioUsage#MEDIA, AudioUsage#GAME}
     */
    AudioUsage[] fadeableUsages;

    /**
     * Optional list of audio attribute content types that should not be faded.
     *
     **<p>The list can be empty in cases where there are no unfadeable content types.
     *
     *<p>If the list is not set car audio service will overwrite the list for the confgiruation
     * with default content type, e.g. {AudioContentType#SPEECH}.
     */
    @nullable AudioContentType[] unfadeableContentTypes;

    /**
     * List of audio attribute that should not be faded.
     *
     *<p>The list can be empty in cases where there are no unfadeable attributes
     */
    List<AudioAttributes> unfadableAudioAttributes;

    /**
     * List of fade out configutions which should apply to this audio fade configurations
     *
     *<p>The list can be empty in cases where there are no fade out configurations.
     */
    List<FadeConfiguration> fadeOutConfigurations;

    /**
     * List of fade in configutions which should apply to this audio fade configurations
     *
     *<p>The list can be empty in cases where there are no fade out configurations
     */
    List<FadeConfiguration> fadeInConfigurations;
}
