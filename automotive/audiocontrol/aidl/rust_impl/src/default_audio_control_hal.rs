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
use android_hardware_automotive_audiocontrol::aidl::android::hardware::automotive::audiocontrol::{
    AudioFocusChange::AudioFocusChange,
    AudioGainConfigInfo::AudioGainConfigInfo,
    DuckingInfo::DuckingInfo,
    IAudioControl::IAudioControl,
    IAudioGainCallback::IAudioGainCallback,
    IFocusListener::IFocusListener,
    IModuleChangeCallback::IModuleChangeCallback,
    MutingInfo::MutingInfo,
    Reasons::Reasons,
};
use android_hardware_audio_common::aidl::android::hardware::audio::common::PlaybackTrackMetadata::PlaybackTrackMetadata;
use binder::{Interface, Result as BinderResult, StatusCode, Strong};

/// This struct is defined to implement IAudioControl AIDL interface.
pub struct DefaultAudioControlHal;

impl Interface for DefaultAudioControlHal {}

impl IAudioControl for DefaultAudioControlHal {
    fn onAudioFocusChange(&self, _usage : &str, _zone_id : i32, _focus_change : AudioFocusChange
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn onDevicesToDuckChange(&self, _ducking_infos : &[DuckingInfo]) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn onDevicesToMuteChange(&self, _muting_infos : &[MutingInfo]) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn registerFocusListener(&self, _listener : &Strong<dyn IFocusListener>) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setBalanceTowardRight(&self, _value : f32) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setFadeTowardFront(&self, _value : f32) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn onAudioFocusChangeWithMetaData(&self, _playback_metadata : &PlaybackTrackMetadata,
            _zone_id : i32, _focus_change : AudioFocusChange) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setAudioDeviceGainsChanged(&self, _reasons : &[Reasons], _gains : &[AudioGainConfigInfo]
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn registerGainCallback(&self, _callback : &Strong<dyn IAudioGainCallback>
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setModuleChangeCallback(&self, _callback : &Strong<dyn IModuleChangeCallback>
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn clearModuleChangeCallback(&self) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }
}
