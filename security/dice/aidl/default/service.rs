// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Main entry point for the android.hardware.security.dice service.

use anyhow::{anyhow, Result};
use diced::{
    dice,
    hal_node::{DiceArtifacts, DiceDevice, ResidentHal, UpdatableDiceArtifacts},
};
use diced_sample_inputs::make_sample_bcc_and_cdis;
use serde::{Deserialize, Serialize};
use std::panic;
use std::sync::Arc;

static DICE_HAL_SERVICE_NAME: &str = "android.hardware.security.dice.IDiceDevice/default";

#[derive(Debug, Serialize, Deserialize, Clone)]
struct InsecureSerializableArtifacts {
    cdi_attest: [u8; dice::CDI_SIZE],
    cdi_seal: [u8; dice::CDI_SIZE],
    bcc: Vec<u8>,
}

impl DiceArtifacts for InsecureSerializableArtifacts {
    fn cdi_attest(&self) -> &[u8; dice::CDI_SIZE] {
        &self.cdi_attest
    }
    fn cdi_seal(&self) -> &[u8; dice::CDI_SIZE] {
        &self.cdi_seal
    }
    fn bcc(&self) -> Option<&[u8]> {
        Some(&self.bcc)
    }
}

impl UpdatableDiceArtifacts for InsecureSerializableArtifacts {
    fn with_artifacts<F, T>(&self, f: F) -> Result<T>
    where
        F: FnOnce(&dyn DiceArtifacts) -> Result<T>,
    {
        f(self)
    }
    fn update(self, new_artifacts: &impl DiceArtifacts) -> Result<Self> {
        Ok(Self {
            cdi_attest: *new_artifacts.cdi_attest(),
            cdi_seal: *new_artifacts.cdi_seal(),
            bcc: new_artifacts
                .bcc()
                .ok_or_else(|| anyhow!("bcc is none"))?
                .to_vec(),
        })
    }
}

fn main() {
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("android.hardware.security.dice")
            .with_min_level(log::Level::Debug),
    );
    // Redirect panic messages to logcat.
    panic::set_hook(Box::new(|panic_info| {
        log::error!("{}", panic_info);
    }));

    // Saying hi.
    log::info!("android.hardware.security.dice is starting.");

    let dice_artifacts =
        make_sample_bcc_and_cdis().expect("Failed to construct sample dice chain.");
    let mut cdi_attest = [0u8; dice::CDI_SIZE];
    cdi_attest.copy_from_slice(dice_artifacts.cdi_attest());
    let mut cdi_seal = [0u8; dice::CDI_SIZE];
    cdi_seal.copy_from_slice(dice_artifacts.cdi_seal());
    let hal_impl = Arc::new(
        unsafe {
            // Safety: ResidentHal cannot be used in multi threaded processes.
            // This service does not start a thread pool. The main thread is the only thread
            // joining the thread pool, thereby keeping the process single threaded.
            ResidentHal::new(InsecureSerializableArtifacts {
                cdi_attest,
                cdi_seal,
                bcc: dice_artifacts.bcc().expect("bcc is none").to_vec(),
            })
        }
        .expect("Failed to create ResidentHal implementation."),
    );

    let hal = DiceDevice::new_as_binder(hal_impl).expect("Failed to construct hal service.");

    binder::add_service(DICE_HAL_SERVICE_NAME, hal.as_binder())
        .expect("Failed to register IDiceDevice Service");

    log::info!("Joining thread pool now.");
    binder::ProcessState::join_thread_pool();
}
