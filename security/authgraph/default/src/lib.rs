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

//! Common functionality for non-secure/testing instance of AuthGraph.

use authgraph_boringssl as boring;
use authgraph_core::{
    error, keyexchange,
    ta::{AuthGraphTa, Role},
};
use authgraph_hal::channel::SerializedChannel;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::{mpsc, Mutex};

/// Implementation of the AuthGraph TA that runs locally in-process (and which is therefore
/// insecure).
pub struct LocalTa {
    channels: Mutex<Channels>,
}

struct Channels {
    in_tx: mpsc::Sender<Vec<u8>>,
    out_rx: mpsc::Receiver<Vec<u8>>,
}

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Result<Self, error::Error> {
        // Create a pair of channels to communicate with the TA thread.
        let (in_tx, in_rx) = mpsc::channel();
        let (out_tx, out_rx) = mpsc::channel();

        // The TA code expects to run single threaded, so spawn a thread to run it in.
        std::thread::spawn(move || {
            let mut ta = AuthGraphTa::new(
                keyexchange::AuthGraphParticipant::new(
                    boring::crypto_trait_impls(),
                    Rc::new(RefCell::new(boring::test_device::AgDevice::default())),
                    keyexchange::MAX_OPENED_SESSIONS,
                )
                .expect("failed to create AG participant"),
                Role::Both,
            );
            // Loop forever processing request messages.
            loop {
                let req_data: Vec<u8> = in_rx.recv().expect("failed to receive next req");
                let rsp_data = ta.process(&req_data);
                out_tx.send(rsp_data).expect("failed to send out rsp");
            }
        });
        Ok(Self {
            channels: Mutex::new(Channels { in_tx, out_rx }),
        })
    }
}

impl SerializedChannel for LocalTa {
    const MAX_SIZE: usize = usize::MAX;

    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        // Serialize across both request and response.
        let channels = self.channels.lock().unwrap();
        channels
            .in_tx
            .send(req_data.to_vec())
            .expect("failed to send in request");
        Ok(channels.out_rx.recv().expect("failed to receive response"))
    }
}
