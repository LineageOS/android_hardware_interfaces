/*
 * Copyright (C) 2023 The Android Open Source Project
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
//! This module implements the ILights AIDL interface.

use log::info;

use android_hardware_light::aidl::android::hardware::light::{
    HwLight::HwLight, HwLightState::HwLightState, ILights::ILights, LightType::LightType,
};

use binder::{ExceptionCode, Interface, Status};

/// Defined so we can implement the ILights AIDL interface.
pub struct LightsService;

impl Interface for LightsService {}

const NUM_DEFAULT_LIGHTS: i32 = 3;

impl ILights for LightsService {
    fn setLightState(&self, id: i32, state: &HwLightState) -> binder::Result<()> {
        info!("Lights setting state for id={} to color {:x}", id, state.color);
        if id <= 0 || id > NUM_DEFAULT_LIGHTS {
            return Err(Status::new_exception(ExceptionCode::UNSUPPORTED_OPERATION, None));
        }

        Ok(())
    }

    fn getLights(&self) -> binder::Result<Vec<HwLight>> {
        let mut lights: Vec<HwLight> = Vec::with_capacity(NUM_DEFAULT_LIGHTS.try_into().unwrap());

        for i in 1..=NUM_DEFAULT_LIGHTS {
            let light = HwLight { id: i, ordinal: i, r#type: LightType::BACKLIGHT };

            lights.push(light);
        }

        info!("Lights reporting supported lights");
        Ok(lights)
    }
}
