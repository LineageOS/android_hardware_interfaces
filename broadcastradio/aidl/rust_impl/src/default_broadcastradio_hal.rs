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
use android_hardware_broadcastradio::aidl::android::hardware::broadcastradio::{
    AmFmRegionConfig::AmFmRegionConfig,
    AnnouncementType::AnnouncementType,
    ConfigFlag::ConfigFlag,
    DabTableEntry::DabTableEntry,
    IAnnouncementListener::IAnnouncementListener,
    IBroadcastRadio::IBroadcastRadio,
    ICloseHandle::ICloseHandle,
    ITunerCallback::ITunerCallback,
    ProgramFilter::ProgramFilter,
    ProgramSelector::ProgramSelector,
    Properties::Properties,
    VendorKeyValue::VendorKeyValue,
};
use binder::{Interface, Result as BinderResult, StatusCode, Strong};
use std::vec::Vec;

/// This struct is defined to implement IBroadcastRadio AIDL interface.
pub struct DefaultBroadcastRadioHal;

impl Interface for DefaultBroadcastRadioHal {}

impl IBroadcastRadio for DefaultBroadcastRadioHal {
    fn getAmFmRegionConfig(&self, _full : bool) -> BinderResult<AmFmRegionConfig> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getDabRegionConfig(&self) -> BinderResult<Vec<DabTableEntry>> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getProperties(&self) -> BinderResult<Properties> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getImage(&self, _id : i32) -> BinderResult<Vec<u8>> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setTunerCallback(&self, _callback : &Strong<dyn ITunerCallback>) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn unsetTunerCallback(&self) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn tune(&self, _program : &ProgramSelector) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn seek(&self, _direction_up : bool, _skip_sub_channel : bool) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn step(&self, _direction_up : bool) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn cancel(&self) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn startProgramListUpdates(&self, _filter : &ProgramFilter) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn stopProgramListUpdates(&self) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn isConfigFlagSet(&self, _flag : ConfigFlag) -> BinderResult<bool> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setConfigFlag(&self, _flag : ConfigFlag, _value : bool) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setParameters(&self, _parameters : &[VendorKeyValue]) -> BinderResult<Vec<VendorKeyValue>> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getParameters(&self, _parameters : &[String]) -> BinderResult<Vec<VendorKeyValue>> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn registerAnnouncementListener(&self, _listener : &Strong<dyn IAnnouncementListener>,
            _enabled : &[AnnouncementType]) -> BinderResult<Strong<dyn ICloseHandle>> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }
}
