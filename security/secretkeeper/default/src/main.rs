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

//! Non-secure implementation of the Secretkeeper HAL.
mod store;

use authgraph_boringssl as boring;
use authgraph_core::keyexchange::{AuthGraphParticipant, MAX_OPENED_SESSIONS};
use authgraph_core::ta::{AuthGraphTa, Role};
use authgraph_hal::channel::SerializedChannel;
use log::{error, info, Level};
use secretkeeper_core::ta::SecretkeeperTa;
use secretkeeper_hal::SecretkeeperService;
use std::sync::Arc;
use std::sync::Mutex;
use store::InMemoryStore;

use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::{
    BpSecretkeeper, ISecretkeeper,
};
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::mpsc;

/// Implementation of the Secrekeeper TA that runs locally in-process (and which is therefore
/// insecure).
pub struct LocalTa {
    in_tx: mpsc::Sender<Vec<u8>>,
    out_rx: mpsc::Receiver<Vec<u8>>,
}

/// Prefix byte for messages intended for the AuthGraph TA.
const AG_MESSAGE_PREFIX: u8 = 0x00;
/// Prefix byte for messages intended for the Secretkeeper TA.
const SK_MESSAGE_PREFIX: u8 = 0x01;

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Self {
        // Create a pair of channels to communicate with the TA thread.
        let (in_tx, in_rx) = mpsc::channel();
        let (out_tx, out_rx) = mpsc::channel();

        // The TA code expects to run single threaded, so spawn a thread to run it in.
        std::thread::spawn(move || {
            let mut crypto_impls = boring::crypto_trait_impls();
            let storage_impl = Box::new(InMemoryStore::default());
            let sk_ta = Rc::new(RefCell::new(
                SecretkeeperTa::new(&mut crypto_impls, storage_impl)
                    .expect("Failed to create local Secretkeeper TA"),
            ));
            let mut ag_ta = AuthGraphTa::new(
                AuthGraphParticipant::new(crypto_impls, sk_ta.clone(), MAX_OPENED_SESSIONS)
                    .expect("Failed to create local AuthGraph TA"),
                Role::Sink,
            );

            // Loop forever processing request messages.
            loop {
                let req_data: Vec<u8> = in_rx.recv().expect("failed to receive next req");
                let rsp_data = match req_data[0] {
                    AG_MESSAGE_PREFIX => ag_ta.process(&req_data[1..]),
                    SK_MESSAGE_PREFIX => {
                        // It's safe to `borrow_mut()` because this code is not a callback
                        // from AuthGraph (the only other holder of an `Rc`), and so there
                        // can be no live `borrow()`s in this (single) thread.
                        sk_ta.borrow_mut().process(&req_data[1..])
                    }
                    prefix => panic!("unexpected messageprefix {prefix}!"),
                };
                out_tx.send(rsp_data).expect("failed to send out rsp");
            }
        });
        Self { in_tx, out_rx }
    }

    fn execute_for(&mut self, prefix: u8, req_data: &[u8]) -> Vec<u8> {
        let mut prefixed_req = Vec::with_capacity(req_data.len() + 1);
        prefixed_req.push(prefix);
        prefixed_req.extend_from_slice(req_data);
        self.in_tx
            .send(prefixed_req)
            .expect("failed to send in request");
        self.out_rx.recv().expect("failed to receive response")
    }
}

pub struct AuthGraphChannel(Arc<Mutex<LocalTa>>);
impl SerializedChannel for AuthGraphChannel {
    const MAX_SIZE: usize = usize::MAX;
    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self
            .0
            .lock()
            .unwrap()
            .execute_for(AG_MESSAGE_PREFIX, req_data))
    }
}

pub struct SecretkeeperChannel(Arc<Mutex<LocalTa>>);
impl SerializedChannel for SecretkeeperChannel {
    const MAX_SIZE: usize = usize::MAX;
    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self
            .0
            .lock()
            .unwrap()
            .execute_for(SK_MESSAGE_PREFIX, req_data))
    }
}

fn main() {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("NonSecureSecretkeeper")
            .with_min_level(Level::Info)
            .with_log_id(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    let ta = Arc::new(Mutex::new(LocalTa::new()));
    let ag_channel = AuthGraphChannel(ta.clone());
    let sk_channel = SecretkeeperChannel(ta.clone());

    let service = SecretkeeperService::new_as_binder(sk_channel, ag_channel);
    let service_name = format!(
        "{}/nonsecure",
        <BpSecretkeeper as ISecretkeeper>::get_descriptor()
    );
    binder::add_service(&service_name, service.as_binder()).unwrap_or_else(|e| {
        panic!("Failed to register service {service_name} because of {e:?}.",);
    });
    info!("Registered Binder service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
}
