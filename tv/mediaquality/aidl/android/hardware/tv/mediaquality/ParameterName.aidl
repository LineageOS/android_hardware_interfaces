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

/**
 * Parameter names.
 * <p>Details of the parameters can be found at
 * android.hardware.tv.mediaquality.PictureParameter and
 * android.hardware.tv.mediaquality.SoundParameter.
 */
@VintfStability
enum ParameterName {
    BRIGHTNESS,
    CONTRAST,
    SHARPNESS,
    SATURATION,
    HUE,
    COLOR_TUNER_BRIGHTNESS,
    COLOR_TUNER_SATURATION,
    COLOR_TUNER_HUE,
    COLOR_TUNER_RED_OFFSET,
    COLOR_TUNER_GREEN_OFFSET,
    COLOR_TUNER_BLUE_OFFSET,
    COLOR_TUNER_RED_GAIN,
    COLOR_TUNER_GREEN_GAIN,
    COLOR_TUNER_BLUE_GAIN,
    NOISE_REDUCTION,
    MPEG_NOISE_REDUCTION,
    FLASH_TONE,
    DE_CONTOUR,
    DYNAMIC_LUMA_CONTROL,
    FILM_MODE,
    BLACK_STRETCH,
    BLUE_STRETCH,
    COLOR_TUNE,
    COLOR_TEMPERATURE,
    GLOBE_DIMMING,
    AUTO_PICTUREQUALITY_ENABLED,
    AUTO_SUPER_RESOLUTION_ENABLED,

    BALANCE,
    BASS,
    TREBLE,
    SURROUND_SOUND_ENABLED,
    EQUALIZER_DETAIL,
    SPEAKERS_ENABLED,
    SPEAKERS_DELAY_MS,
    ENHANCED_AUDIO_RETURN_CHANNEL_ENABLED,
    AUTO_VOLUME_CONTROL,
    DOWNMIX_MODE,
    DTS_DRC,
    DOLBY_AUDIO_PROCESSING,
    DOLBY_DIALOGUE_ENHANCER,
    DTS_VIRTUAL_X,
    DIGITAL_OUTPUT,
    DIGITAL_OUTPUT_DELAY_MS,
}
