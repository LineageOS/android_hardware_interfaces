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

//! Default implementation of the KeyMint HAL and related HALs.
//!
//! This implementation of the HAL is only intended to allow testing and policy compliance.  A real
//! implementation **must implement the TA in a secure environment**, as per CDD 9.11 [C-1-1]:
//! "MUST back up the keystore implementation with an isolated execution environment."
//!
//! The additional device-specific components that are required for a real implementation of KeyMint
//! that is based on the Rust reference implementation are described in system/keymint/README.md.

use kmr_hal::{register_binder_services, HalServiceError, SerializedChannel, ALL_HALS};
use log::{error, info, warn};
use std::ops::DerefMut;
use std::sync::{mpsc, Arc, Mutex};

/// Name of KeyMint binder device instance.
static SERVICE_INSTANCE: &str = "default";

fn main() {
    if let Err(HalServiceError(e)) = inner_main() {
        panic!("HAL service failed: {:?}", e);
    }
}

fn inner_main() -> Result<(), HalServiceError> {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("keymint-hal-nonsecure")
            .with_max_level(log::LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    warn!("Insecure KeyMint HAL service is starting.");

    info!("Starting thread pool now.");
    binder::ProcessState::start_thread_pool();

    // Create a TA in-process, which acts as a local channel for communication.
    let channel = Arc::new(Mutex::new(LocalTa::new()));

    kmr_hal_nonsecure::send_boot_info_and_attestation_id_info(&channel)?;

    // Let the TA know information about the userspace environment.
    if let Err(e) = kmr_hal::send_hal_info(channel.lock().unwrap().deref_mut()) {
        error!("Failed to send HAL info: {:?}", e);
    }

    register_binder_services(&channel, ALL_HALS, SERVICE_INSTANCE)?;

    binder::ProcessState::join_thread_pool();
    info!("KeyMint HAL service is terminating."); // should not reach here
    Ok(())
}

/// Implementation of the KeyMint TA that runs locally in-process (and which is therefore
/// insecure).
#[derive(Debug)]
pub struct LocalTa {
    in_tx: mpsc::Sender<Vec<u8>>,
    out_rx: mpsc::Receiver<Vec<u8>>,
}

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Self {
        // Create a pair of channels to communicate with the TA thread.
        let (in_tx, in_rx) = mpsc::channel();
        let (out_tx, out_rx) = mpsc::channel();

        // The TA code expects to run single threaded, so spawn a thread to run it in.
        std::thread::spawn(move || {
            let mut ta = kmr_ta_nonsecure::build_ta();
            loop {
                let req_data: Vec<u8> = in_rx.recv().expect("failed to receive next req");
                let rsp_data = ta.process(&req_data);
                out_tx.send(rsp_data).expect("failed to send out rsp");
            }
        });
        Self { in_tx, out_rx }
    }
}

impl SerializedChannel for LocalTa {
    const MAX_SIZE: usize = usize::MAX;

    fn execute(&mut self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        self.in_tx
            .send(req_data.to_vec())
            .expect("failed to send in request");
        Ok(self.out_rx.recv().expect("failed to receive response"))
    }
}
