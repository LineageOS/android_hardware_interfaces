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
//! This module implements the IMediaQuality AIDL interface.

use android_hardware_tv_mediaquality::aidl::android::hardware::tv::mediaquality::{
    IMediaQuality::IMediaQuality,
    IMediaQualityCallback::IMediaQualityCallback,
    AmbientBacklightEvent::AmbientBacklightEvent,
    AmbientBacklightSettings::AmbientBacklightSettings,
    IPictureProfileAdjustmentListener::IPictureProfileAdjustmentListener,
    IPictureProfileChangedListener::IPictureProfileChangedListener,
    ParamCapability::ParamCapability,
    ParameterName::ParameterName,
    PictureParameters::PictureParameters,
    ISoundProfileAdjustmentListener::ISoundProfileAdjustmentListener,
    ISoundProfileChangedListener::ISoundProfileChangedListener,
    SoundParameters::SoundParameters,
    VendorParamCapability::VendorParamCapability,
    VendorParameterIdentifier::VendorParameterIdentifier,
};
use binder::{Interface, Strong};
use std::sync::{Arc, Mutex};
use std::thread;

/// Defined so we can implement the IMediaQuality AIDL interface.
pub struct MediaQualityService {
    callback: Arc<Mutex<Option<Strong<dyn IMediaQualityCallback>>>>,
    ambient_backlight_enabled: Arc<Mutex<bool>>,
    ambient_backlight_detector_settings: Arc<Mutex<AmbientBacklightSettings>>,
    auto_pq_supported: Arc<Mutex<bool>>,
    auto_pq_enabled: Arc<Mutex<bool>>,
    auto_sr_supported: Arc<Mutex<bool>>,
    auto_sr_enabled: Arc<Mutex<bool>>,
    auto_aq_supported: Arc<Mutex<bool>>,
    auto_aq_enabled: Arc<Mutex<bool>>,
    picture_profile_adjustment_listener:
            Arc<Mutex<Option<Strong<dyn IPictureProfileAdjustmentListener>>>>,
    sound_profile_adjustment_listener:
            Arc<Mutex<Option<Strong<dyn ISoundProfileAdjustmentListener>>>>,
    picture_profile_changed_listener: Arc<Mutex<Option<Strong<dyn IPictureProfileChangedListener>>>>,
    sound_profile_changed_listener: Arc<Mutex<Option<Strong<dyn ISoundProfileChangedListener>>>>,
}

impl MediaQualityService {

    /// Create a new instance of the MediaQualityService.
    pub fn new() -> Self {
        Self {
            callback: Arc::new(Mutex::new(None)),
            ambient_backlight_enabled: Arc::new(Mutex::new(true)),
            ambient_backlight_detector_settings:
                    Arc::new(Mutex::new(AmbientBacklightSettings::default())),
            auto_pq_supported: Arc::new(Mutex::new(false)),
            auto_pq_enabled: Arc::new(Mutex::new(false)),
            auto_sr_supported: Arc::new(Mutex::new(false)),
            auto_sr_enabled: Arc::new(Mutex::new(false)),
            auto_aq_supported: Arc::new(Mutex::new(false)),
            auto_aq_enabled: Arc::new(Mutex::new(false)),
            picture_profile_adjustment_listener: Arc::new(Mutex::new(None)),
            sound_profile_adjustment_listener: Arc::new(Mutex::new(None)),
            picture_profile_changed_listener: Arc::new(Mutex::new(None)),
            sound_profile_changed_listener: Arc::new(Mutex::new(None)),
        }
    }
}

impl Interface for MediaQualityService {}

impl IMediaQuality for MediaQualityService {

    fn setCallback(
        &self,
        callback: &Strong<dyn IMediaQualityCallback>
    ) -> binder::Result<()> {
        println!("Received callback: {:?}", callback);
        let mut cb = self.callback.lock().unwrap();
        *cb = Some(callback.clone());
        Ok(())
    }

    fn setAmbientBacklightDetector(
        &self,
        settings: &AmbientBacklightSettings
    ) -> binder::Result<()> {
        println!("Received settings: {:?}", settings);
        let mut ambient_backlight_detector_settings = self.ambient_backlight_detector_settings.lock().unwrap();
        ambient_backlight_detector_settings.packageName = settings.packageName.clone();
        ambient_backlight_detector_settings.source = settings.source;
        ambient_backlight_detector_settings.maxFramerate = settings.maxFramerate;
        ambient_backlight_detector_settings.colorFormat = settings.colorFormat;
        ambient_backlight_detector_settings.hZonesNumber = settings.hZonesNumber;
        ambient_backlight_detector_settings.vZonesNumber = settings.vZonesNumber;
        ambient_backlight_detector_settings.hasLetterbox = settings.hasLetterbox;
        ambient_backlight_detector_settings.threshold = settings.threshold;
        Ok(())
    }

    fn setAmbientBacklightDetectionEnabled(&self, enabled: bool) -> binder::Result<()> {
        println!("Received enabled: {}", enabled);
        let mut ambient_backlight_enabled = self.ambient_backlight_enabled.lock().unwrap();
        *ambient_backlight_enabled = enabled;
        if enabled {
            println!("Enable Ambient Backlight detection");
            thread::scope(|s| {
                s.spawn(|| {
                    let cb = self.callback.lock().unwrap();
                    if let Some(cb) = &*cb {
                        let enabled_event = AmbientBacklightEvent::Enabled(true);
                        cb.notifyAmbientBacklightEvent(&enabled_event).unwrap();
                    }
                });
            });
        } else {
            println!("Disable Ambient Backlight detection");
            thread::scope(|s| {
                s.spawn(|| {
                    let cb = self.callback.lock().unwrap();
                    if let Some(cb) = &*cb {
                        let disabled_event = AmbientBacklightEvent::Enabled(false);
                        cb.notifyAmbientBacklightEvent(&disabled_event).unwrap();
                    }
                });
            });
        }
        Ok(())
    }

    fn getAmbientBacklightDetectionEnabled(&self) -> binder::Result<bool> {
        let ambient_backlight_enabled = self.ambient_backlight_enabled.lock().unwrap();
        Ok(*ambient_backlight_enabled)
    }

    fn isAutoPqSupported(&self) -> binder::Result<bool> {
        let auto_pq_supported = self.auto_pq_supported.lock().unwrap();
        Ok(*auto_pq_supported)
    }

    fn getAutoPqEnabled(&self) -> binder::Result<bool> {
        let auto_pq_enabled = self.auto_pq_enabled.lock().unwrap();
        Ok(*auto_pq_enabled)
    }

    fn setAutoPqEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_pq_enabled = self.auto_pq_enabled.lock().unwrap();
        *auto_pq_enabled = enabled;
        if enabled {
            println!("Enable auto picture quality");
        } else {
            println!("Disable auto picture quality");
        }
        Ok(())
    }

    fn isAutoSrSupported(&self) -> binder::Result<bool> {
        let auto_sr_supported = self.auto_sr_supported.lock().unwrap();
        Ok(*auto_sr_supported)
    }

    fn getAutoSrEnabled(&self) -> binder::Result<bool> {
        let auto_sr_enabled = self.auto_sr_enabled.lock().unwrap();
        Ok(*auto_sr_enabled)
    }

    fn setAutoSrEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_sr_enabled = self.auto_sr_enabled.lock().unwrap();
        *auto_sr_enabled = enabled;
        if enabled {
            println!("Enable auto super resolution");
        } else {
            println!("Disable auto super resolution");
        }
        Ok(())
    }

    fn isAutoAqSupported(&self) -> binder::Result<bool> {
        let auto_aq_supported = self.auto_aq_supported.lock().unwrap();
        Ok(*auto_aq_supported)
    }

    fn getAutoAqEnabled(&self) -> binder::Result<bool> {
        let auto_aq_enabled = self.auto_aq_enabled.lock().unwrap();
        Ok(*auto_aq_enabled)
    }

    fn setAutoAqEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_aq_enabled = self.auto_aq_enabled.lock().unwrap();
        *auto_aq_enabled = enabled;
        if enabled {
            println!("Enable auto audio quality");
        } else {
            println!("Disable auto audio quality");
        }
        Ok(())
    }

    fn getPictureProfileListener(&self) -> binder::Result<binder::Strong<dyn IPictureProfileChangedListener>> {
        println!("getPictureProfileListener");
        let listener = self.picture_profile_changed_listener.lock().unwrap();
        listener.clone().ok_or(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn setPictureProfileAdjustmentListener(
        &self,
        picture_profile_adjustment_listener: &Strong<dyn IPictureProfileAdjustmentListener>
    ) -> binder::Result<()> {
        println!("Received picture profile adjustment");
        let mut listener = self.picture_profile_adjustment_listener.lock().unwrap();
        *listener = Some(picture_profile_adjustment_listener.clone());
        Ok(())
    }

    fn sendDefaultPictureParameters(&self, _picture_parameters: &PictureParameters) -> binder::Result<()>{
        println!("Received picture parameters");
        Ok(())
    }

    fn getSoundProfileListener(&self) -> binder::Result<binder::Strong<dyn ISoundProfileChangedListener>> {
        println!("getSoundProfileListener");
        let listener = self.sound_profile_changed_listener.lock().unwrap();
        listener.clone().ok_or(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn setSoundProfileAdjustmentListener(
        &self,
        sound_profile_adjustment_listener: &Strong<dyn ISoundProfileAdjustmentListener>
    ) -> binder::Result<()> {
        println!("Received sound profile adjustment");
        let mut listener = self.sound_profile_adjustment_listener.lock().unwrap();
        *listener = Some(sound_profile_adjustment_listener.clone());
        Ok(())
    }

    fn sendDefaultSoundParameters(&self, _sound_parameters: &SoundParameters) -> binder::Result<()>{
        println!("Received sound parameters");
        Ok(())
    }

    fn getParamCaps(
            &self,
            param_names: &[ParameterName],
            _caps: &mut Vec<ParamCapability>
    ) -> binder::Result<()> {
        println!("getParamCaps. len= {}", param_names.len());
        Ok(())
    }

    fn getVendorParamCaps(
            &self,
            param_names: &[VendorParameterIdentifier],
            _caps: &mut Vec<VendorParamCapability>
    ) -> binder::Result<()> {
        println!("getVendorParamCaps. len= {}", param_names.len());
        Ok(())
    }

    fn sendPictureParameters(&self, _picture_parameters: &PictureParameters) -> binder::Result<()>{
        println!("Received picture parameters");
        Ok(())
    }

    fn sendSoundParameters(&self, _sound_parameters: &SoundParameters) -> binder::Result<()>{
        println!("Received sound parameters");
        Ok(())
    }
}
